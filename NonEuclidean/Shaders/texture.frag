#version 150
precision highp float;

//Inputs
uniform sampler2D tex;
uniform samplerCube shadowCube;
uniform sampler2D cookieTex;
uniform int hasCookie;
uniform vec3 cam_pos;
uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 emissive;
uniform float far_plane;
in vec2 ex_uv;
in vec3 ex_normal;
in vec3 ex_worldPos;
in vec4 ex_shadowCoord;

//Outputs
out vec4 fragColor;

float calcShadow() {
    vec3 fragToLight = ex_worldPos - light_pos;
    float currentDepth = length(fragToLight) / far_plane;

    // Outside shadow range = no shadow
    if (currentDepth > 1.0) return 1.0;

    float storedDepth = texture(shadowCube, fragToLight).r;
    float bias = 0.002;
    return currentDepth - bias > storedDepth ? 0.0 : 1.0;
}

vec3 sampleCookie() {
    vec3 proj = ex_shadowCoord.xyz / ex_shadowCoord.w;
    vec2 cookieUV = proj.xy * 0.5 + 0.5;
    if (cookieUV.x < 0.0 || cookieUV.x > 1.0 || cookieUV.y < 0.0 || cookieUV.y > 1.0)
    return vec3(0.0);
    return texture(cookieTex, cookieUV).rgb;
}

void main(void) {
    vec3 albedo = texture(tex, ex_uv).rgb;
    vec3 N = normalize(ex_normal);

    // Ambient
    vec3 ambient = 0.15 * albedo;

    // Point light — modulate by cookie if present
    vec3 effectiveLight = light_color;
    if (hasCookie != 0) {
        effectiveLight *= sampleCookie();
    }

    vec3 lightDir = light_pos - ex_worldPos;
    float dist = length(lightDir);
    lightDir = lightDir / dist;
    float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

    // Shadow
    float shadow = calcShadow();

    // Diffuse
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = diff * effectiveLight * albedo * attenuation * shadow;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(cam_pos - ex_worldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(N, halfDir), 0.0), 32.0);
    vec3 specular = spec * effectiveLight * 0.3 * attenuation * shadow;

    fragColor = vec4(ambient + diffuse + specular + emissive * albedo, 1.0);
}

#version 150
precision highp float;

#define PI 3.141592653589793
#define SUN_SIZE 0.004
#define SUN_SHARPNESS 1.0

//Inputs
uniform float time_of_day; // 0.0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset
in vec3 ex_normal;

//Outputs
out vec4 fragColor;

void main(void) {
    vec3 n = normalize(ex_normal);

    // Sun direction: full circle, crossing zenith at noon (0.5)
    float sun_angle = (time_of_day - 0.25) * 2.0 * PI;
    vec3 sun_dir = normalize(vec3(cos(sun_angle), sin(sun_angle), 0.3));
    float sun_height = sun_dir.y;

    // Day/twilight blend factors
    float day = smoothstep(-0.15, 0.15, sun_height);
    float twilight = max(0.0, 1.0 - abs(sun_height) * 5.0);

    // Sky color layers
    float h = max(0.0, n.y);
    vec3 day_top = vec3(0.10, 0.38, 0.90);
    vec3 day_bot = vec3(0.45, 0.68, 1.00);
    vec3 night_sky =    vec3(0.05, 0.01, 0.01);
    vec3 twilight_col = vec3(0.90, 0.35, 0.10);

    vec3 day_sky = mix(day_bot, day_top, h);
    vec3 sky = mix(night_sky, day_sky, day) + twilight_col * twilight * (1.0 - h) * 0.7;

    // Sun disc (hidden below horizon)
    float above = step(0.0, sun_dir.y);
    float s = dot(n, sun_dir) - 1.0 + SUN_SIZE;
    float sun_disc = min(exp(s * SUN_SHARPNESS / SUN_SIZE), 1.0) * above;
    vec3 sun_color = mix(vec3(1.0, 0.55, 0.15), vec3(1.0, 1.0, 0.85), day);

    // Waning crescent moon (opposite the sun)
    float moon_angle = sun_angle + PI;
    vec3 moon_dir = normalize(vec3(cos(moon_angle), sin(moon_angle), 0.3));
    float moon_above = step(0.0, moon_dir.y);
    float night = 1.0 - day;

    // Moon disc: angular distance from moon center
    float moon_r = 0.038; // apparent radius
    float moon_dist = length(n - moon_dir);
    float moon_body = smoothstep(moon_r, moon_r * 0.6, moon_dist);

    // Waning crescent: subtract a shadow disc offset toward the sun
    // Shadow offset pulls the dark limb toward the sun direction in screen space
    float crescent_bias = moon_r * 1.3;
    vec3 shadow_center = normalize(moon_dir + sun_dir * crescent_bias);
    float shadow = smoothstep(moon_r * 1.05, moon_r * 0.65, length(n - shadow_center));
    float crescent = clamp(moon_body - shadow, 0.0, 1.0);

    vec3 moon_color = vec3(0.94, 0.95, 0.84);

    fragColor = vec4(sky + sun_disc * sun_color + crescent * moon_color * night * moon_above * vec3(1.0, 0.95, 0.75) * night * moon_above, 1.0);
}

#version 460
#extension GL_EXT_ray_tracing : require

// ════════════════════════════════════════════════════════════════════
// miss.rmiss  —  PRIMARY MISS  (ray hit nothing)
//
// Called when a primary (or portal-redirected) ray escapes the scene
// without hitting any geometry. Fills payload.rgb with the sky color
// so raygen can tone-map it like any other hit.
//
// SBT miss index 0 — selected by the missIndex=0 argument in the
// traceRayEXT calls in raygen.rgen and portalhit.rchit.
// ════════════════════════════════════════════════════════════════════

layout(location = 0) rayPayloadInEXT vec4 payload;

layout(std140, binding = 2, set = 0) uniform UBO {
    layout(row_major) mat4 viewInv;
    layout(row_major) mat4 projInv;
    vec4 lightPos;
    vec4 lightColor;
    vec4 lightPos2;
    vec4 lightColor2;
    vec4 camPos;       // w = time_of_day
};

#define PI 3.141592653589793
#define SUN_SIZE 0.004
#define SUN_SHARPNESS 1.0

void main() {
    // Ray direction (normalised) is the "look direction" toward the sky.
    vec3 n = normalize(gl_WorldRayDirectionEXT);
    float time_of_day = camPos.w; // [0,1] — drives everything below

    // ── Sun position ───────────────────────────────────────────────
    // Matches the direction computed in closesthit so shadows and the
    // visible disc are always consistent.
    float sun_angle = (time_of_day - 0.25) * 2.0 * PI;
    vec3 sun_dir = normalize(vec3(cos(sun_angle), sin(sun_angle), 0.3));
    float sun_height = sun_dir.y; // positive = above horizon

    float day      = smoothstep(-0.15, 0.15, sun_height);
    float twilight = max(0.0, 1.0 - abs(sun_height) * 5.0);

    // ── Sky gradient ───────────────────────────────────────────────
    // h = vertical component of the ray direction (0=horizon, 1=zenith).
    // Sky transitions from a warm horizon colour at h≈0 to deep blue overhead.
    float h = max(0.0, n.y);
    vec3 day_top      = vec3(0.10, 0.38, 0.90);
    vec3 day_bot      = vec3(0.45, 0.68, 1.00);
    vec3 night_sky    = vec3(0.05, 0.01, 0.01);
    vec3 twilight_col = vec3(0.90, 0.35, 0.10);

    vec3 day_sky = mix(day_bot, day_top, h);
    // Add a warm twilight band near the horizon during sunrise/sunset
    vec3 sky = mix(night_sky, day_sky, day) + twilight_col * twilight * (1.0 - h) * 0.7;

    // ── Sun disc ───────────────────────────────────────────────────
    // Only drawn when the sun is above the horizon (above=1).
    // Uses an exponential falloff to produce a soft bright disc.
    float above = step(0.0, sun_dir.y);
    float s = dot(n, sun_dir) - 1.0 + SUN_SIZE;
    float sun_disc = min(exp(s * SUN_SHARPNESS / SUN_SIZE), 1.0) * above;
    vec3 sun_color = mix(vec3(1.0, 0.55, 0.15), vec3(1.0, 1.0, 0.85), day);

    // ── Moon (waning crescent) ─────────────────────────────────────
    // Moon is opposite the sun in the sky (sun_angle + PI).
    float moon_angle = sun_angle + PI;
    vec3 moon_dir    = normalize(vec3(cos(moon_angle), sin(moon_angle), 0.3));
    float moon_above = step(0.0, moon_dir.y); // hidden below horizon
    float night      = 1.0 - day;             // only visible at night

    // Full moon disc using spherical distance
    float moon_r    = 0.038;
    float moon_dist = length(n - moon_dir);
    float moon_body = smoothstep(moon_r, moon_r * 0.6, moon_dist);

    // Crescent: subtract a shadow disc offset slightly toward the sun
    // to mimic the lit crescent shape of a waning moon.
    float crescent_bias  = moon_r * 1.3;
    vec3  shadow_center  = normalize(moon_dir + sun_dir * crescent_bias);
    float shadow         = smoothstep(moon_r * 1.05, moon_r * 0.65, length(n - shadow_center));
    float crescent       = clamp(moon_body - shadow, 0.0, 1.0);

    vec3 moon_color = vec3(0.94, 0.95, 0.84);

    // ── Final sky colour ──────────────────────────────────────────
    // Written to payload.rgb; returned to raygen which tone-maps it.
    payload.rgb = sky
    + sun_disc * sun_color
    + crescent * moon_color * night * moon_above * vec3(1.0, 0.95, 0.75) * night * moon_above;
}

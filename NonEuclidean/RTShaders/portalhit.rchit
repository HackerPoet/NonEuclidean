#version 460
#extension GL_EXT_ray_tracing : require

// ════════════════════════════════════════════════════════════════════
// portalhit.rchit  —  PORTAL CLOSEST-HIT  (primary ray hits a portal)
//
// Called when a primary (or recursed portal) ray hits a portal quad.
// Does NOT shade — instead it warps the ray through the portal and
// re-fires it, so the pixel eventually receives colour from whatever
// is visible on the other side.
//
// SBT slot: hit group 2 (portal instances use sbtOffset=2, stride=2,
//   primary ray sbtOffset arg=0 → 2+0=2)
//
// Recursion is capped at MAX_PORTAL_DEPTH to prevent infinite loops
// (e.g. two portals facing each other).
// ════════════════════════════════════════════════════════════════════

hitAttributeEXT vec2 bary;
layout(location = 0) rayPayloadInEXT vec4 payload;  // .rgb=color, .w=depth

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

struct InstanceInfo {
    mat4 model;
    mat4 normalMat;
    vec4 emissive;
    uvec4 indices;      // x=meshIndex, y=isPortal, z=textureIndex
    mat4 portalFrontWarp;    // this portal front face -> connected portal
    mat4 portalFrontReverse; // connected portal -> this portal front face
    mat4 portalBackWarp;     // this portal back face -> connected portal
    mat4 portalBackReverse;  // connected portal -> this portal back face
};
layout(std430, row_major, binding = 3, set = 0) readonly buffer Instances {
    InstanceInfo inst[];
};

const int MAX_PORTAL_DEPTH = 4; // max times a single pixel can traverse portals

void main() {
    float depth = payload.w;
    // Guard against infinite portal recursion (e.g. two portals facing each other)
    if (depth >= float(MAX_PORTAL_DEPTH)) {
        payload.rgb = vec3(1.0, 0.0, 1.0); // magenta = recursion limit reached
        return;
    }

    InstanceInfo ii = inst[gl_InstanceCustomIndexEXT];

    // Select warp matrix based on which face of the portal quad was hit.
    // Front face: ray entered from the "front" side → warp to connected portal.
    // Back face:  ray entered from behind (back side) → warp in reverse.
    // This enables one-way portals if desired.
    mat4 deltaInv;
    if (gl_HitKindEXT == gl_HitKindFrontFacingTriangleEXT) {
        deltaInv = ii.portalFrontWarp;
    } else {
        deltaInv = ii.portalBackWarp;
    }

    // Compute the exact world-space hit point on the portal surface
    vec3 hitPt = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    // Transform both origin and direction through the portal warp.
    // The warp maps this portal's local frame onto the connected portal's
    // local frame — so the new ray emerges from the exit portal at the
    // correct position and angle.
    vec3 newOrigin = (deltaInv * vec4(hitPt,                        1.0)).xyz;
    vec3 newDir    = normalize((deltaInv * vec4(gl_WorldRayDirectionEXT, 0.0)).xyz);

    // Increment depth so nested portals eventually terminate
    payload.w = depth + 1.0;
    // Re-fire the ray from the exit portal. This may hit:
    //   regular geometry → closesthit (full shading)
    //   another portal   → portalhit again (further recursion)
    //   nothing          → miss (sky)
    // Whichever shader runs will overwrite payload.rgb with the colour
    // seen through this portal opening.
    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xff, 0, 2, 0, newOrigin, 0.001, newDir, 10000.0, 0);
}

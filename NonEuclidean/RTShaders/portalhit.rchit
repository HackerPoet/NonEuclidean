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
    uvec4 indices;      // x=meshIndex, y=isPortal, z=textureIndex, w=portalWarpIndex
};
layout(std430, row_major, binding = 3, set = 0) readonly buffer Instances {
    InstanceInfo inst[];
};

struct PortalWarpInfo {
    mat4 frontWarp;    // this portal front face -> connected portal (deltaInv)
    mat4 frontReverse; // connected portal -> this portal front face (delta)
    mat4 backWarp;     // this portal back face -> connected portal (deltaInv)
    mat4 backReverse;  // connected portal -> this portal back face (delta)
    vec4 center;       // precomputed world-space center xyz
    vec4 forward;      // precomputed world-space forward dir xyz
    vec4 axisX;        // precomputed world-space X axis xyz (includes scale)
    vec4 axisY;        // precomputed world-space Y axis xyz (includes scale)
    vec4 aabbMin;      // influence volume min xyz
    vec4 aabbMax;      // influence volume max xyz
};
layout(std430, row_major, binding = 8, set = 0) readonly buffer PortalWarps {
    PortalWarpInfo portalWarps[];
};

// Hash-based noise for breaking up the recursion-limit boundary
float hash31(vec3 p) {
    p = fract(p * vec3(443.8975, 397.2973, 491.1871));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

const int MAX_PORTAL_DEPTH = 12; // max times a single pixel can traverse portals

void main() {
    float depth = payload.w;
    // Guard against infinite portal recursion (e.g. two portals facing each other)
    if (depth >= float(MAX_PORTAL_DEPTH)) {
        // Still warp through the portal but trace against geometry only
        // (mask 0x01 skips portal quads) so no further recursion occurs.
        // Darken and add noise to hide the seam.
        InstanceInfo ii = inst[gl_InstanceCustomIndexEXT];
        PortalWarpInfo pw = portalWarps[ii.indices.w];
        mat4 deltaInv;
        if (gl_HitKindEXT == gl_HitKindFrontFacingTriangleEXT) {
            deltaInv = pw.frontWarp;
        } else {
            deltaInv = pw.backWarp;
        }
        vec3 hitPt = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
        vec3 newOrigin = (deltaInv * vec4(hitPt, 1.0)).xyz;
        vec3 newDir    = normalize((deltaInv * vec4(gl_WorldRayDirectionEXT, 0.0)).xyz);

        traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0x01, 0, 2, 0, newOrigin, 0.001, newDir, 10000.0, 0);

        float n = hash31(hitPt * 100.0 + newDir * 50.0);
        payload.rgb = payload.rgb * 0.5 + (n - 0.5) * 0.08;
        return;
    }

    InstanceInfo ii = inst[gl_InstanceCustomIndexEXT];
    PortalWarpInfo pw = portalWarps[ii.indices.w];

    // Select warp matrix based on which face of the portal quad was hit.
    // Front face: ray entered from the "front" side → warp to connected portal.
    // Back face:  ray entered from behind (back side) → warp in reverse.
    // This enables one-way portals if desired.
    mat4 deltaInv;
    if (gl_HitKindEXT == gl_HitKindFrontFacingTriangleEXT) {
        deltaInv = pw.frontWarp;
    } else {
        deltaInv = pw.backWarp;
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

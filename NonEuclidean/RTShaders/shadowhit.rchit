#version 460
#extension GL_EXT_ray_tracing : require

// ════════════════════════════════════════════════════════════════════
// shadowhit.rchit  —  SHADOW CLOSEST-HIT  (occluder between surface and light)
//
// Called when a shadow ray (fired from closesthit toward the sun or
// a point light) hits any regular geometry before reaching the light.
// Sets shadowPayload = 0.0 to tell closesthit that this surface is
// in shadow.
//
// SBT slot: hit group 1 (regular instances, sbtOffset=0, stride=2,
//   shadow ray sbtOffset arg=1 → 0+1=1)
// ════════════════════════════════════════════════════════════════════

layout(location = 1) rayPayloadInEXT float shadowPayload;

void main() {
    // An occluder was hit — the surface is blocked from the light.
    // closesthit multiplies lighting terms by this value (0 = no light).
    shadowPayload = 0.0;
}

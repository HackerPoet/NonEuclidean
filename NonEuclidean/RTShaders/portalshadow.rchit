#version 460
#extension GL_EXT_ray_tracing : require

// ════════════════════════════════════════════════════════════════════
// portalshadow.rchit  —  PORTAL SHADOW HIT  (shadow ray hits a portal)
//
// Called when a shadow ray hits a portal quad on its way to the light.
// In the current closesthit setup, ordinary shadow and portal-light
// validation rays use cull mask 0x01, so they ignore portal instances and
// usually never reach this shader. If a ray is traced with a mask that does
// include portals, portal surfaces are treated as opaque occluders.
//
// This is intentional: shadow rays are not recursive and do not warp
// through portals. A surface on the far side of a portal is lit via
// the virtual light positions computed in closesthit (evalPointLight
// loop), not by having shadow rays tunnel through portals.
//
// SBT slot: hit group 3 (portal instances, sbtOffset=2, stride=2,
//   shadow ray sbtOffset arg=1 → 2+1=3)
// ════════════════════════════════════════════════════════════════════

layout(location = 1) rayPayloadInEXT float shadowPayload;

void main() {
    // Portal quad is in the way — treat as an opaque occluder.
    // The virtual-light technique in closesthit handles portal light
    // transmission without needing shadow rays to warp.
    shadowPayload = 0.0;
}

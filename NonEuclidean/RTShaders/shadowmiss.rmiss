#version 460
#extension GL_EXT_ray_tracing : require

// ════════════════════════════════════════════════════════════════════
// shadowmiss.rmiss  —  SHADOW MISS  (nothing between surface and light)
//
// Called when a shadow ray travels its full distance (to the sun or
// just short of the point light) without hitting anything. The
// surface is fully lit by that light source.
//
// SBT miss index 1 — selected by the missIndex=1 argument in the
// shadow traceRayEXT calls in closesthit.rchit / evalPointLight.
// ════════════════════════════════════════════════════════════════════

layout(location = 1) rayPayloadInEXT float shadowPayload;

void main() {
    // Nothing blocked the path to the light — surface is lit.
    // closesthit multiplies lighting terms by this value (1 = full light).
    shadowPayload = 1.0;
}

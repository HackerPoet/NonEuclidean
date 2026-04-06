#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

// ════════════════════════════════════════════════════════════════════
// closesthit.rchit  —  PRIMARY CLOSEST-HIT  (regular geometry)
//
// Called when a primary (or portal-redirected) ray hits a regular
// (non-portal) triangle. Responsible for all shading of visible
// surfaces. Internally fires additional shadow rays, which will call
// either shadowhit/portalshadow (occluded) or shadowmiss (lit).
//
// SBT slot: hit group 0 (sbtOffset=0, stride=2 → index 0+0=0)
// ════════════════════════════════════════════════════════════════════

hitAttributeEXT vec2 bary;  // barycentric coords of the hit point within the triangle
layout(location = 0) rayPayloadInEXT vec4 payload;       // primary payload: .rgb=color, .w=portal depth
layout(location = 1) rayPayloadEXT   float shadowPayload; // shadow sub-ray result: 1.0=lit, 0.0=occluded

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(std140, binding = 2, set = 0) uniform UBO {
    layout(row_major) mat4 viewInv;
    layout(row_major) mat4 projInv;
    vec4 camPos;
    vec4 params;  // x = ambientScale, y = lightCount
};

struct GPULight {
    vec4 pos;
    vec4 color;
};
layout(std430, binding = 13, set = 0) readonly buffer Lights {
    GPULight lights[];
};

struct InstanceInfo {
    mat4 model;
    mat4 normalMat;
    vec4 emissive;      // xyz = emissive
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
layout(std430, binding = 9, set = 0) readonly buffer PortalIndices {
    uint portalIndices[];
};
layout(std430, binding = 4, set = 0) readonly buffer Positions {
    float pos[];
};
layout(std430, binding = 5, set = 0) readonly buffer Normals {
    float nrm[];
};
layout(std430, binding = 6, set = 0) readonly buffer UVs {
    float uvs[];
};
layout(std430, binding = 7, set = 0) readonly buffer MeshOff {
    uint meshOff[];
};
layout(std430, binding = 11, set = 0) readonly buffer Colors {
    float col[];
};
layout(std430, binding = 12, set = 0) readonly buffer TexIndices {
    uint vtex[];
};
layout(binding = 10, set = 0) uniform sampler2D textures[];

vec3 getPos(uint i) {
    return vec3(pos[i*3], pos[i*3+1], pos[i*3+2]);
}
vec3 getNrm(uint i) {
    return vec3(nrm[i*3], nrm[i*3+1], nrm[i*3+2]);
}
vec2 getUV(uint i) {
    return vec2(uvs[i*2], uvs[i*2+1]);
}
vec3 getCol(uint i) {
    return vec3(col[i*3], col[i*3+1], col[i*3+2]);
}

vec3 mulPoint(mat4 m, vec3 p) {
    return (m * vec4(p, 1.0)).xyz;
}

vec3 mulDir(mat4 m, vec3 d) {
    return (m * vec4(d, 0.0)).xyz;
}

vec3 offsetRayOrigin(vec3 pos, vec3 geomNrm, vec3 rayDir) {
    float ndotd = dot(geomNrm, rayDir);
    vec3 offsetNrm = ndotd >= 0.0 ? geomNrm : -geomNrm;
    // Larger bias at grazing angles reduces shadow acne on broad flat
    // surfaces without pushing contacts too far away at head-on angles.
    float bias = 0.002 + (1.0 - abs(ndotd)) * 0.008;
    return pos + offsetNrm * bias;
}

#define PI 3.141592653589793

void main() {
    // ── Reconstruct hit surface ────────────────────────────────────
    // gl_InstanceCustomIndexEXT is the index into the Instances SSBO
    // set when the TLAS instance was built (customIndex = instanceInfos.size()).
    InstanceInfo ii = inst[gl_InstanceCustomIndexEXT];
    // meshOff[meshIndex] gives the base vertex index for this mesh in the
    // global flattened vertex arrays (pos[], nrm[], uvs[]).
    uint base = meshOff[ii.indices.x];
    // gl_PrimitiveID is the triangle index within this BLAS geometry.
    uint v0 = base + gl_PrimitiveID * 3;

    // bary.xy are the barycentric coords from the hardware intersection;
    // reconstruct the third so all three sum to 1.
    vec3 b = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);

    // Interpolate position, normal, and UV across the three triangle vertices
    vec3 lpos = b.x * getPos(v0) + b.y * getPos(v0+1) + b.z * getPos(v0+2);
    vec3 lnrm = b.x * getNrm(v0) + b.y * getNrm(v0+1) + b.z * getNrm(v0+2);
    vec2 luv  = b.x * getUV(v0)  + b.y * getUV(v0+1)  + b.z * getUV(v0+2);
    vec3 lcol = b.x * getCol(v0) + b.y * getCol(v0+1) + b.z * getCol(v0+2);

    // Transform position to world space using the instance model matrix.
    // Normals use the transpose-inverse (normalMat) so non-uniform scale
    // doesn't shear them.
    vec3 worldPos = (ii.model * vec4(lpos, 1.0)).xyz;
    vec3 worldNrm = normalize((ii.normalMat * vec4(lnrm, 0.0)).xyz);
    vec3 wp0 = mulPoint(ii.model, getPos(v0));
    vec3 wp1 = mulPoint(ii.model, getPos(v0+1));
    vec3 wp2 = mulPoint(ii.model, getPos(v0+2));
    vec3 geomNrm = normalize(cross(wp1 - wp0, wp2 - wp0));
    if (dot(geomNrm, worldNrm) < 0.0) {
        geomNrm = -geomNrm;
    }
    const int numPointLights = int(params.y);

    // Build the list of portals visible from the shaded world-space hit point.
    // Each point light builds its own portal-visible list later so portal-
    // warped lighting can be evaluated independently per light.
    const int MAX_VISIBLE_PORTALS = 8;
    uint visiblePortalList[MAX_VISIBLE_PORTALS];
    uint visiblePortalFace[MAX_VISIBLE_PORTALS];
    int visiblePortalCount = 0; {
        int nPortals = int(portalIndices.length());

        for (int pi = 0; pi < nPortals && visiblePortalCount < MAX_VISIBLE_PORTALS; ++pi) {
            uint portalIndex = portalIndices[pi];
            PortalWarpInfo pw = portalWarps[inst[portalIndex].indices.w];

            // AABB early-out: skip if surface point is outside portal influence volume
            if (any(lessThan(worldPos, pw.aabbMin.xyz)) || any(greaterThan(worldPos, pw.aabbMax.xyz))) continue;

            vec3 portalPos = pw.center.xyz;
            vec3 portalFwd = pw.forward.xyz;
            uint portalFace = dot(portalFwd, worldPos - portalPos) > 0.0 ? 0u : 1u;
            vec3 toPortal = portalPos - worldPos;
            float portalDist = length(toPortal);
            if (portalDist <= 0.002) continue;

            vec3 portalDir = toPortal / portalDist;
            vec3 portalOrigin = offsetRayOrigin(worldPos, geomNrm, portalDir);

            shadowPayload = 1.0;
            traceRayEXT(tlas,
                gl_RayFlagsOpaqueEXT,
                0x01, 1, 2, 1,
                portalOrigin, 0.001, portalDir, portalDist - 0.002,
                1);

            if (shadowPayload > 0.5) {
                visiblePortalList[visiblePortalCount] = portalIndex;
                visiblePortalFace[visiblePortalCount] = portalFace;
                visiblePortalCount++;
            }
        }
    }

    // ── Material ───────────────────────────────────────────────────
    uint perVertTexIdx = vtex[v0]; // per-vertex texture index (0 = use per-instance)
    uint texIdx = perVertTexIdx != 0u ? perVertTexIdx : ii.indices.z;
    vec3 albedo   = texture(textures[nonuniformEXT(texIdx)], luv).rgb * lcol;
    vec3 emissive = ii.emissive.xyz;  // surfaces like the light cube glow

    // V points from the hit point back toward the ray origin.
    // Using gl_WorldRayOriginEXT (not camera pos) keeps specular correct
    // for rays that have been redirected through portals.
    vec3 V = normalize(gl_WorldRayOriginEXT - worldPos);
    float time_of_day = camPos.w;  // drives sun/moon position

    // Accumulate every valid point-light path that reaches this surface.
    // This includes the direct path plus one-hop portal-warped paths found by
    // pairing a portal visible from worldPos with its connected portal that is
    // visible from each light. Paths are summed rather than collapsed to one.
    vec3 ptDiffuse = vec3(0.0);
    vec3 ptSpecular = vec3(0.0);
    for (int lightIndex = 0; lightIndex < numPointLights; ++lightIndex) {
        vec3 pointLightPos = lights[lightIndex].pos.xyz;
        float lightRadius = lights[lightIndex].pos.w; // shadow-skip radius around light
        vec3 pointLightColor = lights[lightIndex].color.xyz;
        if (length(pointLightColor) <= 0.001) continue;

        // ── Direct shadow ───────────────────────────────────────────
        vec3 toDirectLight = pointLightPos - worldPos;
        float directDist = length(toDirectLight);
        if (directDist > 0.002) {
            vec3 directDir = toDirectLight / directDist;
            vec3 directOrigin = offsetRayOrigin(worldPos, geomNrm, directDir);

            shadowPayload = 1.0;
            traceRayEXT(tlas,
                gl_RayFlagsOpaqueEXT,
                0x01, 1, 2, 1,
                directOrigin, 0.001, directDir, directDist - lightRadius - 0.002,
                1);
            float directShadow = shadowPayload;

            float directAtten = 1.0 / (1.0 + directDist * directDist * 0.003);
            float directDiffuseTerm = max(dot(worldNrm, directDir), 0.0);
            ptDiffuse += directDiffuseTerm * pointLightColor * albedo * directAtten * directShadow;

            vec3 H_direct = normalize(directDir + V);
            float directSpec = pow(max(dot(worldNrm, H_direct), 0.0), 32.0);
            ptSpecular += directSpec * pointLightColor * 0.3 * directAtten * directShadow;
        }

        // ── Portal-warped lighting ──────────────────────────────────
        // For each portal visible from the surface, compute the virtual
        // light position through the portal warp and check reachability
        // with just 2 shadow rays. This avoids the old O(lights×portals)
        // per-light visibility scan.
        for (int i = 0; i < visiblePortalCount; ++i) {
            uint portalIdx = visiblePortalList[i];
            InstanceInfo portalInfo = inst[portalIdx];
            PortalWarpInfo pw = portalWarps[portalInfo.indices.w];

            mat4 portalWarp = (visiblePortalFace[i] == 0u)
            ? pw.frontWarp
            : pw.backWarp;
            mat4 portalReverse = (visiblePortalFace[i] == 0u)
            ? pw.frontReverse
            : pw.backReverse;

            vec3 portalPos = pw.center.xyz;
            vec3 portalFwd = pw.forward.xyz;
            vec3 portalX = pw.axisX.xyz;
            vec3 portalY = pw.axisY.xyz;

            vec3 virtualLight = mulPoint(portalReverse, pointLightPos);
            float worldSide = dot(portalFwd, worldPos - portalPos);
            float lightSide = dot(portalFwd, virtualLight - portalPos);
            if (worldSide * lightSide > 0.0) continue;

            vec3 currentToLight = virtualLight - worldPos;
            float currentToLightLen = length(currentToLight);
            if (currentToLightLen <= 0.001) continue;
            vec3 currentDir = currentToLight / currentToLightLen;

            float denom = dot(portalFwd, currentDir);
            if (abs(denom) < 1e-5) continue;

            float tHit = dot(portalFwd, portalPos - worldPos) / denom;
            if (tHit <= 0.002) continue;

            vec3 portalCrossing = worldPos + currentDir * tHit;
            vec3 delta = portalCrossing - portalPos;
            float portalXLenSq = dot(portalX, portalX);
            float portalYLenSq = dot(portalY, portalY);
            if (portalXLenSq <= 1e-6 || portalYLenSq <= 1e-6) continue;

            float u = dot(delta, portalX) / portalXLenSq;
            float v = dot(delta, portalY) / portalYLenSq;
            if (abs(u) > 1.0 || abs(v) > 1.0) continue;

            vec3 warpedCrossing = mulPoint(portalWarp, portalCrossing);
            vec3 farToLight = pointLightPos - warpedCrossing;
            float farDist = length(farToLight);
            if (farDist <= 0.002) continue;
            vec3 farDir = farToLight / farDist;

            // Validate the pre-portal segment against regular shadow-casting
            // geometry only. Portal quads are intentionally ignored here so
            // the portal itself does not veto a valid warped-light path.
            vec3 portalEntryOrigin = offsetRayOrigin(worldPos, geomNrm, currentDir);
            shadowPayload = 1.0;
            traceRayEXT(tlas,
                gl_RayFlagsOpaqueEXT,
                0x01, 1, 2, 1,
                portalEntryOrigin, 0.001, currentDir, tHit - 0.002,
                1);
            float portalShadow = shadowPayload;
            if (portalShadow <= 0.0) continue;

            // Same rule for the far-room segment: ignore portal instances and
            // only check for real occluding geometry between the exit point
            // and the light source.
            vec3 exitPortalNrm = normalize(mulDir(portalWarp, portalFwd));
            vec3 portalExitOrigin = offsetRayOrigin(warpedCrossing, exitPortalNrm, farDir);
            shadowPayload = 1.0;
            traceRayEXT(tlas,
                gl_RayFlagsOpaqueEXT,
                0x01, 1, 2, 1,
                portalExitOrigin, 0.001, farDir, farDist - lightRadius - 0.002,
                1);
            portalShadow *= shadowPayload;
            if (portalShadow <= 0.0) continue;

            float portalDist = tHit + farDist;
            float portalAtten = 1.0 / (1.0 + portalDist * portalDist * 0.003);
            float portalDiffuseTerm = max(dot(worldNrm, currentDir), 0.0);
            ptDiffuse += portalDiffuseTerm * pointLightColor * albedo * portalAtten * portalShadow;

            vec3 H_portal = normalize(currentDir + V);
            float portalSpec = pow(max(dot(worldNrm, H_portal), 0.0), 32.0);
            ptSpecular += portalSpec * pointLightColor * 0.3 * portalAtten * portalShadow;
        }
    }

    // ── Sun (directional light) ────────────────────────────────────
    // The sun completes a full circle in the XY plane (+ slight Z tilt)
    // over [0,1] of time_of_day. At 0.25 it is at the horizon; at 0.5
    // it is at the zenith.
    float sun_angle = (time_of_day - 0.25) * 2.0 * PI;
    vec3 sun_dir = normalize(vec3(cos(sun_angle), sin(sun_angle), 0.3));
    float sun_height = sun_dir.y; // positive = above horizon

    // day: 0 at night, 1 during day, smooth crossover around horizon
    float day = smoothstep(-0.15, 0.15, sun_height);
    // twilight: spikes to 1 only when sun is near the horizon
    float twilight = max(0.0, 1.0 - abs(sun_height) * 5.0);

    // Sun color shifts from orange at horizon to near-white overhead
    vec3 sun_color = mix(vec3(1.0, 0.55, 0.15), vec3(1.0, 0.95, 0.85),
        smoothstep(0.0, 0.5, sun_height)) * day * 2.5;

    // Sun shadow ray: directional, so tMax is huge (effectively infinite).
    // Skipped entirely at night (day ~= 0) to avoid wasted traces.
    // Uses mask 0x01 and sbtOffset 1, same shadow pipeline as point light.
    float sunShadow = 1.0;
    if (day > 0.001) {
        vec3 sunOrigin = offsetRayOrigin(worldPos, geomNrm, sun_dir);
        shadowPayload = 1.0;
        traceRayEXT(tlas,
            gl_RayFlagsOpaqueEXT,
            0x01, 1, 2, 1,
            sunOrigin, 0.001, sun_dir, 10000.0,
            1);
        sunShadow = shadowPayload; // 0 = in shadow, 1 = lit by sun
    }

    // Blinn-Phong sun diffuse + specular
    float sunDiff = max(dot(worldNrm, sun_dir), 0.0);
    vec3 sunDiffuse = sunDiff * sun_color * albedo * sunShadow;

    vec3 H_sun = normalize(sun_dir + V);
    float sunSpec = pow(max(dot(worldNrm, H_sun), 0.0), 64.0); // tighter than point light
    vec3 sunSpecular = sunSpec * sun_color * 0.4 * sunShadow;

    // ── Point light ────────────────────────────────────────────────
    // ptDiffuse / ptSpecular above already contain the summed direct and
    // portal-warped point-light contributions for this surface.

    // ── Ambient ───────────────────────────────────────────────────
    // Constant fill light approximating sky/bounce irradiance.
    // Blends from dim blue-night to lighter blue-day, with a warm
    // orange spike at twilight.
    vec3 dayAmb = vec3(0.18, 0.22, 0.30);
    vec3 nightAmb = vec3(0.02, 0.015, 0.03);
    vec3 twilightAmb = vec3(0.15, 0.08, 0.04);
    vec3 ambient = (mix(nightAmb, dayAmb, day) + twilightAmb * twilight * 0.1) * albedo * params.x;

    // ── Combine all terms → final pixel radiance ──────────────────
    // This value is written into payload.rgb and returned to whoever
    // called traceRayEXT: either raygen (first hit) or portalhit
    // (recursed through a portal). raygen will tone-map it.
    payload.rgb = ambient
    + sunDiffuse + sunSpecular
    + ptDiffuse + ptSpecular
    + emissive * albedo; // emissive surfaces (e.g. light cube) glow
}

// portal_warp_test.cpp
// CPU-side unit test mirroring the portal warp math in portalhit.rchit:
//
//   hitPt     = rayOrigin + rayDir * t
//   newOrigin = (deltaInv * vec4(hitPt,  1.0)).xyz
//   newDir    = normalize((deltaInv * vec4(rayDir, 0.0)).xyz)
//
// Build via CMake (see root CMakeLists.txt) or manually:
//   cl /std:c++14 /I ../NonEuclidean/game portal_warp_test.cpp

#include <cstdio>
#include <cmath>
#include "Vector.h"

// ---------------------------------------------------------------------------
// Exact port of the shader warp
// ---------------------------------------------------------------------------
static void portalWarp(
    const Matrix4& deltaInv,
    const Vector3& rayOrigin, const Vector3& rayDir, float t,
    Vector3& newOrigin, Vector3& newDir)
{
    Vector3 hitPt = rayOrigin + rayDir * t;
    newOrigin = (deltaInv * Vector4(hitPt, 1.0f)).XYZ();
    newDir    = (deltaInv * Vector4(rayDir, 0.0f)).XYZ().Normalized();
}

// ---------------------------------------------------------------------------
// Mirrors of Object::LocalToWorld / WorldToLocal (Object.cpp + Vector.h)
// These replicate the engine math without linking the full engine.
// ---------------------------------------------------------------------------
static Matrix4 objectLocalToWorld(const Vector3& pos, const Vector3& euler, const Vector3& scale)
{
    return Matrix4::Trans(pos)
        * Matrix4::RotY(euler.y)
        * Matrix4::RotX(euler.x)
        * Matrix4::RotZ(euler.z)
        * Matrix4::Scale(scale);
}

static Matrix4 objectWorldToLocal(const Vector3& pos, const Vector3& euler, const Vector3& scale)
{
    return Matrix4::Scale(Vector3(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z))
        * Matrix4::RotZ(-euler.z)
        * Matrix4::RotX(-euler.x)
        * Matrix4::RotY(-euler.y)
        * Matrix4::Trans(-pos);
}

// Traversal warp used by portalhit.rchit and uploaded as portal*Warp.
// Maps from the current portal face's world space to the connected face.
static Matrix4 connectTraversalWarp(const Matrix4& from_w2l, const Matrix4& to_l2w)
{
    return to_l2w * from_w2l;
}

// Reverse warp uploaded for closesthit.rchit so it can pull light positions
// from the far room back into the current room without a shader-side inverse.
static Matrix4 connectReverseWarp(const Matrix4& from_l2w, const Matrix4& to_w2l)
{
    return from_l2w * to_w2l;
}

// Specular reflection of a direction off a surface normal.
static Vector3 reflect(const Vector3& dir, const Vector3& normal)
{
    return dir - normal * (2.0f * dir.Dot(normal));
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool approxEq(const Vector3& a, const Vector3& b, float eps = 1e-4f)
{
    return (a - b).Mag() < eps;
}

static int g_passed = 0;
static int g_failed = 0;

static void check(const char* name, const Vector3& got, const Vector3& expected)
{
    if (approxEq(got, expected)) {
        printf("PASS  %s\n", name);
        ++g_passed;
    } else {
        printf("FAIL  %s\n", name);
        printf("      got      (%.5f, %.5f, %.5f)\n", got.x, got.y, got.z);
        printf("      expected (%.5f, %.5f, %.5f)\n", expected.x, expected.y, expected.z);
        ++g_failed;
    }
}

static void checkBool(const char* name, bool ok)
{
    if (ok) {
        printf("PASS  %s\n", name);
        ++g_passed;
    } else {
        printf("FAIL  %s\n", name);
        ++g_failed;
    }
}

static Vector3 portalForward(const Matrix4& portal_l2w)
{
    return (-portal_l2w.ZAxis()).Normalized();
}

static bool portalLightPath(
    const Matrix4& warp,
    const Matrix4& reverseWarp,
    const Vector3& worldPos,
    const Vector3& lightPos,
    const Vector3& portalPos,
    const Vector3& portalFwd,
    const Vector3& portalX,
    const Vector3& portalY,
    float& tHit,
    Vector3& portalCrossing,
    Vector3& warpedCrossing,
    Vector3& currentDir,
    Vector3& farDir)
{
    const Vector3 virtualLight = reverseWarp.MulPoint(lightPos);
    const float worldSide = portalFwd.Dot(worldPos - portalPos);
    const float lightSide = portalFwd.Dot(virtualLight - portalPos);
    if (worldSide * lightSide > 0.0f) {
        return false;
    }

    currentDir = (virtualLight - worldPos).Normalized();
    const float denom = portalFwd.Dot(currentDir);
    if (std::abs(denom) < 1e-5f) {
        return false;
    }

    tHit = portalFwd.Dot(portalPos - worldPos) / denom;
    if (tHit <= 0.001f) {
        return false;
    }

    portalCrossing = worldPos + currentDir * tHit;
    const Vector3 delta = portalCrossing - portalPos;
    const float u = delta.Dot(portalX) / portalX.MagSq();
    const float v = delta.Dot(portalY) / portalY.MagSq();
    if (std::abs(u) > 1.0f || std::abs(v) > 1.0f) {
        return false;
    }

    warpedCrossing = warp.MulPoint(portalCrossing);
    farDir = (lightPos - warpedCrossing).Normalized();
    return true;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Identity: ray should pass through completely unchanged.
static void test_identity()
{
    Matrix4 I = Matrix4::Identity();
    Vector3 newO, newD;
    portalWarp(I, Vector3(0,0,0), Vector3(0,0,1), 5.0f, newO, newD);
    check("identity: origin",    newO, Vector3(0, 0, 5));
    check("identity: direction", newD, Vector3(0, 0, 1));
}

// Pure translation: only the origin shifts; direction is unaffected (w=0).
static void test_pure_translation()
{
    Matrix4 T = Matrix4::Trans(Vector3(3, 0, 0));
    Vector3 newO, newD;
    portalWarp(T, Vector3(0,0,0), Vector3(0,0,1), 5.0f, newO, newD);
    check("translate: origin",    newO, Vector3(3, 0, 5));
    check("translate: direction", newD, Vector3(0, 0, 1));
}

// 90-degree Y rotation: +Z ray becomes +X.
static void test_rotation_y90()
{
    const float pi_2 = 3.14159265f / 2.0f;
    Matrix4 R = Matrix4::RotY(pi_2);
    Vector3 newO, newD;
    portalWarp(R, Vector3(0,0,0), Vector3(0,0,1), 0.0f, newO, newD);
    check("rotY90: direction", newD, Vector3(1, 0, 0));
}

// 180-degree Y rotation + translation: realistic portal that flips the ray
// and teleports it to a different position.
//   – origin at (0,0,0), ray along +Z, hits portal at t=0
//   – portal warps to z=10, facing -Z
static void test_combined_portal()
{
    const float pi = 3.14159265f;
    Matrix4 R = Matrix4::RotY(pi);
    Matrix4 T = Matrix4::Trans(Vector3(0, 0, 10));
    Matrix4 deltaInv = T * R;   // apply rotation first, then translation
    Vector3 newO, newD;
    portalWarp(deltaInv, Vector3(0,0,0), Vector3(0,0,1), 0.0f, newO, newD);
    check("combined portal: origin",    newO, Vector3(0, 0, 10));
    check("combined portal: direction", newD, Vector3(0, 0, -1));
}

// Ray that hits at a non-zero t: the hit point must be transformed, not the origin.
static void test_hit_at_nonzero_t()
{
    Matrix4 T = Matrix4::Trans(Vector3(0, 5, 0));
    Vector3 newO, newD;
    // Ray starts at (1,0,0), travels in +Z, hits at t=3 → hitPt = (1,0,3)
    // After +Y translation: (1, 5, 3)
    portalWarp(T, Vector3(1,0,0), Vector3(0,0,1), 3.0f, newO, newD);
    check("nonzero t: origin",    newO, Vector3(1, 5, 3));
    check("nonzero t: direction", newD, Vector3(0, 0, 1));
}

// ---------------------------------------------------------------------------
// Level7: ray bounces off the light cube's left face, then hits portal A.
// Verifies the final position and direction land on the other side of portal B.
//
// Scene geometry (Level7.cpp):
//   Light cube  : pos=(7, 0.5, -8), emissive; left face at x=6.5, normal=(-1,0,0)
//   frame1      : pos=(4,0,-8), scale=(1,1,0.1), euler.y=-PI/2
//   portal A    : frame1.SetDoor1 => pos~(3.9,1,-8), euler.y=-PI/2, scale=(0.6,0.999,1)
//   frame2      : pos=(-5,0,5),  scale=(1,1,0.1), euler=(0,0,0)
//   portal B    : frame2.SetDoor1 => pos~(-5,1,5.1), euler=(0,0,0), scale=(0.6,0.999,1)
//   Portal::Connect(portal1, portal2)
//
// Ray path:
//   Segment 1 — origin=(0,1,-8), dir=(+1,0,0)
//               travels along z=-8, y=1 until it hits the cube's left face at (6.5,1,-8)
//   Bounce     — specular reflection off normal (-1,0,0) => reflected dir = (-1,0,0)
//   Segment 2 — origin=(6.5,1,-8), dir=(-1,0,0), t=2.6 => hitPt=(3.9,1,-8) = portal A center
//   Warp       — apply portal A's front deltaInv
//   Expected   — newOrigin ≈ portal B center = (-5, 1, 5.1)
//                newDir    ≈ (0, 0, 1)  (through portal B, outward)
// ---------------------------------------------------------------------------
static const float GH_PI_TEST = 3.141592653589793f;

static void test_level7_light_bounce_then_portal()
{
    // ---- Replicate Level7::Load portal setup ----------------------------

    // frame1: pos=(4,0,-8), scale=(1,1,0.1), euler.y=-PI/2
    const Vector3 frame1Pos   = Vector3(4.0f, 0.0f, -8.0f);
    const Vector3 frame1Euler = Vector3(0.0f, -GH_PI_TEST / 2.0f, 0.0f);
    const Vector3 frame1Scale = Vector3(1.0f, 1.0f, 0.1f);
    const Matrix4 frame1_l2w  = objectLocalToWorld(frame1Pos, frame1Euler, frame1Scale);

    // Tunnel::SetDoor1: portal.pos = frame_l2w.MulPoint(0,1,1)
    //                  portal.euler = frame.euler
    //                  portal.scale = (0.6, 0.999, 1) * frame.scale.x
    const Vector3 p1Pos   = frame1_l2w.MulPoint(Vector3(0.0f, 1.0f, 1.0f));
    const Vector3 p1Euler = frame1Euler;
    const Vector3 p1Scale = Vector3(0.6f, 0.999f, 1.0f) * frame1Scale.x;

    // frame2: pos=(-5,0,5), scale=(1,1,0.1), euler=(0,0,0)
    const Vector3 frame2Pos   = Vector3(-5.0f, 0.0f, 5.0f);
    const Vector3 frame2Euler = Vector3(0.0f, 0.0f, 0.0f);
    const Vector3 frame2Scale = Vector3(1.0f, 1.0f, 0.1f);
    const Matrix4 frame2_l2w  = objectLocalToWorld(frame2Pos, frame2Euler, frame2Scale);

    const Vector3 p2Pos   = frame2_l2w.MulPoint(Vector3(0.0f, 1.0f, 1.0f));
    const Vector3 p2Euler = frame2Euler;
    const Vector3 p2Scale = Vector3(0.6f, 0.999f, 1.0f) * frame2Scale.x;

    // Portal::Connect warp matrix (front of portal A → back of portal B)
    const Matrix4 p1_w2l  = objectWorldToLocal(p1Pos, p1Euler, p1Scale);
    const Matrix4 p2_l2w  = objectLocalToWorld(p2Pos, p2Euler, p2Scale);
    const Matrix4 deltaInv = connectTraversalWarp(p1_w2l, p2_l2w);

    // ---- Segment 1: player ray hits the light cube's left face ----------
    // Ray: origin=(0,1,-8), dir=(+1,0,0)
    // Cube left face at x=6.5, normal=(-1,0,0)
    const Vector3 seg1Origin = Vector3(0.0f, 1.0f, -8.0f);
    const Vector3 seg1Dir    = Vector3(1.0f, 0.0f, 0.0f);
    const float   seg1T      = 6.5f;   // hitPt.x = 0 + 1*6.5 = 6.5
    const Vector3 hitCube    = seg1Origin + seg1Dir * seg1T;  // (6.5, 1, -8)

    // ---- Bounce off cube left face (normal = (-1,0,0)) ------------------
    const Vector3 cubeNormal  = Vector3(-1.0f, 0.0f, 0.0f);
    const Vector3 seg2Dir     = reflect(seg1Dir, cubeNormal);  // (-1,0,0)

    // ---- Segment 2: reflected ray hits portal A -------------------------
    // hitPt = (6.5,1,-8) + (-1,0,0)*t = (3.9,1,-8) => t = 2.6
    const float   seg2T      = 2.6f;

    // ---- Apply portal warp (shader logic) -------------------------------
    Vector3 newOrigin, newDir;
    portalWarp(deltaInv, hitCube, seg2Dir, seg2T, newOrigin, newDir);

    // ---- Expected: other side of portal B -------------------------------
    // Portal B center = p2Pos, portal B outward direction = (0,0,1)
    check("level7 bounce+portal: origin", newOrigin, p2Pos);
    check("level7 bounce+portal: dir",    newDir,    Vector3(0.0f, 0.0f, 1.0f));
}

static void test_level7_portal_light_alignment()
{
    const Vector3 lightPos = Vector3(7.0f, 0.5f, -8.0f);

    const Vector3 frame1Pos   = Vector3(4.0f, 0.0f, -8.0f);
    const Vector3 frame1Euler = Vector3(0.0f, -GH_PI_TEST / 2.0f, 0.0f);
    const Vector3 frame1Scale = Vector3(1.0f, 1.0f, 0.1f);
    const Matrix4 frame1_l2w  = objectLocalToWorld(frame1Pos, frame1Euler, frame1Scale);

    const Vector3 p1Pos   = frame1_l2w.MulPoint(Vector3(0.0f, 1.0f, 1.0f));
    const Vector3 p1Euler = frame1Euler;
    const Vector3 p1Scale = Vector3(0.6f, 0.999f, 1.0f) * frame1Scale.x;
    const Matrix4 p1_l2w  = objectLocalToWorld(p1Pos, p1Euler, p1Scale);
    const Matrix4 p1_w2l  = objectWorldToLocal(p1Pos, p1Euler, p1Scale);

    const Vector3 frame2Pos   = Vector3(-5.0f, 0.0f, 5.0f);
    const Vector3 frame2Euler = Vector3(0.0f, 0.0f, 0.0f);
    const Vector3 frame2Scale = Vector3(1.0f, 1.0f, 0.1f);
    const Matrix4 frame2_l2w  = objectLocalToWorld(frame2Pos, frame2Euler, frame2Scale);

    const Vector3 p2Pos   = frame2_l2w.MulPoint(Vector3(0.0f, 1.0f, 1.0f));
    const Vector3 p2Euler = frame2Euler;
    const Vector3 p2Scale = Vector3(0.6f, 0.999f, 1.0f) * frame2Scale.x;
    const Matrix4 p2_l2w  = objectLocalToWorld(p2Pos, p2Euler, p2Scale);
    const Matrix4 p2_w2l  = objectWorldToLocal(p2Pos, p2Euler, p2Scale);

    // Light path in room A that goes straight to portal A's center.
    const Matrix4 aFrontWarp = connectTraversalWarp(p1_w2l, p2_l2w);
    const Vector3 dirToPortalA = (p1Pos - lightPos).Normalized();
    const float distToPortalA = (p1Pos - lightPos).Mag();
    Vector3 exitOrigin, exitDir;
    portalWarp(aFrontWarp, lightPos, dirToPortalA, distToPortalA, exitOrigin, exitDir);
    check("level7 portal light: portal exit", exitOrigin, p2Pos);

    // Choose a point in room B that lies on the exact ray emerging from portal B.
    const Vector3 worldPos = exitOrigin + exitDir * 2.0f;

    // From room B, the visible portal face is the back face, so we need the
    // B->A traversal warp plus the reverse A->B mapping for the light image.
    const Matrix4 bBackWarp = connectTraversalWarp(p2_w2l, p1_l2w);
    const Matrix4 bBackReverse = connectReverseWarp(p2_l2w, p1_w2l);

    float tHit = 0.0f;
    Vector3 portalCrossing, warpedCrossing, currentDir, farDir;
    const bool valid = portalLightPath(
        bBackWarp,
        bBackReverse,
        worldPos,
        lightPos,
        p2Pos,
        portalForward(p2_l2w),
        p2_l2w.XAxis(),
        p2_l2w.YAxis(),
        tHit,
        portalCrossing,
        warpedCrossing,
        currentDir,
        farDir);
    checkBool("level7 portal light: path valid", valid);
    if (!valid) {
        return;
    }

    check("level7 portal light: crossing point", portalCrossing, p2Pos);
    check("level7 portal light: warped crossing", warpedCrossing, p1Pos);

    Vector3 portalhitOrigin, portalhitDir;
    portalWarp(bBackWarp, worldPos, currentDir, tHit, portalhitOrigin, portalhitDir);
    check("level7 portal light: portalhit origin", portalhitOrigin, warpedCrossing);
    check("level7 portal light: portalhit dir", portalhitDir, farDir);
    check("level7 portal light: far-room dir", farDir, (lightPos - p1Pos).Normalized());
}

static void test_level8_portal_light_readout()
{
    const Vector3 lightPos = Vector3(0.0f, 1.5f, -18.5f);

    const Vector3 insideFramePos   = Vector3(0.0f, 0.0f, -6.0f);
    const Vector3 outsideFramePos  = Vector3(0.0f, 0.0f, -16.0f);
    const Vector3 frameEuler       = Vector3(0.0f, 0.0f, 0.0f);
    const Vector3 frameScale       = Vector3(1.0f, 1.0f, 0.1f);

    const Matrix4 insideFrame_l2w  = objectLocalToWorld(insideFramePos, frameEuler, frameScale);
    const Matrix4 outsideFrame_l2w = objectLocalToWorld(outsideFramePos, frameEuler, frameScale);

    const Vector3 insidePortalPos   = insideFrame_l2w.MulPoint(Vector3(0.0f, 1.0f, 1.0f));
    const Vector3 outsidePortalPos  = outsideFrame_l2w.MulPoint(Vector3(0.0f, 1.0f, 1.0f));
    const Vector3 portalEuler       = frameEuler;
    const Vector3 portalScale       = Vector3(0.6f, 0.999f, 1.0f) * frameScale.x;

    const Matrix4 insidePortal_l2w  = objectLocalToWorld(insidePortalPos, portalEuler, portalScale);
    const Matrix4 insidePortal_w2l  = objectWorldToLocal(insidePortalPos, portalEuler, portalScale);
    const Matrix4 outsidePortal_l2w = objectLocalToWorld(outsidePortalPos, portalEuler, portalScale);
    const Matrix4 outsidePortal_w2l = objectWorldToLocal(outsidePortalPos, portalEuler, portalScale);

    // From the room side we see the inside portal's back face, which connects
    // to the outside portal's front face where the light sits.
    const Matrix4 insideBackWarp = connectTraversalWarp(insidePortal_w2l, outsidePortal_l2w);
    const Matrix4 insideBackReverse = connectReverseWarp(insidePortal_l2w, outsidePortal_w2l);

    // Point on the actual room floor directly below the portal beam.
    const Vector3 litProbePos = Vector3(0.55f, 0.03f, -4.85f);

    float tHit = 0.0f;
    Vector3 portalCrossing, warpedCrossing, currentDir, farDir;
    const bool valid = portalLightPath(
        insideBackWarp,
        insideBackReverse,
        litProbePos,
        lightPos,
        insidePortalPos,
        portalForward(insidePortal_l2w),
        insidePortal_l2w.XAxis(),
        insidePortal_l2w.YAxis(),
        tHit,
        portalCrossing,
        warpedCrossing,
        currentDir,
        farDir);
    checkBool("level8 portal light: floor patch valid", valid);
    if (!valid) {
        return;
    }

    Vector3 portalhitOrigin, portalhitDir;
    portalWarp(insideBackWarp, litProbePos, currentDir, tHit, portalhitOrigin, portalhitDir);
    check("level8 portal light: portalhit origin", portalhitOrigin, warpedCrossing);
    check("level8 portal light: portalhit dir", portalhitDir, farDir);
    check("level8 portal light: warped crossing offset",
        warpedCrossing, portalCrossing + Vector3(0.0f, 0.0f, -10.0f));

    // A point too far to the right should miss the aperture and receive no portal path.
    float offAxisTHit = 0.0f;
    Vector3 offAxisCrossing, offAxisWarped, offAxisDir, offAxisFarDir;
    const bool offAxisValid = portalLightPath(
        insideBackWarp,
        insideBackReverse,
        Vector3(3.2f, 1.6f, 0.75f),
        lightPos,
        insidePortalPos,
        portalForward(insidePortal_l2w),
        insidePortal_l2w.XAxis(),
        insidePortal_l2w.YAxis(),
        offAxisTHit,
        offAxisCrossing,
        offAxisWarped,
        offAxisDir,
        offAxisFarDir);
    checkBool("level8 portal light: off-axis rejected", !offAxisValid);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main()
{
    printf("=== portal warp tests ===\n\n");

    test_identity();
    test_pure_translation();
    test_rotation_y90();
    test_combined_portal();
    test_hit_at_nonzero_t();
    test_level7_light_bounce_then_portal();
    test_level7_portal_light_alignment();
    test_level8_portal_light_readout();

    printf("\n%d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}

#include "Portal.h"
#include "Engine.h"
#include <cassert>
#include <iostream>

Portal::Portal() : front(this), back(this) {
  mesh = AquireMesh("double_quad.obj");
  shader = AquireShader("portal");
  errShader = AquireShader("pink");
}

void Portal::Draw(const Camera& cam, GLuint curFBO) {
  assert(euler.x == 0.0f);
  assert(euler.z == 0.0f);

  //Draw pink to indicate end of render chain
  if (GH_REC_LEVEL <= 0) {
    DrawPink(cam);
    return;
  }

  //Find normal relative to camera
  Vector3 normal = Forward();
  const Vector3 camPos = cam.worldView.Inverse().Translation();
  const bool frontDirection = (camPos - pos).Dot(normal) > 0;
  const Warp* warp = (frontDirection ? &front : &back);
  if (frontDirection) {
    normal = -normal;
  }

  //Extra clipping to prevent artifacts
  const float extra_clip = GH_MIN(GH_ENGINE->NearestPortalDist() * 0.5f, 0.1f);

  //Create new portal camera
  Camera portalCam = cam;
  portalCam.ClipOblique(pos - normal*extra_clip, -normal);
  portalCam.worldView *= warp->delta;
  portalCam.width = GH_FBO_SIZE;
  portalCam.height = GH_FBO_SIZE;

  //Render portal's view from new camera
  frameBuf[GH_REC_LEVEL - 1].Render(portalCam, curFBO, warp->toPortal);
  cam.UseViewport();

  //Now we can render the portal texture to the screen
  const Matrix4 mv = LocalToWorld();
  const Matrix4 mvp = cam.Matrix() * mv;
  shader->Use();
  frameBuf[GH_REC_LEVEL - 1].Use();
  shader->SetMVP(mvp.m, mv.m);
  mesh->Draw();
}

void Portal::DrawPink(const Camera& cam) {
  const Matrix4 mv = LocalToWorld();
  const Matrix4 mvp = cam.Matrix() * mv;
  errShader->Use();
  errShader->SetMVP(mvp.m, mv.m);
  mesh->Draw();
}

Vector3 Portal::GetBump(const Vector3& a) const {
  const Vector3 n = Forward();
  return n * ((a - pos).Dot(n) > 0 ? 1.0f : -1.0f);
}

const Portal::Warp* Portal::Intersects(const Vector3& a, const Vector3& b, const Vector3& bump) const {
  const Vector3 n = Forward();
  const Vector3 p = pos + bump;
  const float da = n.Dot(a - p);
  const float db = n.Dot(b - p);
  if (da * db > 0.0f) {
    return nullptr;
  }
  const Matrix4 m = LocalToWorld();
  const Vector3 d = a + (b - a) * (da / (da - db)) - p;
  const Vector3 x = (m * Vector4(1, 0, 0, 0)).XYZ();
  if (std::abs(d.Dot(x)) >= x.Dot(x)) {
    return nullptr;
  }
  const Vector3 y = (m * Vector4(0, 1, 0, 0)).XYZ();
  if (std::abs(d.Dot(y)) >= y.Dot(y)) {
    return nullptr;
  }
  return (da > 0.0f ? &front : &back);
}

float Portal::DistTo(const Vector3& pt) const {
  //Get world delta
  const Matrix4 localToWorld = LocalToWorld();
  const Vector3 v = pt - localToWorld.Translation();

  //Get axes
  const Vector3 x = localToWorld.XAxis();
  const Vector3 y = localToWorld.YAxis();

  //Find closest point
  const float px = GH_CLAMP(v.Dot(x) / x.MagSq(), -1.0f, 1.0f);
  const float py = GH_CLAMP(v.Dot(y) / y.MagSq(), -1.0f, 1.0f);
  const Vector3 closest = x*px + y*py;

  //Calculate distance to closest point
  return (v - closest).Mag();
}

void Portal::Connect(std::shared_ptr<Portal>& a, std::shared_ptr<Portal>& b) {
  Connect(a->front, b->back);
  Connect(b->front, a->back);
}

void Portal::Connect(Warp& a, Warp& b) {
  a.toPortal = b.fromPortal;
  b.toPortal = a.fromPortal;
  a.delta = a.fromPortal->LocalToWorld() * b.fromPortal->WorldToLocal();
  b.delta = b.fromPortal->LocalToWorld() * a.fromPortal->WorldToLocal();
  a.deltaInv = b.delta;
  b.deltaInv = a.delta;
}

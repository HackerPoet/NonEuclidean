#include "Portal.h"
#include "Engine.h"
#include <cassert>
#include <iostream>

Portal::Portal() : front(this), back(this) {
  mesh = AquireMesh("double_quad.obj");
  shader = AquireShader("portal");
  errShader = AquireShader("pink");
}

void Portal::Draw(const Camera& cam, uint32_t curFBO) {
  // No-op: portals are rendered by VulkanRenderer via RT
}

void Portal::DrawPink(const Camera& cam) {
  // No-op
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
  // delta moves a point from the connected portal's world space back into
  // this portal face's world space. deltaInv is the traversal warp used by
  // portalhit.rchit to move from this face to the connected face.
  a.delta = a.fromPortal->LocalToWorld() * b.fromPortal->WorldToLocal();
  b.delta = b.fromPortal->LocalToWorld() * a.fromPortal->WorldToLocal();
  a.deltaInv = b.delta;
  b.deltaInv = a.delta;
}

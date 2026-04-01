#include "Physical.h"
#include "GameHeader.h"

Physical::Physical() {
  Reset();
}

void Physical::Reset() {
  Object::Reset();
  velocity.SetZero();
  gravity.Set(0.0f, GH_GRAVITY, 0.0f);
  bounce = 0.0f;
  friction = 0.0f;
  high_friction = 0.0f;
  drag = 0.0f;
  prev_pos.SetZero();
}

void Physical::Update() {
  prev_pos = pos;
  velocity += gravity * p_scale * GH_DT;
  velocity *= (1.0f - drag);
  pos += velocity * GH_DT;
}

void Physical::OnCollide(Object& other, const Vector3& push) {
  //Update position to avoid collision
  pos += push;

  //Ignore push if delta is too small
  if (push.MagSq() < 1e-8f * p_scale) {
    return;
  }

  //Calculate kinetic friction
  float kinetic_friction = friction;
  if (high_friction > 0.0f) {
    const float vel_ratio = velocity.Mag() / (high_friction * p_scale);
    kinetic_friction = GH_MIN(friction * (vel_ratio + 5.0f) / (vel_ratio + 1.0f), 1.0f);
  }

  //Update velocity to react to collision
  const Vector3 push_proj = push * (velocity.Dot(push) / push.Dot(push));
  velocity = (velocity - push_proj) * (1.0f - kinetic_friction) - push_proj * bounce;
}

bool Physical::TryPortal(const Portal& portal) {
  const Vector3 bump = portal.GetBump(prev_pos) * (2 * GH_NEAR_MIN * p_scale);
  const Portal::Warp* warp = portal.Intersects(prev_pos, pos, bump);
  if (warp) {
    //Teleport object
    pos = warp->deltaInv.MulPoint(pos - bump * 2);
    velocity = warp->deltaInv.MulDirection(velocity);
    prev_pos = pos;

    //Update camera direction
    const Vector3 forward(-std::sin(euler.y), 0, -std::cos(euler.y));
    const Vector3 newDir = warp->deltaInv.MulDirection(forward);
    euler.y = -std::atan2(newDir.x, -newDir.z);

    //Update object scale
    p_scale *= warp->deltaInv.XAxis().Mag();
    return true;
  }
  return false;
}

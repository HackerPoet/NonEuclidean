#include "Player.h"
#include "Input.h"
#include "GameHeader.h"
#include <Windows.h>
#include <iostream>

Player::Player() {
  Reset();
  hitSpheres.push_back(Sphere(Vector3(0, 0, 0), GH_PLAYER_RADIUS));
  hitSpheres.push_back(Sphere(Vector3(0, GH_PLAYER_RADIUS - GH_PLAYER_HEIGHT, 0), GH_PLAYER_RADIUS));
}

void Player::Reset() {
  Physical::Reset();
  cam_rx = 0.0f;
  cam_ry = 0.0f;
  bob_mag = 0.0f;
  bob_phi = 0.0f;
  friction = 0.04f;
  drag = 0.002f;
  onGround = true;
}

void Player::Update() {
  //Update bobbing motion
  float magT = (prev_pos - pos).Mag() / (GH_DT * p_scale);
  if (!onGround) { magT = 0.0f; }
  bob_mag = bob_mag*(1.0f - GH_BOB_DAMP) + magT*GH_BOB_DAMP;
  if (bob_mag < GH_BOB_MIN) {
    bob_phi = 0.0f;
  } else {
    bob_phi += GH_BOB_FREQ * GH_DT;
    if (bob_phi > 2 * GH_PI) {
      bob_phi -= 2 * GH_PI;
    }
  }

  //Physics
  Physical::Update();

  //Looking
  Look(GH_INPUT->mouse_dx, GH_INPUT->mouse_dy);

  //Movement
  float moveF = 0.0f;
  float moveL = 0.0f;
  if (GH_INPUT->key['W']) {
    moveF += 1.0f;
  }
  if (GH_INPUT->key['S']) {
    moveF -= 1.0f;
  }
  if (GH_INPUT->key['A']) {
    moveL += 1.0f;
  }
  if (GH_INPUT->key['D']) {
    moveL -= 1.0f;
  }
  Move(moveF, moveL);

#if 0
  //Jumping
  if (onGround && GH_INPUT->key[VK_SPACE]) {
    velocity.y += 2.0f * p_scale;
  }
#endif

  //Reset ground state after update finishes
  onGround = false;
}

void Player::Look(float mouseDx, float mouseDy) {
  //Adjust x-axis rotation
  cam_rx -= mouseDy * GH_MOUSE_SENSITIVITY;
  if (cam_rx > GH_PI / 2) {
    cam_rx = GH_PI / 2;
  } else if (cam_rx < -GH_PI / 2) {
    cam_rx = -GH_PI / 2;
  }

  //Adjust y-axis rotation
  cam_ry -= mouseDx * GH_MOUSE_SENSITIVITY;
  if (cam_ry > GH_PI) {
    cam_ry -= GH_PI * 2;
  } else if (cam_ry < -GH_PI) {
    cam_ry += GH_PI * 2;
  }
}

void Player::Move(float moveF, float moveL) {
  //Make sure movement is not too fast
  const float mag = std::sqrt(moveF*moveF + moveL*moveL);
  if (mag > 1.0f) {
    moveF /= mag;
    moveL /= mag;
  }

  //Movement
  const Matrix4 camToWorld = LocalToWorld() * Matrix4::RotY(cam_ry);
  velocity += camToWorld.MulDirection(Vector3(-moveL, 0, -moveF)) * (GH_WALK_ACCEL * GH_DT);

  //Don't allow non-falling speeds above the player's max speed
  const float tempY = velocity.y;
  velocity.y = 0.0f;
  velocity.ClipMag(p_scale * GH_WALK_SPEED);
  velocity.y = tempY;
}

void Player::OnCollide(Object& other, const Vector3& push) {
  //Prevent player from rolling down hills if they're not too steep
  Vector3 newPush = push;
  if (push.Normalized().y > 0.7f) {
    newPush.x = 0.0f;
    newPush.z = 0.0f;
    onGround = true;
  }

  //Friction should only apply when player is on ground
  const float cur_friction = friction;
  if (!onGround) {
    friction = 0.0f;
  }

  //Base call
  Physical::OnCollide(other, newPush);
  friction = cur_friction;
}

Matrix4 Player::WorldToCam() const {
  return Matrix4::RotX(-cam_rx) * Matrix4::RotY(-cam_ry) * Matrix4::Trans(-CamOffset()) * WorldToLocal();
}

Matrix4 Player::CamToWorld() const {
  return LocalToWorld() * Matrix4::Trans(CamOffset()) * Matrix4::RotY(cam_ry) * Matrix4::RotX(cam_rx);
}

Vector3 Player::CamOffset() const {
  //If bob is too small, don't even bother
  if (bob_mag < GH_BOB_MIN) {
    return Vector3::Zero();
  }

  //Convert bob to translation
  const float theta = (GH_PI/2) * std::sin(bob_phi);
  const float y = bob_mag * GH_BOB_OFFS * (1.0f - std::cos(theta));
  return Vector3(0, y, 0);
}

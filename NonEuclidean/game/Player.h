#pragma once
#include "Vector.h"
#include "Physical.h"

class Player : public Physical {
public:
  Player();
  virtual ~Player() override {}

  virtual void Reset() override;
  virtual void Update() override;
  virtual void OnCollide(Object& other, const Vector3& push) override;

  void Look(float mouseDx, float mouseDy);
  void Move(float moveF, float moveL);

  Matrix4 WorldToCam() const;
  Matrix4 CamToWorld() const;
  Vector3 CamOffset() const;

private:
  float cam_rx;
  float cam_ry;

  float bob_mag;
  float bob_phi;

  bool onGround;
};

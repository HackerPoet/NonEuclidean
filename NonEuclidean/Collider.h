#pragma once
#include "Vector.h"
#include "Camera.h"

class Collider {
public:
  Collider(const Vector3& a, const Vector3& b, const Vector3& c);

  bool Collide(const Matrix4& localToWorld, Vector3& delta) const;

  void DebugDraw(const Camera& cam, const Matrix4& objMat);

private:
  void CreateSorted(const Vector3& da, const Vector3& c, const Vector3& db);

  Matrix4 mat;
};
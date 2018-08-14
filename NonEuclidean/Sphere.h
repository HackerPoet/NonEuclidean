#pragma once
#include "Vector.h"
#include <cassert>

class Sphere {
public:
  Sphere(float r=1.0f) : center(0.0f), radius(r) {}
  Sphere(const Vector3& pos, float r) : center(pos), radius(r) {}

  //Transformations to and frpom sphere coordinates
  Matrix4 UnitToLocal() const {
    assert(radius > 0.0f);
    return Matrix4::Trans(center) * Matrix4::Scale(radius);
  }
  Matrix4 LocalToUnit() const {
    assert(radius > 0.0f);
    return Matrix4::Scale(1.0f / radius) * Matrix4::Trans(-center);
  }

  Vector3 center;
  float radius;
};

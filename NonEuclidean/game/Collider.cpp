#include "Collider.h"
#include "GameHeader.h"
#include "GL/glew.h"
#include <cassert>
#include <iostream>

Collider::Collider(const Vector3& a, const Vector3& b, const Vector3& c) {
  const Vector3 ab = b - a;
  const Vector3 bc = c - b;
  const Vector3 ca = a - c;
  const float magAB = ab.MagSq();
  const float magBC = bc.MagSq();
  const float magCA = ca.MagSq();
  if (magAB >= magBC && magAB >= magCA) {
    CreateSorted(bc*0.5f, (a + b)*0.5f, ca*0.5f);
  } else if (magBC >= magAB && magBC >= magCA) {
    CreateSorted(ca*0.5f, (b + c)*0.5f, ab*0.5f);
  } else {
    CreateSorted(ab*0.5f, (c + a)*0.5f, bc*0.5f);
  }
}

bool Collider::Collide(const Matrix4& localToUnit, Vector3& delta) const {
  //Get world delta
  const Matrix4 local = localToUnit * mat;
  const Vector3 v = -local.Translation();

  //Get axes
  const Vector3 x = local.XAxis();
  const Vector3 y = local.YAxis();

  //Find closest point
  const float px = GH_CLAMP(v.Dot(x) / x.MagSq(), -1.0f, 1.0f);
  const float py = GH_CLAMP(v.Dot(y) / y.MagSq(), -1.0f, 1.0f);
  const Vector3 closest = x*px + y*py;

  //Calculate distance to closest point
  delta = v - closest;
  if (delta.MagSq() >= 1.0f) {
    return false;
  } else {
    delta = delta.Normalized() - delta;
    return true;
  }
}

void Collider::DebugDraw(const Camera& cam, const Matrix4& objMat) {
  glDepthFunc(GL_ALWAYS);
  glUseProgram(0);
  glBegin(GL_LINE_LOOP);
  glColor3f(0.0f, 1.0f, 0.0f);

  const Matrix4 m = cam.Matrix() * objMat * mat;
  Vector4 v;

  v = m * Vector4(1, 1, 0, 1);
  glVertex4f(v.x, v.y, v.z, v.w);
  v = m * Vector4(1, -1, 0, 1);
  glVertex4f(v.x, v.y, v.z, v.w);
  v = m * Vector4(-1, -1, 0, 1);
  glVertex4f(v.x, v.y, v.z, v.w);
  v = m * Vector4(-1, 1, 0, 1);
  glVertex4f(v.x, v.y, v.z, v.w);

  glEnd();
  glDepthFunc(GL_LESS);
}

void Collider::CreateSorted(const Vector3& da, const Vector3& c, const Vector3& db) {
  assert(std::abs(da.Dot(db)) / (da.Mag() * db.Mag()) < 0.001f);
  mat.MakeIdentity();
  mat.SetTranslation(c);
  mat.SetXAxis(da);
  mat.SetYAxis(db);
}

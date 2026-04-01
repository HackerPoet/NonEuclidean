#include "Camera.h"
#include "GameHeader.h"
#include <GL/glew.h>
#include <cmath>

Camera::Camera() :
  width(256),
  height(256) {
  worldView.MakeIdentity();
  projection.MakeIdentity();
}

void Camera::SetSize(int w, int h, float n, float f) {
  width = w;
  height = h;
  near = n;
  far = f;

  const float e = 1.0f / std::tan(GH_FOV * GH_PI / 360.0f);
  const float a = float(height) / float(width);
  const float d = near - far;

  projection.m[0] = e * a;
  projection.m[1] = 0.0f;
  projection.m[2] = 0.0f;
  projection.m[3] = 0.0f;
  projection.m[4] = 0.0f;
  projection.m[5] = e;
  projection.m[6] = 0.0f;
  projection.m[7] = 0.0f;
  projection.m[8] = 0.0f;
  projection.m[9] = 0.0f;
  projection.m[10] = (near + far) / d;
  projection.m[11] = (2 * near * far) / d;
  projection.m[12] = 0.0f;
  projection.m[13] = 0.0f;
  projection.m[14] = -1.0f;
  projection.m[15] = 0.0f;
}

void Camera::SetPositionOrientation(const Vector3& pos, float rotX, float rotY) {
  worldView = Matrix4::RotX(rotX) * Matrix4::RotY(rotY) * Matrix4::Trans(-pos);
}

Matrix4 Camera::InverseProjection() const {
  Matrix4 invProjection = Matrix4::Zero();
  const float a = projection.m[0];
  const float b = projection.m[5];
  const float c = projection.m[10];
  const float d = projection.m[11];
  const float e = projection.m[14];
  invProjection.m[0] = 1.0f / a;
  invProjection.m[5] = 1.0f / b;
  invProjection.m[11] = 1.0f / e;
  invProjection.m[14] = 1.0f / d;
  invProjection.m[15] = -c / (d * e);
  return invProjection;
}

Matrix4 Camera::Matrix() const {
  return projection * worldView;
}

void Camera::UseViewport() const {
  glViewport(0, 0, width, height);
}

void Camera::ClipOblique(const Vector3& pos, const Vector3& normal) {
  const Vector3 cpos = (worldView * Vector4(pos, 1)).XYZ();
  const Vector3 cnormal = (worldView * Vector4(normal, 0)).XYZ();
  const Vector4 cplane(cnormal.x, cnormal.y, cnormal.z, -cpos.Dot(cnormal));

  const Vector4 q = projection.Inverse() * Vector4(
    (cplane.x < 0.0f ? 1.0f : -1.0f),
    (cplane.y < 0.0f ? 1.0f : -1.0f),
    1.0f,
    1.0f
  );
  const Vector4 c = cplane * (2.0f / cplane.Dot(q));

  projection.m[8] = c.x - projection.m[12];
  projection.m[9] = c.y - projection.m[13];
  projection.m[10] = c.z - projection.m[14];
  projection.m[11] = c.w - projection.m[15];
}

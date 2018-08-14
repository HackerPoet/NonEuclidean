#pragma once
#include "Object.h"

class Tunnel : public Object {
public:
  enum Type {
    NORMAL = 0,
    SCALE = 1,
    SLOPE = 2,
  };

  Tunnel(Type type) : type(type) {
    if (type == SCALE) {
      mesh = AquireMesh("tunnel_scale.obj");
    } else if (type == SLOPE) {
      mesh = AquireMesh("tunnel_slope.obj");
    } else {
      mesh = AquireMesh("tunnel.obj");
    }
    shader = AquireShader("texture");
    texture = AquireTexture("checker_gray.bmp");
  }
  virtual ~Tunnel() {}

  void SetDoor1(Object& portal) const {
    portal.pos = LocalToWorld().MulPoint(Vector3(0, 1, 1));
    portal.euler = euler;
    portal.scale = Vector3(0.6f, 0.999f, 1) * scale.x;
  }
  void SetDoor2(Object& portal) const {
    portal.euler = euler;
    if (type == SCALE) {
      portal.pos = LocalToWorld().MulPoint(Vector3(0, 0.5f, -1));
      portal.scale = Vector3(0.3f, 0.499f, 0.5f) * scale.x;
    } else if (type == SLOPE) {
      portal.pos = LocalToWorld().MulPoint(Vector3(0, -1, -1));
      portal.scale = Vector3(0.6f, 0.999f, 1) * scale.x;
    } else {
      portal.pos = LocalToWorld().MulPoint(Vector3(0, 1, -1));
      portal.scale = Vector3(0.6f, 0.999f, 1) * scale.x;
    }
  }

private:
  Type type;
};

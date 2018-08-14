#pragma once
#include "Object.h"
#include "Resources.h"

class PillarRoom : public Object {
public:
  PillarRoom() {
    mesh = AquireMesh("pillar_room.obj");
    shader = AquireShader("texture");
    texture = AquireTexture("three_room.bmp");
    scale = Vector3(1.1f);
  }

  void SetPortal(Object& portal) const {
    portal.pos = LocalToWorld().MulPoint(Vector3(0, 1.5f, -1));
    portal.euler = euler;
    portal.euler.y -= GH_PI / 2;
    portal.scale = Vector3(1, 1.5f, 1) * scale;
  }
  virtual ~PillarRoom() {}
};
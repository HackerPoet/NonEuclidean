#pragma once
#include "Object.h"
#include "Resources.h"
#include <random>

class DeadTree : public Object {
public:
  DeadTree() {
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<float> yawDist(0.0f, GH_PI * 2.0f);

    mesh    = AquireMesh("DeadTree.obj");
    shader  = AquireShader("texture");
    texture = AquireTexture("proto_dark_01.bmp");
    euler.y = yawDist(rng);
  }
  virtual ~DeadTree() {}
};
    
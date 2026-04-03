#pragma once
#include "Resources.h"
#include "Vector.h"

class Sky {
public:

  int TimeOfDay = 3000; // 0 = midnight, 6000 = sunrise, 12000 = noon, 18000 = sunset

  Sky(const char* shaderName = "sky") {
    mesh = AquireMesh("quad.obj");
    shader = AquireShader(shaderName);
  }

  void Draw(const Camera& cam) {
    // No-op: sky is handled by the RT miss shader
  }

private:
  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Shader> shader;
};

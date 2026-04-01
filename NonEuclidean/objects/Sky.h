#pragma once
#include "Resources.h"
#include "Vector.h"

class Sky {
public:
  Sky() {
    mesh = AquireMesh("quad.obj");
    shader = AquireShader("sky");
  }

  void Draw(const Camera& cam) {
    glDepthMask(GL_FALSE);
    const Matrix4 mvp = cam.projection.Inverse();
    const Matrix4 mv = cam.worldView.Inverse();
    shader->Use();
    shader->SetMVP(mvp.m, mv.m);
    mesh->Draw();
    glDepthMask(GL_TRUE);
  }

private:
  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Shader> shader;
};

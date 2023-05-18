#pragma once
#include "Camera.h"
#include <GL/glew.h>

//Forward declaration
class Portal;

class FrameBuffer {
public:
  FrameBuffer();
  ~FrameBuffer();
  void Render(const Camera& cam, GLuint curFBO, const Portal* skipPortal);
  void Use();

private:
  GLuint texId;
  GLuint fbo;
  GLuint renderBuf;
};

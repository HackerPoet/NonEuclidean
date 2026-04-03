#pragma once
#include "Camera.h"

class Portal;

class FrameBuffer {
public:
  FrameBuffer() {}
  void Render(const Camera&, uint32_t, const Portal*) {}
  void Use() {}
};

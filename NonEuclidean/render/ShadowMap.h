#pragma once
#include <cstdint>

class ShadowMap {
public:
  ShadowMap() {}
  ~ShadowMap() {}

  bool Init(int) { return true; }
  void Destroy() {}
  void BindFaceForWriting(int) {}
  void BindForReading(int) {}
  void Unbind() {}
  int GetResolution() const { return 0; }
};

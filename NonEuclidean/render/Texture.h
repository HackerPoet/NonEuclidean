#pragma once
#include <cstdint>
#include <vector>

class Texture {
public:
  Texture(const char* fname, int rows, int cols);

  void Use() {}
  void UseOnUnit(int /*unit*/) {}

  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels; // BGR, 3 bytes per pixel
  bool is3D = false;
};

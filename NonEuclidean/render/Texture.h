#pragma once
#include <string>
#include <cstdint>
#include <vector>

class Texture {
public:
  Texture(const char* fname, int rows, int cols);
  Texture(int w, int h, const uint8_t* rgbaData); // from RGBA memory (e.g. embedded GLB textures)

  void Use() {}
  void UseOnUnit(int /*unit*/) {}

  int width = 0;
  int height = 0;
  int rows = 1;
  int cols = 1;
  std::vector<uint8_t> pixels; // BGR, 3 bytes per pixel (BMP) or RGBA, 4 bytes per pixel
  bool is3D = false;
  bool isRGBA = false; // true when pixels are RGBA 4-byte format
  std::string sourceName;
};

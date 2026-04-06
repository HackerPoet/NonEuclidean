#include "Texture.h"
#include <fstream>
#include <cassert>
#include <cstring>

Texture::Texture(const char* fname, int rows, int cols) {
  assert(rows >= 1 && cols >= 1);
  sourceName = fname ? fname : "";
  this->rows = rows;
  this->cols = cols;
  is3D = (rows > 1 || cols > 1);

  std::ifstream fin(std::string("Textures/") + fname, std::ios::in | std::ios::binary);
  if (!fin) return;

  char input[54];
  fin.read(input, 54);
  width = *reinterpret_cast<int32_t*>(&input[18]);
  height = *reinterpret_cast<int32_t*>(&input[22]);
  assert(width % cols == 0);
  assert(height % rows == 0);
  const int block_w = width / cols;
  const int block_h = height / rows;
  pixels.resize(width * height * 3);
  for (int y = height; y--> 0;) {
    const int row = y / block_h;
    const int ty = y % block_h;
    for (int x = 0; x < width; x++) {
      const int col = x / block_w;
      const int tx = x % block_w;
      uint8_t* ptr = pixels.data() + ((row*cols + col)*(block_w * block_h) + ty*block_w + tx)*3;
      fin.read(reinterpret_cast<char*>(ptr), 3);
    }
    const int padding = (width * 3) % 4;
    if (padding) {
      char junk[3];
      fin.read(junk, 4 - padding);
    }
  }
}

Texture::Texture(int w, int h, const uint8_t* rgbaData) {
  width = w;
  height = h;
  isRGBA = true;
  pixels.resize(w * h * 4);
  memcpy(pixels.data(), rgbaData, w * h * 4);
}
#pragma once
#include "Collider.h"
#include "Camera.h"
#include <cstdint>
#include <vector>
#include <map>

class Mesh {
public:
  Mesh(const char* fname);
  ~Mesh();

  void Draw() {} // no-op: rendering handled by VulkanRenderer

  void DebugDraw(const Camera& cam, const Matrix4& objMat) {}

  std::vector<Collider> colliders;

  // Vertex data (public for VulkanRenderer access)
  std::vector<float> verts;   // 3 floats per vertex (x,y,z)
  std::vector<float> uvs;     // 2 or 3 floats per vertex
  std::vector<float> normals;  // 3 floats per vertex
  bool is3DTex = false;

private:
  void AddFace(
    const std::vector<float>& vert_palette, const std::vector<float>& uv_palette,
    uint32_t a, uint32_t at, uint32_t b, uint32_t bt, uint32_t c, uint32_t ct, bool is3DTex);
};

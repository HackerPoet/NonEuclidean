#pragma once
#include "Collider.h"
#include "ColliderBVH.h"
#include "Texture.h"
#include "Camera.h"
#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <memory>

class Mesh {
public:
  Mesh();
  Mesh(const char* fname);
  Mesh(const std::vector<float>& verts, const std::vector<float>& uvs, bool is3DTex = false);
  ~Mesh();

  void Draw() {} // no-op: rendering handled by VulkanRenderer

  void DebugDraw(const Camera&, const Matrix4&) {}

  void BuildCollidersFromMesh();

  std::vector<Collider> colliders;
  ColliderBVH bvh;

  // Vertex data (public for VulkanRenderer access)
  std::vector<float> verts;   // 3 floats per vertex (x,y,z)
  std::vector<float> uvs;     // 2 or 3 floats per vertex
  std::vector<float> normals;  // 3 floats per vertex
  std::vector<float> colors;   // 3 floats per vertex (RGB, 0-1) — empty if no vertex colors
  bool is3DTex = false;
  std::string sourceName;

  // Embedded textures from GLB/FBX (extracted via Assimp)
  std::vector<std::shared_ptr<Texture>> embeddedTextures;
  std::vector<uint32_t> texIndices; // per-vertex, 0-based index into embeddedTextures

  // Per-node vertex ranges from GLB import (node name → [startVertex, endVertex))
  std::map<std::string, std::pair<size_t, size_t>> nodeVertexRanges;

  // Portal markers extracted from GLB sub-meshes named "Portal {id}"
  struct PortalMarker {
    int id;                       // numeric id parsed from node name
    std::vector<Vector3> wsVerts; // world-space vertices (triangulated)
    Vector3 center;               // world-space centroid
    Vector3 forward;              // unit face normal
  };
  std::vector<PortalMarker> portalMarkers;

  // Light markers extracted from GLB sub-meshes named "light*"
  struct LightMarker {
    std::string name;
    Vector3 center;               // world-space centroid
    Vector3 color;                // material diffuse/base color
    float radius;                 // bounding sphere radius around center
    std::shared_ptr<Mesh> mesh;   // extracted sub-mesh (not in main arrays)
  };
  std::vector<LightMarker> lightMarkers;

  // Extract a sub-mesh from a vertex range [startVert, endVert)
  std::shared_ptr<Mesh> ExtractSubMesh(size_t startVert, size_t endVert) const;

private:
  void RebuildFlatNormals();
  void AddFace(
    const std::vector<float>& vert_palette, const std::vector<float>& uv_palette,
    uint32_t a, uint32_t at, uint32_t b, uint32_t bt, uint32_t c, uint32_t ct, bool faceIs3DTex);
};

#include "ColliderBVH.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

static const uint32_t LEAF_MAX = 8; // max colliders per leaf

void ColliderBVH::Build(std::vector<Collider>& colliders) {
  nodes.clear();
  if (colliders.empty()) return;
  nodes.reserve(colliders.size() * 2); // rough upper bound
  BuildRecursive(colliders, 0, (uint32_t)colliders.size());
}

uint32_t ColliderBVH::BuildRecursive(std::vector<Collider>& colliders,
                                     uint32_t begin, uint32_t end) {
  uint32_t idx = (uint32_t)nodes.size();
  nodes.push_back(Node());
  Node& node = nodes[idx];

  // Compute combined AABB
  Vector3 bmin(1e30f, 1e30f, 1e30f);
  Vector3 bmax(-1e30f, -1e30f, -1e30f);
  for (uint32_t i = begin; i < end; ++i) {
    Vector3 cmin, cmax;
    colliders[i].GetAABB(cmin, cmax);
    bmin.x = std::min(bmin.x, cmin.x);
    bmin.y = std::min(bmin.y, cmin.y);
    bmin.z = std::min(bmin.z, cmin.z);
    bmax.x = std::max(bmax.x, cmax.x);
    bmax.y = std::max(bmax.y, cmax.y);
    bmax.z = std::max(bmax.z, cmax.z);
  }
  node.aabbMin = bmin;
  node.aabbMax = bmax;

  uint32_t count = end - begin;
  if (count <= LEAF_MAX) {
    // Leaf
    node.left = ~0u;
    node.right = ~0u;
    node.start = begin;
    node.count = count;
    return idx;
  }

  // Pick longest axis to split
  Vector3 extent = bmax - bmin;
  int axis = 0;
  if (extent.y > extent.x) axis = 1;
  if ((axis == 0 ? extent.z > extent.x : extent.z > extent.y)) axis = 2;

  // Sort colliders along chosen axis by centroid
  std::sort(&colliders[begin], &colliders[end],
    [axis](const Collider& a, const Collider& b) {
      Vector3 amin, amax, bmin2, bmax2;
      a.GetAABB(amin, amax);
      b.GetAABB(bmin2, bmax2);
      float ca = (axis == 0) ? (amin.x + amax.x) :
                 (axis == 1) ? (amin.y + amax.y) :
                               (amin.z + amax.z);
      float cb = (axis == 0) ? (bmin2.x + bmax2.x) :
                 (axis == 1) ? (bmin2.y + bmax2.y) :
                               (bmin2.z + bmax2.z);
      return ca < cb;
    });

  uint32_t mid = begin + count / 2;

  // Build children — note: nodes may reallocate, so don't hold references
  uint32_t leftChild = BuildRecursive(colliders, begin, mid);
  uint32_t rightChild = BuildRecursive(colliders, mid, end);

  nodes[idx].left = leftChild;
  nodes[idx].right = rightChild;
  nodes[idx].start = 0;
  nodes[idx].count = 0;
  return idx;
}

bool ColliderBVH::AABBOverlapsUnitSphere(const Vector3& bmin, const Vector3& bmax,
                                         const Matrix4& localToUnit) {
  // Transform the 8 AABB corners into unit-sphere space, compute
  // a new AABB in that space, then check overlap with the unit sphere.
  const float* m = localToUnit.m;

  // Use Arvo's method: transform AABB center + half-extent through the matrix,
  // compute the resulting AABB in unit-sphere space, then check overlap.
  Vector3 center = (bmin + bmax) * 0.5f;
  Vector3 half = (bmax - bmin) * 0.5f;

  // Center in unit space
  float cx = m[0]*center.x + m[1]*center.y + m[2]*center.z + m[3];
  float cy = m[4]*center.x + m[5]*center.y + m[6]*center.z + m[7];
  float cz = m[8]*center.x + m[9]*center.y + m[10]*center.z + m[11];

  // Half-extent in unit space (absolute values of matrix rows dotted with half)
  float ex = std::abs(m[0])*half.x + std::abs(m[1])*half.y + std::abs(m[2])*half.z;
  float ey = std::abs(m[4])*half.x + std::abs(m[5])*half.y + std::abs(m[6])*half.z;
  float ez = std::abs(m[8])*half.x + std::abs(m[9])*half.y + std::abs(m[10])*half.z;

  // Closest point on AABB-in-unit-space to origin
  float dx = std::max(0.0f, std::abs(cx) - ex);
  float dy = std::max(0.0f, std::abs(cy) - ey);
  float dz = std::max(0.0f, std::abs(cz) - ez);

  return (dx*dx + dy*dy + dz*dz) < 1.0f;
}

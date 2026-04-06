#pragma once
#include "Collider.h"
#include "Vector.h"
#include <vector>
#include <cstdint>

// Axis-aligned bounding volume hierarchy over a Collider array.
// Build once, then query with CollideBVH() to avoid O(n) per-frame tests.
class ColliderBVH {
public:
  ColliderBVH() {}

  // Build from a pre-existing collider vector.  Reorders `colliders` in place.
  void Build(std::vector<Collider>& colliders);

  // Test all colliders whose AABBs can possibly overlap the unit sphere
  // after being transformed by `localToUnit`.  Calls `cb(collider, push)`
  // for every positive hit.  Returns true if at least one hit occurred.
  template<typename Callback>
  bool Query(const Matrix4& localToUnit, const std::vector<Collider>& colliders,
             Callback cb) const;

  bool Empty() const { return nodes.empty(); }

private:
  struct Node {
    Vector3 aabbMin, aabbMax;
    uint32_t left;   // child index or ~0u for leaf
    uint32_t right;  // child index or ~0u for leaf
    uint32_t start;  // first collider index (leaf only)
    uint32_t count;  // number of colliders  (leaf only)
  };
  std::vector<Node> nodes;

  uint32_t BuildRecursive(std::vector<Collider>& colliders,
                          uint32_t begin, uint32_t end);

  // Test whether an AABB (in mesh-local space), once transformed by
  // localToUnit, could overlap the unit sphere at the origin.
  static bool AABBOverlapsUnitSphere(const Vector3& bmin, const Vector3& bmax,
                                     const Matrix4& localToUnit);
};

// ── inline template implementation ──────────────────────────────

template<typename Callback>
bool ColliderBVH::Query(const Matrix4& localToUnit,
                        const std::vector<Collider>& colliders,
                        Callback cb) const {
  if (nodes.empty()) return false;

  bool anyHit = false;
  // Simple iterative stack (max depth ~64 is more than enough)
  uint32_t stack[64];
  int sp = 0;
  stack[sp++] = 0; // root

  while (sp > 0) {
    const Node& n = nodes[stack[--sp]];
    if (!AABBOverlapsUnitSphere(n.aabbMin, n.aabbMax, localToUnit))
      continue;

    if (n.left == ~0u) {
      // Leaf — test individual colliders
      for (uint32_t i = n.start; i < n.start + n.count; ++i) {
        Vector3 push;
        if (colliders[i].Collide(localToUnit, push)) {
          cb(colliders[i], push);
          anyHit = true;
        }
      }
    } else {
      stack[sp++] = n.left;
      stack[sp++] = n.right;
    }
  }
  return anyHit;
}

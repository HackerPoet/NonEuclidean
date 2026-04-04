#pragma once
#include "DeadTree.h"
#include "world.h"
#include <cmath>
#include <string>

struct PortalFrameLayoutSettings {
  float portalDistance;
  float portalHalfWidth;
  float portalHalfHeight;
  float treeScale;
};

inline PortalFrameLayoutSettings GetLevel2PortalFrameLayoutSettings(bool wideLayout) {
  PortalFrameLayoutSettings settings = {};
  settings.portalDistance = wideLayout ? 16.0f : 12.0f;
  settings.portalHalfWidth = wideLayout ? 6.3f : 2.0f;
  settings.portalHalfHeight = wideLayout ? 8.5f : 6.6f;
  settings.treeScale = wideLayout ? 3.6f : 3.2f;
  return settings;
}

inline void AddPortalFrameTreesNamed(
  World& world,
  const std::string& baseName,
  const Vector3& portalPos,
  float portalYaw,
  float portalHalfWidth,
  float treeScale,
  bool rotateFirstTreeLeft = false) {
  const Vector3 right(std::cos(portalYaw), 0.0f, std::sin(portalYaw));
  const Vector3 forward(-std::sin(portalYaw), 0.0f, -std::cos(portalYaw));
  const float directions[2] = {-1.0f, 1.0f};
  const float treeOffset = portalHalfWidth + 2.1f;
  const float leftYawOffset = 10.0f * GH_PI / 180.0f;
  const float rightYawOffset = -160.0f * GH_PI / 180.0f;

  (void)rotateFirstTreeLeft;

  for (int treeIndex = 0; treeIndex < 2; ++treeIndex) {
    float direction = directions[treeIndex];
    std::shared_ptr<DeadTree> tree(new DeadTree());
    if (!baseName.empty()) {
      tree->name = baseName + (treeIndex == 0 ? " frame left" : " frame right");
    }
    tree->scale = Vector3(treeScale, treeScale, treeScale);
    tree->pos = portalPos + right * (treeOffset * direction) - forward * 0.35f;
    tree->pos.y = 0.0f;
    tree->euler.y = portalYaw + (treeIndex == 0 ? leftYawOffset : rightYawOffset);
    world.objects.push_back(tree);
  }
}

inline void AddPortalFrameTrees(
  World& world,
  const Vector3& portalPos,
  float portalYaw,
  float portalHalfWidth,
  float treeScale,
  bool rotateFirstTreeLeft = false) {
  AddPortalFrameTreesNamed(world, std::string(), portalPos, portalYaw, portalHalfWidth, treeScale, rotateFirstTreeLeft);
}
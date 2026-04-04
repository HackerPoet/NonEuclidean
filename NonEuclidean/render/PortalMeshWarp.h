#pragma once
#include "Mesh.h"
#include "Vector.h"
#include <memory>

enum class PortalWarpSide {
  Positive,
  Negative,
};

struct PortalMeshSplitResult {
  std::shared_ptr<Mesh> localMesh;
  std::shared_ptr<Mesh> warpedMesh;
};

PortalMeshSplitResult BuildPortalWarpMeshSplit(
  const Mesh& sourceMesh,
  const Matrix4& localToWorld,
  const Vector3& planePoint,
  const Vector3& planeNormal,
  const Matrix4& warpMatrix,
  PortalWarpSide sideToWarp);
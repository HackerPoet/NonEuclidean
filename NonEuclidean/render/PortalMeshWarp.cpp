#include "PortalMeshWarp.h"
#include <array>
#include <vector>

namespace {

struct ClipVertex {
  Vector3 pos;
  Vector3 uv;
};

float SignedDistanceToPlane(const Vector3& point, const Vector3& planePoint, const Vector3& planeNormal) {
  return (point - planePoint).Dot(planeNormal);
}

ClipVertex LerpVertex(const ClipVertex& a, const ClipVertex& b, float t) {
  ClipVertex result;
  result.pos = a.pos + (b.pos - a.pos) * t;
  result.uv = a.uv + (b.uv - a.uv) * t;
  return result;
}

bool IsInsideHalfSpace(float signedDistance, bool keepPositive) {
  return keepPositive ? signedDistance >= 0.0f : signedDistance <= 0.0f;
}

std::vector<ClipVertex> ClipPolygonAgainstPlane(
  const std::vector<ClipVertex>& polygon,
  const Vector3& planePoint,
  const Vector3& planeNormal,
  bool keepPositive) {
  std::vector<ClipVertex> output;
  if (polygon.empty()) {
    return output;
  }

  ClipVertex previous = polygon.back();
  float previousDistance = SignedDistanceToPlane(previous.pos, planePoint, planeNormal);
  bool previousInside = IsInsideHalfSpace(previousDistance, keepPositive);

  for (const ClipVertex& current : polygon) {
    float currentDistance = SignedDistanceToPlane(current.pos, planePoint, planeNormal);
    bool currentInside = IsInsideHalfSpace(currentDistance, keepPositive);

    if (currentInside != previousInside) {
      float denom = previousDistance - currentDistance;
      float t = denom == 0.0f ? 0.0f : previousDistance / denom;
      output.push_back(LerpVertex(previous, current, t));
    }

    if (currentInside) {
      output.push_back(current);
    }

    previous = current;
    previousDistance = currentDistance;
    previousInside = currentInside;
  }

  return output;
}

void AppendVertex(
  const ClipVertex& vertex,
  std::vector<float>& outVerts,
  std::vector<float>& outUVs,
  int uvComponents,
  const Matrix4* transform) {
  Vector3 pos = transform ? transform->MulPoint(vertex.pos) : vertex.pos;
  outVerts.push_back(pos.x);
  outVerts.push_back(pos.y);
  outVerts.push_back(pos.z);

  outUVs.push_back(vertex.uv.x);
  outUVs.push_back(vertex.uv.y);
  if (uvComponents == 3) {
    outUVs.push_back(vertex.uv.z);
  }
}

void AppendTriangleFan(
  const std::vector<ClipVertex>& polygon,
  std::vector<float>& outVerts,
  std::vector<float>& outUVs,
  int uvComponents,
  const Matrix4* transform) {
  if (polygon.size() < 3) {
    return;
  }

  for (size_t i = 1; i + 1 < polygon.size(); ++i) {
    Vector3 triA = polygon[0].pos;
    Vector3 triB = polygon[i].pos;
    Vector3 triC = polygon[i + 1].pos;
    if ((triB - triA).Cross(triC - triA).MagSq() <= 1e-8f) {
      continue;
    }

    AppendVertex(polygon[0], outVerts, outUVs, uvComponents, transform);
    AppendVertex(polygon[i], outVerts, outUVs, uvComponents, transform);
    AppendVertex(polygon[i + 1], outVerts, outUVs, uvComponents, transform);
  }
}

}

PortalMeshSplitResult BuildPortalWarpMeshSplit(
  const Mesh& sourceMesh,
  const Matrix4& localToWorld,
  const Vector3& planePoint,
  const Vector3& planeNormal,
  const Matrix4& warpMatrix,
  PortalWarpSide sideToWarp) {
  PortalMeshSplitResult result;
  const int uvComponents = sourceMesh.is3DTex ? 3 : 2;
  std::vector<float> localVerts;
  std::vector<float> localUVs;
  std::vector<float> warpedVerts;
  std::vector<float> warpedUVs;

  const bool warpPositiveSide = sideToWarp == PortalWarpSide::Positive;
  const size_t triangleCount = sourceMesh.verts.size() / 9;
  for (size_t triangle = 0; triangle < triangleCount; ++triangle) {
    std::vector<ClipVertex> polygon;
    polygon.reserve(3);
    for (int vertex = 0; vertex < 3; ++vertex) {
      const size_t vertOffset = triangle * 9 + vertex * 3;
      const size_t uvOffset = triangle * 3 * uvComponents + vertex * uvComponents;

      ClipVertex clipVertex;
      clipVertex.pos = localToWorld.MulPoint(Vector3(&sourceMesh.verts[vertOffset]));
      clipVertex.uv = Vector3::Zero();
      if (!sourceMesh.uvs.empty()) {
        clipVertex.uv.x = sourceMesh.uvs[uvOffset];
        clipVertex.uv.y = sourceMesh.uvs[uvOffset + 1];
        if (uvComponents == 3) {
          clipVertex.uv.z = sourceMesh.uvs[uvOffset + 2];
        }
      }
      polygon.push_back(clipVertex);
    }

    std::vector<ClipVertex> positiveSide = ClipPolygonAgainstPlane(polygon, planePoint, planeNormal, true);
    std::vector<ClipVertex> negativeSide = ClipPolygonAgainstPlane(polygon, planePoint, planeNormal, false);

    const std::vector<ClipVertex>& localSide = warpPositiveSide ? negativeSide : positiveSide;
    const std::vector<ClipVertex>& warpedSide = warpPositiveSide ? positiveSide : negativeSide;

    AppendTriangleFan(localSide, localVerts, localUVs, uvComponents, nullptr);
    AppendTriangleFan(warpedSide, warpedVerts, warpedUVs, uvComponents, &warpMatrix);
  }

  if (!localVerts.empty()) {
    result.localMesh = std::make_shared<Mesh>(localVerts, localUVs, sourceMesh.is3DTex);
  }
  if (!warpedVerts.empty()) {
    result.warpedMesh = std::make_shared<Mesh>(warpedVerts, warpedUVs, sourceMesh.is3DTex);
  }

  return result;
}
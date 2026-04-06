#pragma once
#include "Mesh.h"
#include "Vector.h"
#include <memory>
#include <vector>

// Generates a maze using recursive backtracking starting from the center cell.
// The maze is a grid of cells; walls between cells become 3D wall geometry.
// Portal slots are placed ON internal walls (replacing the wall geometry).
struct MazeGenerator {
  struct PortalSlot {
    Vector3 pos;    // world-space center of the wall face
    float yaw;      // euler.y (portal normal direction)
    float width;    // half-width  (for portal scale.x)
    float height;   // half-height (for portal scale.y)
  };

  struct Result {
    std::shared_ptr<Mesh> mesh;
    Vector3 exitPos;                        // world-space position above the exit
    float exitYaw;                          // facing direction of exit
    Vector3 doorPos;                        // center of the exit door quad
    float doorYaw;                          // door facing direction
    float doorWidth;                        // door half-width
    float doorHeight;                       // door half-height
    std::vector<PortalSlot> portalSlots;    // even-count; pair [0,1], [2,3], etc.
  };

  // cols/rows = number of cells in X/Z. wallHeight/wallThick in world units.
  // cellSize = distance between cell centers.
  // The maze is centered at the origin on the XZ plane with Y=0 as floor.
  static Result Generate(int cols, int rows,
                         float cellSize = 4.0f,
                         float wallHeight = 3.0f,
                         float wallThick = 0.3f);
};

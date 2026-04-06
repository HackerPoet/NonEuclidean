#include "MazeGenerator.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <queue>
#include <set>

// Each cell stores which walls are present (N/S/E/W) and whether it's visited.
namespace {

enum Wall { NORTH = 1, SOUTH = 2, EAST = 4, WEST = 8 };

struct Cell {
  int walls;   // bitmask of present walls
  bool visited;
  int dist;    // BFS distance from center (-1 = unset)
};

struct IVec2 { int x, y; };

// Adds a quad (two triangles) to the vert/uv arrays.
void AddQuad(std::vector<float>& verts, std::vector<float>& uvs,
             float x0, float y0, float z0,
             float x1, float y1, float z1,
             float x2, float y2, float z2,
             float x3, float y3, float z3) {
  verts.push_back(x0); verts.push_back(y0); verts.push_back(z0);
  verts.push_back(x1); verts.push_back(y1); verts.push_back(z1);
  verts.push_back(x2); verts.push_back(y2); verts.push_back(z2);
  verts.push_back(x0); verts.push_back(y0); verts.push_back(z0);
  verts.push_back(x2); verts.push_back(y2); verts.push_back(z2);
  verts.push_back(x3); verts.push_back(y3); verts.push_back(z3);
  uvs.push_back(0.0f); uvs.push_back(0.0f);
  uvs.push_back(1.0f); uvs.push_back(0.0f);
  uvs.push_back(1.0f); uvs.push_back(1.0f);
  uvs.push_back(0.0f); uvs.push_back(0.0f);
  uvs.push_back(1.0f); uvs.push_back(1.0f);
  uvs.push_back(0.0f); uvs.push_back(1.0f);
}

// Adds a wall segment as a box (4 side faces).
void AddWallBox(std::vector<float>& verts, std::vector<float>& uvs,
                float cx, float cz, float hw, float hd, float h) {
  float x0 = cx - hw, x1 = cx + hw;
  float z0 = cz - hd, z1 = cz + hd;
  AddQuad(verts, uvs, x0,0,z1, x1,0,z1, x1,h,z1, x0,h,z1);
  AddQuad(verts, uvs, x1,0,z0, x0,0,z0, x0,h,z0, x1,h,z0);
  AddQuad(verts, uvs, x1,0,z1, x1,0,z0, x1,h,z0, x1,h,z1);
  AddQuad(verts, uvs, x0,0,z0, x0,0,z1, x0,h,z1, x0,h,z0);
}

bool isBoundary(int x, int y, int cols, int rows) {
  return x == 0 || y == 0 || x == cols - 1 || y == rows - 1;
}

} // namespace

MazeGenerator::Result MazeGenerator::Generate(int cols, int rows,
                                              float cellSize,
                                              float wallHeight,
                                              float wallThick) {
  Result result;
  const float PI = 3.14159265f;

  // --- 1. Build the maze grid via recursive-backtracking from center ---
  std::vector<Cell> grid((size_t)(cols * rows));
  for (int i = 0; i < cols * rows; ++i) {
    grid[i].walls = NORTH | SOUTH | EAST | WEST;
    grid[i].visited = false;
    grid[i].dist = -1;
  }

  std::srand((unsigned)std::time(nullptr));

  int startX = cols / 2;
  int startY = rows / 2;
  std::vector<IVec2> stack;
  stack.push_back({startX, startY});
  grid[startY * cols + startX].visited = true;

  while (!stack.empty()) {
    IVec2 cur = stack.back();
    int dirs[4];
    int nDirs = 0;
    if (cur.y > 0 && !grid[(cur.y - 1) * cols + cur.x].visited)
      dirs[nDirs++] = 0;
    if (cur.y < rows - 1 && !grid[(cur.y + 1) * cols + cur.x].visited)
      dirs[nDirs++] = 1;
    if (cur.x < cols - 1 && !grid[cur.y * cols + cur.x + 1].visited)
      dirs[nDirs++] = 2;
    if (cur.x > 0 && !grid[cur.y * cols + cur.x - 1].visited)
      dirs[nDirs++] = 3;

    if (nDirs == 0) {
      stack.pop_back();
      continue;
    }

    int chosen = dirs[std::rand() % nDirs];
    int nx = cur.x, ny = cur.y;
    int removeWallCur = 0, removeWallNeighbor = 0;
    switch (chosen) {
      case 0: ny--; removeWallCur = NORTH; removeWallNeighbor = SOUTH; break;
      case 1: ny++; removeWallCur = SOUTH; removeWallNeighbor = NORTH; break;
      case 2: nx++; removeWallCur = EAST;  removeWallNeighbor = WEST;  break;
      case 3: nx--; removeWallCur = WEST;  removeWallNeighbor = EAST;  break;
    }

    grid[cur.y * cols + cur.x].walls &= ~removeWallCur;
    grid[ny * cols + nx].walls &= ~removeWallNeighbor;
    grid[ny * cols + nx].visited = true;
    stack.push_back({nx, ny});
  }

  // --- 2. BFS from center to compute distances ---
  float ox = -(cols * cellSize) * 0.5f;
  float oz = -(rows * cellSize) * 0.5f;

  {
    std::queue<IVec2> q;
    q.push({startX, startY});
    grid[startY * cols + startX].dist = 0;
    while (!q.empty()) {
      IVec2 c = q.front(); q.pop();
      Cell& cell = grid[c.y * cols + c.x];
      int d = cell.dist + 1;
      if (!(cell.walls & NORTH) && c.y > 0 && grid[(c.y-1)*cols+c.x].dist < 0) {
        grid[(c.y-1)*cols+c.x].dist = d; q.push({c.x, c.y-1});
      }
      if (!(cell.walls & SOUTH) && c.y < rows-1 && grid[(c.y+1)*cols+c.x].dist < 0) {
        grid[(c.y+1)*cols+c.x].dist = d; q.push({c.x, c.y+1});
      }
      if (!(cell.walls & EAST) && c.x < cols-1 && grid[c.y*cols+c.x+1].dist < 0) {
        grid[c.y*cols+c.x+1].dist = d; q.push({c.x+1, c.y});
      }
      if (!(cell.walls & WEST) && c.x > 0 && grid[c.y*cols+c.x-1].dist < 0) {
        grid[c.y*cols+c.x-1].dist = d; q.push({c.x-1, c.y});
      }
    }
  }

  // --- 3. Find exit: farthest boundary cell, open its outer wall ---
  int exitX = 0, exitY = 0, exitDist = -1;
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      if (isBoundary(c, r, cols, rows) && grid[r * cols + c].dist > exitDist) {
        exitDist = grid[r * cols + c].dist;
        exitX = c;
        exitY = r;
      }
    }
  }

  float exitCX = ox + exitX * cellSize + cellSize * 0.5f;
  float exitCZ = oz + exitY * cellSize + cellSize * 0.5f;
  float exitYaw = 0.0f;
  float exitPosX = exitCX, exitPosZ = exitCZ;
  float doorPosX = exitCX, doorPosZ = exitCZ;

  if (exitY == 0) {
    grid[exitY * cols + exitX].walls &= ~NORTH;
    exitPosZ = oz + exitY * cellSize;
    doorPosZ = exitPosZ;
    exitYaw = 0.0f;
  } else if (exitY == rows - 1) {
    grid[exitY * cols + exitX].walls &= ~SOUTH;
    exitPosZ = oz + (exitY + 1) * cellSize;
    doorPosZ = exitPosZ;
    exitYaw = PI;
  } else if (exitX == 0) {
    grid[exitY * cols + exitX].walls &= ~WEST;
    exitPosX = ox + exitX * cellSize;
    doorPosX = exitPosX;
    exitYaw = PI * 0.5f;
  } else {
    grid[exitY * cols + exitX].walls &= ~EAST;
    exitPosX = ox + (exitX + 1) * cellSize;
    doorPosX = exitPosX;
    exitYaw = -PI * 0.5f;
  }

  result.exitPos = Vector3(exitPosX, wallHeight, exitPosZ);
  result.exitYaw = exitYaw;
  result.doorPos = Vector3(doorPosX, wallHeight * 0.5f, doorPosZ);
  result.doorYaw = exitYaw;
  result.doorWidth = cellSize * 0.5f;
  result.doorHeight = wallHeight * 0.5f;

  // --- 4. Collect internal walls as portal candidates ---
  // Internal walls sit between two maze cells (not boundary walls).
  // Portal replaces the wall geometry; the double_quad portal mesh fills the gap.
  struct WallId { int cellIdx; int flag; };
  std::vector<WallId> candidates;

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const Cell& cell = grid[r * cols + c];
      if (r > 0 && (cell.walls & NORTH))
        candidates.push_back({r * cols + c, NORTH});
      if (c > 0 && (cell.walls & WEST))
        candidates.push_back({r * cols + c, WEST});
    }
  }

  // Shuffle
  for (size_t i = candidates.size(); i > 1; --i) {
    size_t j = (size_t)(std::rand() % (int)i);
    std::swap(candidates[i - 1], candidates[j]);
  }

  // Select pairs and mark portal walls so their geometry is skipped
  std::set<long long> portalWalls;
  size_t pairCount = candidates.size() / 2;
  if (pairCount > 20) pairCount = 20;

  for (size_t i = 0; i < pairCount * 2; ++i) {
    const WallId& w = candidates[i];
    portalWalls.insert((long long)w.cellIdx * 16 + w.flag);

    int c = w.cellIdx % cols;
    int r = w.cellIdx / cols;
    float cx = ox + c * cellSize;
    float cz = oz + r * cellSize;

    PortalSlot slot;
    if (w.flag == NORTH) {
      slot.pos = Vector3(cx + cellSize * 0.5f, wallHeight * 0.5f, cz);
      slot.yaw = 0.0f;
    } else {
      slot.pos = Vector3(cx, wallHeight * 0.5f, cz + cellSize * 0.5f);
      slot.yaw = PI * 0.5f;
    }
    slot.width = cellSize * 0.5f;
    slot.height = wallHeight * 0.5f;
    result.portalSlots.push_back(slot);
  }

  // --- 5. Convert grid walls to 3D wall geometry (skip portal walls) ---
  std::vector<float> verts, uvs;
  float halfThick = wallThick * 0.5f;

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const Cell& cell = grid[r * cols + c];
      float cx = ox + c * cellSize;
      float cz = oz + r * cellSize;

      if (cell.walls & NORTH) {
        long long key = (long long)(r * cols + c) * 16 + NORTH;
        if (portalWalls.find(key) == portalWalls.end()) {
          float wcx = cx + cellSize * 0.5f;
          AddWallBox(verts, uvs, wcx, cz, cellSize * 0.5f + halfThick, halfThick, wallHeight);
        }
      }
      if (cell.walls & WEST) {
        long long key = (long long)(r * cols + c) * 16 + WEST;
        if (portalWalls.find(key) == portalWalls.end()) {
          float wcz = cz + cellSize * 0.5f;
          AddWallBox(verts, uvs, cx, wcz, halfThick, cellSize * 0.5f + halfThick, wallHeight);
        }
      }
    }
  }

  // Outer boundary: south edge (skip exit column if exit is on south)
  for (int c = 0; c < cols; ++c) {
    if (exitY == rows - 1 && c == exitX) continue;
    float cx = ox + c * cellSize;
    float cz = oz + rows * cellSize;
    float wcx = cx + cellSize * 0.5f;
    AddWallBox(verts, uvs, wcx, cz, cellSize * 0.5f + halfThick, halfThick, wallHeight);
  }
  // Outer boundary: east edge (skip exit row if exit is on east)
  for (int r = 0; r < rows; ++r) {
    if (exitX == cols - 1 && r == exitY) continue;
    float cx = ox + cols * cellSize;
    float cz = oz + r * cellSize;
    float wcz = cz + cellSize * 0.5f;
    AddWallBox(verts, uvs, cx, wcz, halfThick, cellSize * 0.5f + halfThick, wallHeight);
  }

  // --- 6. Build mesh with colliders ---
  result.mesh = std::make_shared<Mesh>(verts, uvs);
  result.mesh->BuildCollidersFromMesh();
  result.mesh->bvh.Build(result.mesh->colliders);
  return result;
}

#include "Level4.h"
#include "Object.h"
#include "Portal.h"
#include "Resources.h"
#include "MazeGenerator.h"
#include "DeadTree.h"
#include "Sky.h"
#include <cstdlib>

void Level4::Load(World& world, Player& player) {
  world.ambientScale = 0.5f;
  world.sky = std::make_shared<Sky>("sky");
  world.time_of_day = 0.0f;

  // Ground
  std::shared_ptr<Object> ground(new Object());
  ground->mesh = AquireMesh("ground_large.obj");
  ground->shader = AquireShader("texture");
  ground->texture = AquireTexture("proto_dark_01.bmp");
  ground->scale = Vector3(500.0f, 1.0f, 500.0f);
  world.objects.push_back(ground);

  // Generate maze (15x15 cells, centered at origin)
  const int mazeCols = 15;
  const int mazeRows = 15;
  const float cellSize = 4.0f;
  const float wallHeight = 3.0f;

  MazeGenerator::Result mazeResult =
      MazeGenerator::Generate(mazeCols, mazeRows, cellSize, wallHeight);

  std::shared_ptr<Object> maze(new Object());
  maze->mesh = mazeResult.mesh;
  maze->shader = AquireShader("texture");
  maze->texture = AquireTexture("proto_orange_02.bmp");
  world.objects.push_back(maze);

  // Red light at the exit
  {
    Vector3 lpos = mazeResult.exitPos;
    Vector3 lcol(6.0f, 0.5f, 0.3f);

    std::shared_ptr<Object> cube(new Object());
    cube->mesh = AquireMesh("cube.obj");
    cube->shader = AquireShader("texture");
    cube->texture = AquireTexture("white.bmp");
    cube->pos = lpos;
    cube->scale = Vector3(0.3f, 0.3f, 0.3f);
    cube->castShadow = false;
    cube->emissive = lcol;
    world.objects.push_back(cube);
    world.lights.push_back({lpos, lcol, 0.0f});
  }

  // Light above the center
  {
    Vector3 lpos(0.0f, wallHeight + 2.0f, 0.0f);
    Vector3 lcol(5.0f, 4.8f, 4.0f);

    std::shared_ptr<Object> cube(new Object());
    cube->mesh = AquireMesh("cube.obj");
    cube->shader = AquireShader("texture");
    cube->texture = AquireTexture("white.bmp");
    cube->pos = lpos;
    cube->scale = Vector3(0.3f, 0.3f, 0.3f);
    cube->castShadow = false;
    cube->emissive = lcol;
    world.objects.push_back(cube);
    world.lights.push_back({lpos, lcol, 0.0f});
  }

  // Lights at the four corners of the maze
  float halfExtent = (mazeCols * cellSize) * 0.5f;
  float cornerOffsets[4][2] = {
    {-halfExtent, -halfExtent},
    { halfExtent, -halfExtent},
    {-halfExtent,  halfExtent},
    { halfExtent,  halfExtent},
  };
  for (int i = 0; i < 4; ++i) {
    Vector3 lpos(cornerOffsets[i][0], wallHeight + 1.0f, cornerOffsets[i][1]);
    Vector3 lcol(3.0f, 2.8f, 2.5f);

    std::shared_ptr<Object> cube(new Object());
    cube->mesh = AquireMesh("cube.obj");
    cube->shader = AquireShader("texture");
    cube->texture = AquireTexture("white.bmp");
    cube->pos = lpos;
    cube->scale = Vector3(0.2f, 0.2f, 0.2f);
    cube->castShadow = false;
    cube->emissive = lcol;
    world.objects.push_back(cube);
    world.lights.push_back({lpos, lcol, 0.0f});
  }

  // Create portals from maze slots (paired: [0,1], [2,3], ...)
  // Portals are placed ON walls — the wall geometry is omitted and the
  // double_quad portal mesh replaces it visually, making portals invisible.
  for (size_t i = 0; i + 1 < mazeResult.portalSlots.size(); i += 2) {
    const MazeGenerator::PortalSlot& a = mazeResult.portalSlots[i];
    const MazeGenerator::PortalSlot& b = mazeResult.portalSlots[i + 1];

    std::shared_ptr<Portal> p1(new Portal());
    p1->pos = a.pos;
    p1->euler.y = a.yaw;
    p1->scale = Vector3(a.width, a.height, 1.0f);

    std::shared_ptr<Portal> p2(new Portal());
    p2->pos = b.pos;
    p2->euler.y = b.yaw;
    p2->scale = Vector3(b.width, b.height, 1.0f);

    Portal::Connect(p1, p2);
    world.portals.push_back(p1);
    world.portals.push_back(p2);
  }

  // Exit door (flat quad marking the exit opening)
  {
    std::shared_ptr<Object> door(new Object());
    door->mesh = AquireMesh("double_quad.obj");
    door->shader = AquireShader("texture");
    door->texture = AquireTexture("proto_dark_01.bmp");
    door->pos = mazeResult.doorPos;
    door->euler.y = mazeResult.doorYaw;
    door->scale = Vector3(mazeResult.doorWidth, mazeResult.doorHeight, 1.0f);
    world.objects.push_back(door);
  }

  // Entrance portal: outside the exit → center of the maze
  {
    // Place the outside end a short distance beyond the exit door
    float entranceOffset = cellSize * 1.5f;
    float sinYaw = std::sin(mazeResult.doorYaw);
    float cosYaw = std::cos(mazeResult.doorYaw);
    Vector3 outsidePos = mazeResult.doorPos +
        Vector3(-sinYaw * entranceOffset, 0.0f, cosYaw * entranceOffset);

    std::shared_ptr<Portal> entranceOut(new Portal());
    entranceOut->pos = outsidePos;
    entranceOut->euler.y = mazeResult.doorYaw + GH_PI;
    entranceOut->scale = Vector3(mazeResult.doorWidth, mazeResult.doorHeight, 1.0f);

    std::shared_ptr<Portal> entranceIn(new Portal());
    entranceIn->pos = Vector3(0.0f, wallHeight * 0.5f, 0.0f);
    entranceIn->euler.y = 0.0f;
    entranceIn->scale = Vector3(mazeResult.doorWidth, mazeResult.doorHeight, 1.0f);

    Portal::Connect(entranceOut, entranceIn);
    world.portals.push_back(entranceOut);
    world.portals.push_back(entranceIn);
  }

  // Scatter trees outside the maze walls
  {
    const float margin = 3.0f;    // gap between maze edge and first tree
    const float spread = 40.0f;   // how far out trees extend
    const int treeCount = 60;
    float inner = halfExtent + margin;
    float outer = halfExtent + spread;
    std::srand((unsigned)std::time(nullptr));
    for (int i = 0; i < treeCount; ++i) {
      float angle = (float)(std::rand() % 3600) / 3600.0f * GH_PI * 2.0f;
      float radius = inner + (float)(std::rand() % 1000) / 1000.0f * (outer - inner);
      float tx = std::cos(angle) * radius;
      float tz = std::sin(angle) * radius;
      float s = 1.5f + (float)(std::rand() % 1000) / 1000.0f * 2.5f; // scale 1.5-4.0
      std::shared_ptr<DeadTree> tree(new DeadTree());
      tree->pos = Vector3(tx, 0.0f, tz);
      tree->scale = Vector3(s, s, s);
      world.objects.push_back(tree);
    }
  }

  // Player starts at the center cell of the maze
  player.SetPosition(Vector3(0.0f, GH_PLAYER_HEIGHT, 0.0f));
}

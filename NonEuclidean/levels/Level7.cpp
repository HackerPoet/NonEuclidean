#include "Level7.h"
#include "Object.h"
#include "Resources.h"
#include "Sky.h"
#include "DeadTree.h"
#include "Tunnel.h"

void Level7::Load(World& world, Player& player) {
  std::shared_ptr<Object> ground(new Object());
  ground->mesh    = AquireMesh("ground_large.obj");
  ground->shader  = AquireShader("texture");
  ground->texture = AquireTexture("proto_dark_01.bmp");
  ground->scale   = Vector3(500, 1, 500);
  world.objects.push_back(ground);
  world.sky = std::make_shared<Sky>("sky");

  std::shared_ptr<DeadTree> tree(new DeadTree());
  tree->pos = Vector3(-3, 0, -5);
  world.objects.push_back(tree);

  std::shared_ptr<Object> cube(new Object());
  cube->mesh    = AquireMesh("cube.obj");
  cube->shader  = AquireShader("texture");
  cube->texture = AquireTexture("white.bmp");
  cube->pos     = Vector3(7, 0.5f, -8);
  cube->castShadow = false;
  cube->emissive = Vector3(4.0f, 3.8f, 3.2f);
  world.objects.push_back(cube);

  world.lights.push_back({cube->pos, Vector3(4.0f, 3.8f, 3.2f), 0.0f});
  // world.lightCookie = AquireTexture("proto_dark_01.bmp");

  // Door frame near the cube, looking at it
  std::shared_ptr<Tunnel> frame1(new Tunnel(Tunnel::NORMAL));
  frame1->pos = Vector3(4, 0, -8);
  frame1->scale = Vector3(1, 1, 0.1f);
  frame1->euler.y = -GH_PI / 2;
  frame1->texture = AquireTexture("proto_dark_01.bmp");
  world.objects.push_back(frame1);

  // Door frame elsewhere
  std::shared_ptr<Tunnel> frame2(new Tunnel(Tunnel::NORMAL));
  frame2->pos = Vector3(-5, 0, 5);
  frame2->scale = Vector3(1, 1, 0.1f);
  frame2->texture = AquireTexture("proto_dark_01.bmp");
  world.objects.push_back(frame2);

  std::shared_ptr<Portal> portal1(new Portal());
  frame1->SetDoor1(*portal1);
  world.portals.push_back(portal1);

  std::shared_ptr<Portal> portal2(new Portal());
  frame2->SetDoor1(*portal2);
  world.portals.push_back(portal2);

  Portal::Connect(portal1, portal2);

  player.SetPosition(Vector3(0, GH_PLAYER_HEIGHT, 0));
}



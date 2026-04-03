#include "Level8.h"
#include "Object.h"
#include "Resources.h"
#include "Sky.h"
#include "Tunnel.h"
#include "GameHeader.h"

void Level8::Load(World& world, Player& player) {
  std::shared_ptr<Texture> roomTex = AquireTexture("proto_dark_13.bmp");
  std::shared_ptr<Texture> probeTex = AquireTexture("white.bmp");

  // Same baseline as Level 7: wide outdoor plane plus sky.
  std::shared_ptr<Object> ground(new Object());
  ground->mesh    = AquireMesh("ground_large.obj");
  ground->shader  = AquireShader("texture");
  ground->texture = roomTex;
  ground->pos     = Vector3(0, -0.05f, 0); // avoid coplanar overlap with the box floor
  ground->scale   = Vector3(500, 1, 500);
  world.objects.push_back(ground);
  world.sky = std::make_shared<Sky>("sky");
  world.time_of_day = 0.0f; // keep the room dark so portal lighting reads clearly
  world.ambientScale = 0.35f;

  // Build a large double-sided box around the player.
  const float halfW = 6.0f;
  const float halfH = 3.0f;
  const float halfD = 6.0f;

  auto addPanel = [&](const Vector3& pos, const Vector3& scale, const Vector3& euler,
                      const char* meshName = "double_quad.obj") {
    std::shared_ptr<Object> panel(new Object());
    panel->mesh    = AquireMesh(meshName);
    panel->shader  = AquireShader("texture");
    panel->texture = roomTex;
    panel->pos     = pos;
    panel->scale   = scale;
    panel->euler   = euler;
    world.objects.push_back(panel);
  };

  // Front / back walls
  addPanel(Vector3(0, halfH, -halfD), Vector3(halfW, halfH, 1), Vector3(0, 0, 0));
  addPanel(Vector3(0, halfH,  halfD), Vector3(halfW, halfH, 1), Vector3(0, GH_PI, 0));
  // Left / right walls
  addPanel(Vector3(-halfW, halfH, 0), Vector3(halfD, halfH, 1), Vector3(0, -GH_PI / 2, 0));
  addPanel(Vector3( halfW, halfH, 0), Vector3(halfD, halfH, 1), Vector3(0,  GH_PI / 2, 0));
  // Floor / ceiling use single quads to avoid doubled coplanar triangles,
  // which otherwise show up as shadow acne on large flat surfaces.
  addPanel(Vector3(0, 0,       0), Vector3(halfW, halfD, 1), Vector3(-GH_PI / 2, 0, 0), "quad.obj");
  addPanel(Vector3(0, halfH*2, 0), Vector3(halfW, halfD, 1), Vector3( GH_PI / 2, 0, 0), "quad.obj");

  // Portal frame inside the box, mounted on the front wall so the default
  // player view (-Z) looks straight at it.
  std::shared_ptr<Tunnel> insideFrame(new Tunnel(Tunnel::NORMAL));
  insideFrame->pos     = Vector3(0, 0.0f, -halfD);
  insideFrame->scale   = Vector3(1.6f, 1.6f, 0.1f);
  insideFrame->euler   = Vector3(0, 0, 0);
  insideFrame->texture = AquireTexture("proto_dark_01.bmp");
  world.objects.push_back(insideFrame);

  // Linked portal outside the box, directly ahead in open space.
  std::shared_ptr<Tunnel> outsideFrame(new Tunnel(Tunnel::NORMAL));
  outsideFrame->pos     = Vector3(300.0f, 0.0f, -16.0f);
  outsideFrame->scale   = Vector3(1.6f, 1.6f, 0.1f);
  outsideFrame->euler   = Vector3(0, 0, 0);
  outsideFrame->texture = AquireTexture("proto_dark_01.bmp");
  world.objects.push_back(outsideFrame);

  std::shared_ptr<Portal> insidePortal(new Portal());
  insideFrame->SetDoor1(*insidePortal);
  world.portals.push_back(insidePortal);

  std::shared_ptr<Portal> outsidePortal(new Portal());
  outsideFrame->SetDoor1(*outsidePortal);
  world.portals.push_back(outsidePortal);

  Portal::Connect(insidePortal, outsidePortal);

  // External light source placed in front of the outside portal.
  std::shared_ptr<Object> lightCube(new Object());
  lightCube->mesh       = AquireMesh("cube.obj");
  lightCube->shader     = AquireShader("texture");
  lightCube->texture    = probeTex;
  lightCube->pos        = Vector3(300.0f, 1.5f, -18.5f);
  lightCube->scale      = Vector3(0.45f, 0.45f, 0.45f);
  lightCube->castShadow = false;
  lightCube->emissive   = Vector3(8.0f, 7.6f, 6.4f);
  world.objects.push_back(lightCube);

  world.lightPos   = lightCube->pos;
  world.lightColor = lightCube->emissive;

  // A thin blocker casts a shadow onto the readout screen when the light is warped correctly.
  std::shared_ptr<Object> shadowPost(new Object());
  shadowPost->mesh    = AquireMesh("cube.obj");
  shadowPost->shader  = AquireShader("texture");
  shadowPost->texture = roomTex;
  shadowPost->pos     = Vector3(299.05f, 1.2f, -4.85f);
  shadowPost->scale   = Vector3(0.14f, 2.4f, 0.14f);
  world.objects.push_back(shadowPost);

  // Spawn the player inside the box looking toward the inside portal.
  player.SetPosition(Vector3(0.0f, GH_PLAYER_HEIGHT, 2.15f));
  player.euler.y = 0.0f;
}

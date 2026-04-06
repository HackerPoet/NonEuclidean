#include "Level4.h"
#include "Object.h"
#include "Resources.h"
#include "Sky.h"

void Level4::Load(World& world, Player& player) {
  world.ambientScale = 0.5f;
  world.sky = std::make_shared<Sky>("sky");
  world.time_of_day = 0.0f;

  std::shared_ptr<Object> ground(new Object());
  ground->mesh = AquireMesh("ground_large.obj");
  ground->shader = AquireShader("texture");
  ground->texture = AquireTexture("proto_dark_01.bmp");
  ground->scale = Vector3(500.0f, 1.0f, 500.0f);
  world.objects.push_back(ground);

  world.importScene("level4.scene");

  // 10 point lights arranged in a ring around the player spawn
  struct LightDef { float x, y, z, r, g, b; };
  const LightDef lightDefs[10] = {
    {  0.0f, 4.0f,  15.0f,  4.0f, 3.8f, 3.2f },
    { 10.0f, 5.0f,  10.0f,  3.5f, 2.0f, 1.0f },
    { 15.0f, 3.5f,   0.0f,  2.0f, 4.0f, 2.0f },
    { 10.0f, 6.0f, -10.0f,  1.5f, 2.5f, 4.5f },
    {  0.0f, 4.5f, -15.0f,  4.0f, 4.0f, 2.0f },
    {-10.0f, 5.0f, -10.0f,  3.0f, 1.5f, 4.0f },
    {-15.0f, 3.5f,   0.0f,  4.5f, 3.0f, 1.5f },
    {-10.0f, 6.0f,  10.0f,  2.5f, 4.0f, 3.0f },
    {  5.0f, 8.0f,   0.0f,  3.8f, 3.8f, 3.8f },
    { -5.0f, 2.5f,   5.0f,  2.0f, 3.5f, 4.5f },
  };

  for (int i = 0; i < 10; ++i) {
    const LightDef& ld = lightDefs[i];
    Vector3 lpos(ld.x, ld.y, ld.z);
    Vector3 lcol(ld.r, ld.g, ld.b);

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

  player.SetPosition(Vector3(0.0f, GH_PLAYER_HEIGHT, 8.0f));
}

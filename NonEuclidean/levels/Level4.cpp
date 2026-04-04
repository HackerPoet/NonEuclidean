#include "Level4.h"
#include "Object.h"
#include "Resources.h"
#include "Sky.h"

void Level4::Load(World& world, Player& player) {
  world.ambientScale = 0.7f;
  world.sky = std::make_shared<Sky>("sky");
  world.time_of_day = 0.5f;

  std::shared_ptr<Object> ground(new Object());
  ground->mesh = AquireMesh("ground_large.obj");
  ground->shader = AquireShader("texture");
  ground->texture = AquireTexture("proto_dark_01.bmp");
  ground->scale = Vector3(500.0f, 1.0f, 500.0f);
  world.objects.push_back(ground);

  world.importScene("level4.scene");

  player.SetPosition(Vector3(0.0f, GH_PLAYER_HEIGHT, 8.0f));
}

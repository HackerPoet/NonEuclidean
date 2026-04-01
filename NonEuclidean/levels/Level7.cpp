#include "Level7.h"
#include "Object.h"
#include "Resources.h"

void Level7::Load(PObjectVec& objs, PPortalVec& portals, Player& player) {
  std::shared_ptr<Object> ground(new Object());
  ground->mesh    = AquireMesh("ground_large.obj");
  ground->shader  = AquireShader("texture");
  ground->texture = AquireTexture("proto_dark_01.bmp");
  ground->scale   = Vector3(500, 1, 500);
  objs.push_back(ground);

  player.SetPosition(Vector3(0, GH_PLAYER_HEIGHT, 0));
}



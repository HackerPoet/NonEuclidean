#include "Level3.h"
#include "Pillar.h"
#include "Ground.h"
#include "Statue.h"
#include "PillarRoom.h"

void Level3::Load(PObjectVec& objs, PPortalVec& portals, Player& player) {
  //Room 1
  std::shared_ptr<Pillar> pillar1(new Pillar);
  objs.push_back(pillar1);

  std::shared_ptr<PillarRoom> pillarRoom1(new PillarRoom);
  objs.push_back(pillarRoom1);

  std::shared_ptr<Ground> ground1(new Ground);
  ground1->scale *= 2.0f;
  objs.push_back(ground1);

  std::shared_ptr<Statue> statue1(new Statue("teapot.obj"));
  statue1->pos = Vector3(0, 0.5f, 9);
  statue1->scale = Vector3(0.5f);
  statue1->euler.y = GH_PI / 2;
  objs.push_back(statue1);

  //Room 2
  std::shared_ptr<Pillar> pillar2(new Pillar);
  pillar2->pos = Vector3(200, 0, 0);
  objs.push_back(pillar2);

  std::shared_ptr<PillarRoom> pillarRoom2(new PillarRoom);
  pillarRoom2->pos = Vector3(200, 0, 0);
  objs.push_back(pillarRoom2);

  std::shared_ptr<Ground> ground2(new Ground);
  ground2->pos = Vector3(200, 0, 0);
  ground2->scale *= 2.0f;
  objs.push_back(ground2);

  std::shared_ptr<Statue> statue2(new Statue("bunny.obj"));
  statue2->pos = Vector3(200, -0.4f, 9);
  statue2->scale = Vector3(14.0f);
  statue2->euler.y = GH_PI;
  objs.push_back(statue2);

  //Room 3
  std::shared_ptr<Pillar> pillar3(new Pillar);
  pillar3->pos = Vector3(400, 0, 0);
  objs.push_back(pillar3);

  std::shared_ptr<PillarRoom> pillarRoom3(new PillarRoom);
  pillarRoom3->pos = Vector3(400, 0, 0);
  objs.push_back(pillarRoom3);

  std::shared_ptr<Ground> ground3(new Ground);
  ground3->pos = Vector3(400, 0, 0);
  ground3->scale *= 2.0f;
  objs.push_back(ground3);

  std::shared_ptr<Statue> statue3(new Statue("suzanne.obj"));
  statue3->pos = Vector3(400, 0.9f, 9);
  statue3->scale = Vector3(1.2f);
  statue3->euler.y = GH_PI;
  objs.push_back(statue3);

  //Portals
  std::shared_ptr<Portal> portal1(new Portal);
  pillarRoom1->SetPortal(*portal1);
  portals.push_back(portal1);

  std::shared_ptr<Portal> portal2(new Portal);
  pillarRoom2->SetPortal(*portal2);
  portals.push_back(portal2);

  std::shared_ptr<Portal> portal3(new Portal);
  pillarRoom3->SetPortal(*portal3);
  portals.push_back(portal3);

  Portal::Connect(portal1->front, portal2->back);
  Portal::Connect(portal2->front, portal3->back);
  Portal::Connect(portal3->front, portal1->back);

  player.SetPosition(Vector3(0, GH_PLAYER_HEIGHT, 3));
}

#include "Level1.h"
#include "Tunnel.h"
#include "Ground.h"

void Level1::Load(PObjectVec& objs, PPortalVec& portals, Player& player) {
  std::shared_ptr<Tunnel> tunnel1(new Tunnel(Tunnel::NORMAL));
  tunnel1->pos = Vector3(-2.4f, 0, -1.8f);
  tunnel1->scale = Vector3(1, 1, 4.8f);
  objs.push_back(tunnel1);

  std::shared_ptr<Tunnel> tunnel2(new Tunnel(Tunnel::NORMAL));
  tunnel2->pos = Vector3(2.4f, 0, 0);
  tunnel2->scale = Vector3(1, 1, 0.6f);
  objs.push_back(tunnel2);

  std::shared_ptr<Ground> ground(new Ground());
  ground->scale *= 1.2f;
  objs.push_back(ground);

  std::shared_ptr<Portal> portal1(new Portal());
  tunnel1->SetDoor1(*portal1);
  portals.push_back(portal1);

  std::shared_ptr<Portal> portal2(new Portal());
  tunnel2->SetDoor1(*portal2);
  portals.push_back(portal2);

  std::shared_ptr<Portal> portal3(new Portal());
  tunnel1->SetDoor2(*portal3);
  portals.push_back(portal3);

  std::shared_ptr<Portal> portal4(new Portal());
  tunnel2->SetDoor2(*portal4);
  portals.push_back(portal4);

  Portal::Connect(portal1, portal2);
  Portal::Connect(portal3, portal4);

  player.SetPosition(Vector3(0, GH_PLAYER_HEIGHT, 5));
}

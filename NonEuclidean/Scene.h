#pragma once
#include "Object.h"
#include "Portal.h"
#include "Player.h"

class Scene {
public:
  virtual void Load(PObjectVec& objs, PPortalVec& portals, Player& player)=0;
  virtual void Unload() {};
};

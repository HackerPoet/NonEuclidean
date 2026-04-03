#pragma once
#include "world.h"
#include "Player.h"

class Scene {
public:
  virtual void Load(World& world, Player& player) = 0;
  virtual void Unload() {};
};

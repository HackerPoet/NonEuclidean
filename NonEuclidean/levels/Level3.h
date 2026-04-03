#pragma once
#include "Scene.h"

class Level3 : public Scene {
public:
  virtual void Load(World& world, Player& player) override;
};

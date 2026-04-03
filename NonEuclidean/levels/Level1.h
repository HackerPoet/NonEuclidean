#pragma once
#include "Scene.h"

class Level1 : public Scene {
public:
  virtual void Load(World& world, Player& player) override;
  void CreateDeadForest(World &world);
};

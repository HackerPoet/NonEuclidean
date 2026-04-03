#pragma once
#include "Scene.h"

class Level2 : public Scene {
public:
  Level2(int rooms) : num_rooms(rooms) {}

  virtual void Load(World& world, Player& player) override;

private:
  int num_rooms;
};

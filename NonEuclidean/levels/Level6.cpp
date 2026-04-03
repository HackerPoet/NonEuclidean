#include "Level6.h"
#include "Floorplan.h"

void Level6::Load(World& world, Player& player) {
  std::shared_ptr<Floorplan> floorplan(new Floorplan);
  world.objects.push_back(floorplan);
  floorplan->AddPortals(world.portals);

  player.SetPosition(Vector3(2, GH_PLAYER_HEIGHT, 2));
}

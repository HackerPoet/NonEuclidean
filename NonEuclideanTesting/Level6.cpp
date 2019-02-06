#include "Level6.h"
#include "Floorplan.h"

void Level6::Load(PObjectVec& objs, PPortalVec& portals, Player& player) {
  std::shared_ptr<Floorplan> floorplan(new Floorplan);
  objs.push_back(floorplan);
  floorplan->AddPortals(portals);

  player.SetPosition(Vector3(2, GH_PLAYER_HEIGHT, 2));
}

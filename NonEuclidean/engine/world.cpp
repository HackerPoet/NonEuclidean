#include "world.h"

void World::Clear() {
  objects.clear();
  portals.clear();
  sky.reset();
  lightPos = Vector3(0.0f);
  lightColor = Vector3(0.0f);
  ambientScale = 1.0f;
  lightVP.MakeIdentity();
  lightCookie.reset();
  time_of_day = 3000.0f;
}

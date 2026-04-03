#pragma once
#include "Object.h"
#include "Portal.h"
#include "Sky.h"
#include "Texture.h"
#include <memory>

struct World {
  PObjectVec objects;
  PPortalVec portals;
  std::shared_ptr<Sky> sky; // null = use engine default sky

  Vector3 lightPos;
  Vector3 lightColor;
  Vector3 lightPos2;
  Vector3 lightColor2;
  float ambientScale = 1.0f; // multiplies the shader ambient term for this world
  Matrix4 lightVP; // light view-projection for shadow mapping
  std::shared_ptr<Texture> lightCookie; // projected light cookie texture
  float time_of_day = 3000.0f; // 0=midnight, 0.25=sunrise, 0.5=noon, 0.75=sunset

  void Clear();
};

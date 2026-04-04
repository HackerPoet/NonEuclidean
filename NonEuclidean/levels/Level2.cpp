#include "Level2.h"
#include "Object.h"
#include "PortalFrameTrees.h"
#include "Resources.h"
#include "Sky.h"
#include <cmath>

namespace {

std::shared_ptr<Portal> CreatePortal(World& world, const Vector3& pos, float yaw, const Vector3& scale) {
  std::shared_ptr<Portal> portal(new Portal());
  portal->pos = pos;
  portal->euler.y = yaw;
  portal->scale = scale;
  world.portals.push_back(portal);
  return portal;
}

std::shared_ptr<Object> CreateLightCube(const Vector3& pos, const Vector3& color) {
  std::shared_ptr<Object> cube(new Object());
  cube->mesh = AquireMesh("cube.obj");
  cube->shader = AquireShader("texture");
  cube->texture = AquireTexture("white.bmp");
  cube->pos = pos;
  cube->castShadow = false;
  cube->emissive = color;
  return cube;
}

}

void Level2::Load(World& world, Player& player) {
  const bool wideLayout = true;
  const PortalFrameLayoutSettings frameLayout = GetLevel2PortalFrameLayoutSettings(wideLayout);

  world.ambientScale = 0.7f;
  world.sky = std::make_shared<Sky>("sky");

  std::shared_ptr<Object> ground(new Object());
  ground->mesh = AquireMesh("ground_large.obj");
  ground->shader = AquireShader("texture");
  ground->texture = AquireTexture("proto_dark_01.bmp");
  ground->scale = Vector3(250.0f, 1.0f, 250.0f);
  world.objects.push_back(ground);

  std::shared_ptr<Object> lightCube = CreateLightCube(Vector3(0.0f, 4.0f, 0.0f), Vector3(4.0f, 3.8f, 3.2f));
  world.objects.push_back(lightCube);
  world.lightPos = lightCube->pos;
  world.lightColor = lightCube->emissive;

  std::shared_ptr<Object> accentLightCube = CreateLightCube(
      Vector3(-(frameLayout.portalHalfWidth + 4.0f), frameLayout.portalHalfHeight * 0.65f, -frameLayout.portalDistance * 0.35f),
      Vector3(2.6f, 3.0f, 4.4f));
  world.objects.push_back(accentLightCube);
  world.lightPos2 = accentLightCube->pos;
  world.lightColor2 = accentLightCube->emissive;

  const Vector3 portalScale(frameLayout.portalHalfWidth, frameLayout.portalHalfHeight, 1.0f);

  // Keep the portals facing each other so Level 2 works as a simple portal test range.
  std::shared_ptr<Portal> portalA = CreatePortal(world, Vector3(0.0f, frameLayout.portalHalfHeight, -frameLayout.portalDistance), 0.0f, portalScale);
  std::shared_ptr<Portal> portalB = CreatePortal(world, Vector3(0.0f, frameLayout.portalHalfHeight, frameLayout.portalDistance), 0.0f, portalScale);

  AddPortalFrameTrees(world, portalA->pos, portalA->euler.y, frameLayout.portalHalfWidth, frameLayout.treeScale);
  AddPortalFrameTrees(world, portalB->pos, portalB->euler.y, frameLayout.portalHalfWidth, frameLayout.treeScale);

  Portal::Connect(portalA, portalB);

  player.SetPosition(Vector3(0.0f, GH_PLAYER_HEIGHT, wideLayout ? 6.0f : 4.0f));
}

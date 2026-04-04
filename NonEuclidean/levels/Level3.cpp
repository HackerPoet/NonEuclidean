#include "Level3.h"
#include "Object.h"
#include "Portal.h"
#include "PortalMeshWarp.h"
#include "Resources.h"
#include "Sky.h"
#include <cmath>

namespace {

std::shared_ptr<Object> CreateStaticObject(
  const char* meshName,
  const char* textureName,
  const Vector3& pos,
  const Vector3& scale,
  float yaw = 0.0f,
  const Vector3& emissive = Vector3::Zero()) {
  std::shared_ptr<Object> object(new Object());
  object->mesh = AquireMesh(meshName);
  object->shader = AquireShader("texture");
  object->texture = AquireTexture(textureName);
  object->pos = pos;
  object->scale = scale;
  object->euler.y = yaw;
  object->emissive = emissive;
  if (emissive.MagSq() > 0.0f) {
    object->castShadow = false;
  }
  return object;
}

std::shared_ptr<Object> CreateLightCube(const Vector3& pos, const Vector3& color) {
  return CreateStaticObject("cube.obj", "white.bmp", pos, Vector3(0.8f), 0.0f, color);
}

std::shared_ptr<Portal> CreatePortal(World& world, const Vector3& pos, float yaw, const Vector3& scale) {
  std::shared_ptr<Portal> portal(new Portal());
  portal->pos = pos;
  portal->euler.y = yaw;
  portal->scale = scale;
  world.portals.push_back(portal);
  return portal;
}

void AddPortalFrame(World& world, const Vector3& portalPos, const Vector3& portalScale, float portalYaw) {
  const Vector3 right(std::cos(portalYaw), 0.0f, std::sin(portalYaw));
  const Vector3 up = Vector3::UnitY();
  const float postOffset = portalScale.x + 0.45f;
  const float topOffset = portalScale.y + 0.45f;
  const Vector3 postScale(0.35f, portalScale.y + 0.75f, 0.35f);
  const Vector3 beamScale(portalScale.x + 0.9f, 0.35f, 0.35f);

  world.objects.push_back(CreateStaticObject(
    "cube.obj", "proto_dark_01.bmp", portalPos - right * postOffset, postScale, portalYaw));
  world.objects.push_back(CreateStaticObject(
    "cube.obj", "proto_dark_01.bmp", portalPos + right * postOffset, postScale, portalYaw));
  world.objects.push_back(CreateStaticObject(
    "cube.obj", "proto_dark_01.bmp", portalPos + up * topOffset, beamScale, portalYaw));
}

std::shared_ptr<Object> CreateWarpProxy(const std::shared_ptr<Object>& source, const std::shared_ptr<Mesh>& mesh) {
  std::shared_ptr<Object> proxy(new Object());
  proxy->mesh = mesh;
  proxy->shader = source->shader;
  proxy->texture = source->texture;
  proxy->castShadow = source->castShadow;
  proxy->emissive = source->emissive;
  return proxy;
}

void AddWarpedPreview(
  World& world,
  const char* meshName,
  const char* textureName,
  const Vector3& pos,
  const Vector3& scale,
  float yaw,
  const Portal& portal,
  const Portal::Warp& warp,
  PortalWarpSide sideToWarp) {
  std::shared_ptr<Object> source = CreateStaticObject(meshName, textureName, pos, scale, yaw);
  if (!source->mesh) {
    return;
  }

  source->visible = false;
  world.objects.push_back(source);

  PortalMeshSplitResult split = BuildPortalWarpMeshSplit(
    *source->mesh,
    source->LocalToWorld(),
    portal.pos,
    portal.Forward(),
    warp.deltaInv,
    sideToWarp);

  if (split.localMesh) {
    world.objects.push_back(CreateWarpProxy(source, split.localMesh));
  }
  if (split.warpedMesh) {
    world.objects.push_back(CreateWarpProxy(source, split.warpedMesh));
  }
}

}

void Level3::Load(World& world, Player& player) {
  const Vector3 portalScale(3.0f, 3.4f, 1.0f);

  world.ambientScale = 0.65f;
  world.sky = std::make_shared<Sky>("sky");

  world.objects.push_back(CreateStaticObject(
    "ground_large.obj", "proto_dark_01.bmp", Vector3(10.0f, 0.0f, -8.0f), Vector3(240.0f, 1.0f, 240.0f)));

  std::shared_ptr<Object> keyLight = CreateLightCube(Vector3(-4.0f, 7.5f, 10.0f), Vector3(4.4f, 4.0f, 3.4f));
  world.objects.push_back(keyLight);
  world.lightPos = keyLight->pos;
  world.lightColor = keyLight->emissive;

  std::shared_ptr<Object> fillLight = CreateLightCube(Vector3(26.0f, 6.5f, -12.0f), Vector3(2.4f, 3.2f, 4.8f));
  world.objects.push_back(fillLight);
  world.lightPos2 = fillLight->pos;
  world.lightColor2 = fillLight->emissive;

  std::shared_ptr<Portal> portalA = CreatePortal(world, Vector3(0.0f, portalScale.y, 0.0f), GH_PI, portalScale);
  std::shared_ptr<Portal> portalB = CreatePortal(world, Vector3(26.0f, portalScale.y, -14.0f), -GH_PI / 2.0f, portalScale);
  Portal::Connect(portalA, portalB);

  AddPortalFrame(world, portalA->pos, portalScale, portalA->euler.y);
  AddPortalFrame(world, portalB->pos, portalScale, portalB->euler.y);

  // Reference objects on the approach side.
  world.objects.push_back(CreateStaticObject(
    "cube.obj", "white.bmp", Vector3(-7.5f, 1.35f, 8.0f), Vector3(2.2f, 2.2f, 2.2f)));
  world.objects.push_back(CreateStaticObject(
    "teapot.obj", "gold.bmp", Vector3(6.5f, 1.2f, 8.0f), Vector3(0.85f), GH_PI * 0.18f));

  // Two static objects intentionally straddle the portal plane so their far
  // halves are rendered as warped proxy meshes on the connected portal side.
  AddWarpedPreview(
    world,
    "cube.obj",
    "white.bmp",
    Vector3(-1.35f, 1.35f, 0.0f),
    Vector3(2.2f, 2.2f, 2.2f),
    0.0f,
    *portalA,
    portalA->front,
    PortalWarpSide::Negative);

  AddWarpedPreview(
    world,
    "teapot.obj",
    "gold.bmp",
    Vector3(1.45f, 1.15f, 0.0f),
    Vector3(0.85f),
    GH_PI * 0.18f,
    *portalA,
    portalA->front,
    PortalWarpSide::Negative);

  player.SetPosition(Vector3(0.0f, GH_PLAYER_HEIGHT, 12.0f));
}

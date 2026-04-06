#include "Level5.h"
#include "Ground.h"
#include "Resources.h"

void Level5::Load(World& world, Player& player) {
  std::shared_ptr<Mesh> sceneMesh = AquireMesh("18728_autosave.glb");

  // Override the Ground node's texture with proto_dark_01 and tile it
  {
    auto it = sceneMesh->nodeVertexRanges.find("Ground");
    if (it != sceneMesh->nodeVertexRanges.end()) {
      std::shared_ptr<Texture> groundTex = AquireTexture("proto_dark_01.bmp");
      uint32_t texIdx = (uint32_t)sceneMesh->embeddedTextures.size();
      sceneMesh->embeddedTextures.push_back(groundTex);
      int uvStride = sceneMesh->is3DTex ? 3 : 2;
      float uvScale = 100.0f;
      for (size_t v = it->second.first; v < it->second.second; ++v) {
        sceneMesh->texIndices[v] = texIdx;
        sceneMesh->uvs[v * uvStride + 0] *= uvScale;
        sceneMesh->uvs[v * uvStride + 1] *= uvScale;
      }
    }
  }

  std::shared_ptr<Object> customObj(new Object());
  customObj->mesh = sceneMesh;
  world.objects.push_back(customObj);
  world.time_of_day = 0.0f;

  // Create emissive objects and register lights from pre-extracted light markers
  for (size_t li = 0; li < sceneMesh->lightMarkers.size(); ++li) {
    const Mesh::LightMarker& lm = sceneMesh->lightMarkers[li];
    std::shared_ptr<Object> lightObj(new Object());
    lightObj->mesh = lm.mesh;
    lightObj->emissive = lm.color;
    lightObj->castShadow = false;
    world.objects.push_back(lightObj);
    world.lights.push_back({lm.center, lm.color, lm.radius});
  }

  // Side 1  (Blender: 238.8, -248.39, 11.664  rot 90/0/88)
  std::shared_ptr<Portal> side1(new Portal());
  side1->pos = Vector3(238.8f, 11.664f, 248.39f);
  side1->euler.y = 88.0f * GH_PI / 180.0f;
  side1->scale = Vector3(8.12f, 12.45f, 1.0f);

  // Side 2  (Blender: 366.22, -247.33, 11.664  rot 90/0/84)
  std::shared_ptr<Portal> side2(new Portal());
  side2->pos = Vector3(366.22f, 11.664f, 247.33f);
  side2->euler.y = 84.0f * GH_PI / 180.0f;
  side2->scale = Vector3(8.12f, 12.45f, 1.0f);

  Portal::Connect(side1, side2);
  world.portals.push_back(side1);
  world.portals.push_back(side2);

  // Back  (Blender: 292.86, -338.77, 11.664  rot 90/0/0)
  std::shared_ptr<Portal> back(new Portal());
  back->pos = Vector3(292.86f, 11.664f, 338.77f);
  back->euler.y = 0.0f;
  back->scale = Vector3(8.12f, 12.45f, 1.0f);

  // Back partner  (Blender: 248.02, -161.88, 11.664  rot 90/0/38)
  std::shared_ptr<Portal> backPartner(new Portal());
  backPartner->pos = Vector3(248.02f, 11.664f, 161.88f);
  backPartner->euler.y = 38.0f * GH_PI / 180.0f;
  backPartner->scale = Vector3(8.12f, 12.45f, 1.0f);

  Portal::Connect(back, backPartner);
  world.portals.push_back(back);
  world.portals.push_back(backPartner);

  // Floor portal high  (Blender: 216.12, -135.63, 100.63  rot 0/0/-56.383)
  std::shared_ptr<Portal> floorHigh(new Portal());
  floorHigh->pos = Vector3(216.12f, 100.63f, 135.63f);
  floorHigh->euler.x = GH_PI / 2.0f;
  floorHigh->euler.y = -56.383f * GH_PI / 180.0f;
  floorHigh->scale = Vector3(8.108f, 15.469f, 1.469f);

  // Floor portal low  (Blender: 296.19, -160.08, 9.0157  rot 0/0/0)
  std::shared_ptr<Portal> floorLow(new Portal());
  floorLow->pos = Vector3(296.19f, 9.0157f, 160.08f);
  floorLow->euler.x = -GH_PI / 2.0f;
  floorLow->scale = Vector3(8.108f, 15.469f, 1.469f);

  Portal::Connect(floorHigh, floorLow);
  world.portals.push_back(floorHigh);
  world.portals.push_back(floorLow);

  world.ambientScale = 0.5f;

  player.SetPosition(Vector3(248.414413f, 1.5f, 166.681381f));
}

#include "world.h"
#include "PortalFrameTrees.h"
#include "SceneFile.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <string>

namespace {

struct PortalMarkerPair {
  std::shared_ptr<Object> markerA;
  std::shared_ptr<Object> markerB;
  std::string displayName;
};

std::string TrimString(const std::string& value) {
  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }

  size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }

  return value.substr(start, end - start);
}

std::string ToLowerString(const std::string& value) {
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return lower;
}

bool ParsePortalMarkerName(const std::string& objectName, std::string& pairKey, std::string& displayName, char& side) {
  const std::string trimmed = TrimString(objectName);
  const std::string lower = ToLowerString(trimmed);
  static const std::string prefix = "portal ";

  if (lower.size() <= prefix.size() + 1 || lower.compare(0, prefix.size(), prefix) != 0) {
    return false;
  }

  side = lower.back();
  if ((side != 'a' && side != 'b') || !std::isspace(static_cast<unsigned char>(trimmed[trimmed.size() - 2]))) {
    return false;
  }

  displayName = TrimString(trimmed.substr(prefix.size(), trimmed.size() - prefix.size() - 2));
  if (displayName.empty()) {
    return false;
  }

  pairKey = ToLowerString(displayName);
  return true;
}

float PortalYawFromDirection(const Vector3& from, const Vector3& to) {
  Vector3 direction = to - from;
  direction.y = 0.0f;
  if (direction.MagSq() <= 1e-6f) {
    return 0.0f;
  }

  direction.Normalize();
  return std::atan2(-direction.x, -direction.z);
}

float MarkerGroundY(const std::shared_ptr<Object>& marker) {
  return marker->pos.y - marker->scale.y * marker->p_scale * 0.5f;
}

std::shared_ptr<Portal> CreatePortal(World& world, const std::string& name, const Vector3& pos, float yaw, const Vector3& scale) {
  std::shared_ptr<Portal> portal(new Portal());
  portal->name = name;
  portal->pos = pos;
  portal->euler.y = yaw;
  portal->scale = scale;
  world.portals.push_back(portal);
  return portal;
}

}

void World::Clear() {
  objects.clear();
  portals.clear();
  sky.reset();
  lights.clear();
  ambientScale = 1.0f;
  lightVP.MakeIdentity();
  lightCookie.reset();
  time_of_day = 3000.0f;
}

void World::materializePortalMarkers() {
  const PortalFrameLayoutSettings frameLayout = GetLevel2PortalFrameLayoutSettings(true);
  const Vector3 portalScale(frameLayout.portalHalfWidth, frameLayout.portalHalfHeight, 1.0f);
  std::map<std::string, PortalMarkerPair> markerPairs;

  for (size_t i = 0; i < objects.size(); ++i) {
    const std::shared_ptr<Object>& object = objects[i];
    if (!object || !object->isMarker) {
      continue;
    }

    std::string pairKey;
    std::string displayName;
    char side = 0;
    if (!ParsePortalMarkerName(object->name, pairKey, displayName, side)) {
      continue;
    }

    PortalMarkerPair& pair = markerPairs[pairKey];
    if (pair.displayName.empty()) {
      pair.displayName = displayName;
    }
    if (side == 'a' && !pair.markerA) {
      pair.markerA = object;
    } else if (side == 'b' && !pair.markerB) {
      pair.markerB = object;
    }
  }

  std::set<Object*> consumedMarkers;
  for (std::map<std::string, PortalMarkerPair>::iterator it = markerPairs.begin(); it != markerPairs.end(); ++it) {
    PortalMarkerPair& pair = it->second;
    if (!pair.markerA || !pair.markerB) {
      continue;
    }

    const float portalYaw = PortalYawFromDirection(pair.markerA->pos, pair.markerB->pos);
    const float portalHalfHeight = frameLayout.portalHalfHeight;

    Vector3 portalAPos(pair.markerA->pos.x, MarkerGroundY(pair.markerA) + portalHalfHeight, pair.markerA->pos.z);
    Vector3 portalBPos(pair.markerB->pos.x, MarkerGroundY(pair.markerB) + portalHalfHeight, pair.markerB->pos.z);

    std::shared_ptr<Portal> portalA = CreatePortal(*this, "Portal " + pair.displayName + " A", portalAPos, portalYaw, portalScale);
    std::shared_ptr<Portal> portalB = CreatePortal(*this, "Portal " + pair.displayName + " B", portalBPos, portalYaw, portalScale);
    Portal::Connect(portalA, portalB);

    AddPortalFrameTreesNamed(*this, portalA->name, portalA->pos, portalA->euler.y, frameLayout.portalHalfWidth, frameLayout.treeScale);
    AddPortalFrameTreesNamed(*this, portalB->name, portalB->pos, portalB->euler.y, frameLayout.portalHalfWidth, frameLayout.treeScale);

    consumedMarkers.insert(pair.markerA.get());
    consumedMarkers.insert(pair.markerB.get());
  }

  if (consumedMarkers.empty()) {
    return;
  }

  PObjectVec remainingObjects;
  remainingObjects.reserve(objects.size() - consumedMarkers.size());
  for (size_t i = 0; i < objects.size(); ++i) {
    if (consumedMarkers.find(objects[i].get()) == consumedMarkers.end()) {
      remainingObjects.push_back(objects[i]);
    }
  }
  objects.swap(remainingObjects);
}

bool World::importScene(const char* filename) {
  World importedWorld;
  if (!LoadSceneFile(filename, importedWorld, nullptr)) {
    return false;
  }

  objects.insert(objects.end(), importedWorld.objects.begin(), importedWorld.objects.end());
  portals.insert(portals.end(), importedWorld.portals.begin(), importedWorld.portals.end());
  if (importedWorld.sky) {
    sky = importedWorld.sky;
  }
  ambientScale = importedWorld.ambientScale;
  time_of_day = importedWorld.time_of_day;
  lights.insert(lights.end(), importedWorld.lights.begin(), importedWorld.lights.end());
  lightCookie = importedWorld.lightCookie;
  lightVP = importedWorld.lightVP;
  materializePortalMarkers();
  return true;
}

bool World::exportScene(const char* filename) const {
  return SaveSceneFile(filename, *this, nullptr);
}

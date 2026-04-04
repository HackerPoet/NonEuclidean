#include "SceneFile.h"
#include "Player.h"
#include "Portal.h"
#include "Resources.h"
#include "Shader.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

const char kSceneMagic[] = "NONEUCLIDEAN_SCENE_V1";

void WriteVector3(std::ostream& out, const Vector3& v) {
  out << v.x << ' ' << v.y << ' ' << v.z;
}

bool ReadVector3(std::istream& in, Vector3& v) {
  return bool(in >> v.x >> v.y >> v.z);
}

void WriteMatrix4(std::ostream& out, const Matrix4& m) {
  for (int i = 0; i < 16; ++i) {
    if (i > 0) out << ' ';
    out << m.m[i];
  }
}

bool ReadMatrix4(std::istream& in, Matrix4& m) {
  for (int i = 0; i < 16; ++i) {
    if (!(in >> m.m[i])) {
      return false;
    }
  }
  return true;
}

void WriteFloatArray(std::ostream& out, const std::vector<float>& values) {
  out << values.size();
  for (size_t i = 0; i < values.size(); ++i) {
    out << ' ' << values[i];
  }
  out << '\n';
}

bool ReadFloatArray(std::istream& in, std::vector<float>& values) {
  size_t count = 0;
  if (!(in >> count)) {
    return false;
  }
  values.resize(count);
  for (size_t i = 0; i < count; ++i) {
    if (!(in >> values[i])) {
      return false;
    }
  }
  return true;
}

void WriteOptionalResource(std::ostream& out, const char* label, const std::string& value) {
  out << label << ' ' << (value.empty() ? "-" : value) << '\n';
}

std::string NormalizeToken(const std::string& value) {
  return value == "-" ? std::string() : value;
}

void WriteObject(std::ostream& out, const Object& object) {
  out << "OBJECT\n";
  out << "NAME " << std::quoted(object.name) << '\n';
  out << "POS "; WriteVector3(out, object.pos); out << '\n';
  out << "EULER "; WriteVector3(out, object.euler); out << '\n';
  out << "SCALE "; WriteVector3(out, object.scale); out << '\n';
  out << "PSCALE " << object.p_scale << '\n';
  out << "VISIBLE " << (object.visible ? 1 : 0) << '\n';
  out << "CASTSHADOW " << (object.castShadow ? 1 : 0) << '\n';
  out << "MARKER " << (object.isMarker ? 1 : 0) << '\n';
  out << "EMISSIVE "; WriteVector3(out, object.emissive); out << '\n';

  if (object.mesh && !object.mesh->sourceName.empty()) {
    out << "MESH FILE " << object.mesh->sourceName << '\n';
  } else if (object.mesh) {
    out << "MESH DYNAMIC " << (object.mesh->is3DTex ? 1 : 0) << '\n';
    out << "VERTS "; WriteFloatArray(out, object.mesh->verts);
    out << "UVS "; WriteFloatArray(out, object.mesh->uvs);
  } else {
    out << "MESH NONE\n";
  }

  if (object.texture) {
    out << "TEXTURE " << object.texture->sourceName << ' '
        << object.texture->rows << ' ' << object.texture->cols << '\n';
  } else {
    out << "TEXTURE - 1 1\n";
  }

  WriteOptionalResource(out, "SHADER", object.shader ? object.shader->sourceName : std::string());
  out << "ENDOBJECT\n";
}

bool ReadObject(std::istream& in, World& world) {
  std::shared_ptr<Object> object(new Object());
  std::vector<float> dynamicVerts;
  std::vector<float> dynamicUvs;
  bool dynamicMesh = false;
  bool meshIs3DTex = false;
  std::string meshFile;
  std::string textureFile;
  int textureRows = 1;
  int textureCols = 1;
  std::string shaderName;

  std::string token;
  while (in >> token) {
    if (token == "ENDOBJECT") {
      break;
    }
    if (token == "NAME") {
      if (!(in >> std::quoted(object->name))) return false;
    } else if (token == "POS") {
      if (!ReadVector3(in, object->pos)) return false;
    } else if (token == "EULER") {
      if (!ReadVector3(in, object->euler)) return false;
    } else if (token == "SCALE") {
      if (!ReadVector3(in, object->scale)) return false;
    } else if (token == "PSCALE") {
      if (!(in >> object->p_scale)) return false;
    } else if (token == "VISIBLE") {
      int flag = 0;
      if (!(in >> flag)) return false;
      object->visible = flag != 0;
    } else if (token == "CASTSHADOW") {
      int flag = 0;
      if (!(in >> flag)) return false;
      object->castShadow = flag != 0;
    } else if (token == "MARKER") {
      int flag = 0;
      if (!(in >> flag)) return false;
      object->isMarker = flag != 0;
    } else if (token == "EMISSIVE") {
      if (!ReadVector3(in, object->emissive)) return false;
    } else if (token == "MESH") {
      std::string meshKind;
      if (!(in >> meshKind)) return false;
      if (meshKind == "FILE") {
        if (!(in >> meshFile)) return false;
        meshFile = NormalizeToken(meshFile);
      } else if (meshKind == "DYNAMIC") {
        int is3D = 0;
        if (!(in >> is3D)) return false;
        dynamicMesh = true;
        meshIs3DTex = is3D != 0;
      } else if (meshKind != "NONE") {
        return false;
      }
    } else if (token == "VERTS") {
      if (!ReadFloatArray(in, dynamicVerts)) return false;
    } else if (token == "UVS") {
      if (!ReadFloatArray(in, dynamicUvs)) return false;
    } else if (token == "TEXTURE") {
      if (!(in >> textureFile >> textureRows >> textureCols)) return false;
      textureFile = NormalizeToken(textureFile);
    } else if (token == "SHADER") {
      if (!(in >> shaderName)) return false;
      shaderName = NormalizeToken(shaderName);
    } else {
      return false;
    }
  }

  if (!meshFile.empty()) {
    object->mesh = AquireMesh(meshFile.c_str());
  } else if (dynamicMesh) {
    object->mesh = std::make_shared<Mesh>(dynamicVerts, dynamicUvs, meshIs3DTex);
  }

  if (!textureFile.empty()) {
    object->texture = AquireTexture(textureFile.c_str(), textureRows, textureCols);
  }
  if (!shaderName.empty()) {
    object->shader = AquireShader(shaderName.c_str());
  }

  world.objects.push_back(object);
  return true;
}

void WritePortal(std::ostream& out, const Portal& portal) {
  out << "PORTAL\n";
  out << "POS "; WriteVector3(out, portal.pos); out << '\n';
  out << "EULER "; WriteVector3(out, portal.euler); out << '\n';
  out << "SCALE "; WriteVector3(out, portal.scale); out << '\n';
  out << "FRONTDELTA "; WriteMatrix4(out, portal.front.delta); out << '\n';
  out << "FRONTDELTAINV "; WriteMatrix4(out, portal.front.deltaInv); out << '\n';
  out << "BACKDELTA "; WriteMatrix4(out, portal.back.delta); out << '\n';
  out << "BACKDELTAINV "; WriteMatrix4(out, portal.back.deltaInv); out << '\n';
  out << "ENDPORTAL\n";
}

bool ReadPortal(std::istream& in, World& world) {
  std::shared_ptr<Portal> portal(new Portal());
  std::string token;
  while (in >> token) {
    if (token == "ENDPORTAL") {
      break;
    }
    if (token == "POS") {
      if (!ReadVector3(in, portal->pos)) return false;
    } else if (token == "EULER") {
      if (!ReadVector3(in, portal->euler)) return false;
    } else if (token == "SCALE") {
      if (!ReadVector3(in, portal->scale)) return false;
    } else if (token == "FRONTDELTA") {
      if (!ReadMatrix4(in, portal->front.delta)) return false;
    } else if (token == "FRONTDELTAINV") {
      if (!ReadMatrix4(in, portal->front.deltaInv)) return false;
    } else if (token == "BACKDELTA") {
      if (!ReadMatrix4(in, portal->back.delta)) return false;
    } else if (token == "BACKDELTAINV") {
      if (!ReadMatrix4(in, portal->back.deltaInv)) return false;
    } else {
      return false;
    }
  }

  world.portals.push_back(portal);
  return true;
}

void WritePlayer(std::ostream& out, const Player* player) {
  out << "PLAYER " << (player ? 1 : 0) << '\n';
  if (!player) {
    return;
  }

  out << "PLAYERPOS "; WriteVector3(out, player->pos); out << '\n';
  out << "PLAYEREULER "; WriteVector3(out, player->euler); out << '\n';
  out << "PLAYERPSCALE " << player->p_scale << '\n';
  out << "PLAYERVELOCITY "; WriteVector3(out, player->velocity); out << '\n';
  out << "PLAYERLOOK " << player->LookPitch() << ' ' << player->LookYaw() << '\n';
}

bool ReadPlayer(std::istream& in, Player* player, int hasPlayer) {
  if (!hasPlayer || !player) {
    return true;
  }

  std::string token;
  while (in >> token) {
    if (token == "ENDSCENE" || token == "OBJECTS" || token == "PORTALS") {
      return false;
    }
    if (token == "PLAYERPOS") {
      if (!ReadVector3(in, player->pos)) return false;
      player->prev_pos = player->pos;
    } else if (token == "PLAYEREULER") {
      if (!ReadVector3(in, player->euler)) return false;
    } else if (token == "PLAYERPSCALE") {
      if (!(in >> player->p_scale)) return false;
    } else if (token == "PLAYERVELOCITY") {
      if (!ReadVector3(in, player->velocity)) return false;
    } else if (token == "PLAYERLOOK") {
      float pitch = 0.0f;
      float yaw = 0.0f;
      if (!(in >> pitch >> yaw)) return false;
      player->SetLookAngles(pitch, yaw);
      break;
    } else {
      return false;
    }
  }

  return true;
}

}

bool SaveSceneFile(const char* filename, const World& world, const Player* player) {
  std::ofstream out(filename);
  if (!out) {
    return false;
  }

  out << std::setprecision(9);
  out << kSceneMagic << '\n';
  out << "AMBIENT " << world.ambientScale << '\n';
  out << "TIME " << world.time_of_day << '\n';
  out << "SKY " << (world.sky ? 1 : 0) << '\n';
  out << "LIGHT1 "; WriteVector3(out, world.lightPos); out << ' '; WriteVector3(out, world.lightColor); out << '\n';
  out << "LIGHT2 "; WriteVector3(out, world.lightPos2); out << ' '; WriteVector3(out, world.lightColor2); out << '\n';

  size_t savedObjectCount = 0;
  for (size_t i = 0; i < world.objects.size(); ++i) {
    if (player && world.objects[i].get() == player) {
      continue;
    }
    ++savedObjectCount;
  }

  out << "OBJECTS " << savedObjectCount << '\n';
  for (size_t i = 0; i < world.objects.size(); ++i) {
    if (player && world.objects[i].get() == player) {
      continue;
    }
    WriteObject(out, *world.objects[i]);
  }

  out << "PORTALS " << world.portals.size() << '\n';
  for (size_t i = 0; i < world.portals.size(); ++i) {
    WritePortal(out, *world.portals[i]);
  }

  WritePlayer(out, player);
  out << "ENDSCENE\n";
  return true;
}

bool LoadSceneFile(const char* filename, World& world, Player* player) {
  std::ifstream in(filename);
  if (!in) {
    return false;
  }

  std::string token;
  if (!(in >> token) || token != kSceneMagic) {
    return false;
  }

  world.Clear();
  if (player) {
    player->Reset();
  }

  while (in >> token) {
    if (token == "ENDSCENE") {
      return true;
    }
    if (token == "AMBIENT") {
      if (!(in >> world.ambientScale)) return false;
    } else if (token == "TIME") {
      if (!(in >> world.time_of_day)) return false;
    } else if (token == "SKY") {
      int hasSky = 0;
      if (!(in >> hasSky)) return false;
      if (hasSky) {
        world.sky = std::make_shared<Sky>("sky");
      }
    } else if (token == "LIGHT1") {
      if (!ReadVector3(in, world.lightPos) || !ReadVector3(in, world.lightColor)) return false;
    } else if (token == "LIGHT2") {
      if (!ReadVector3(in, world.lightPos2) || !ReadVector3(in, world.lightColor2)) return false;
    } else if (token == "OBJECTS") {
      size_t count = 0;
      if (!(in >> count)) return false;
      for (size_t i = 0; i < count; ++i) {
        std::string marker;
        if (!(in >> marker) || marker != "OBJECT") {
          return false;
        }
        if (!ReadObject(in, world)) {
          return false;
        }
      }
    } else if (token == "PORTALS") {
      size_t count = 0;
      if (!(in >> count)) return false;
      for (size_t i = 0; i < count; ++i) {
        std::string marker;
        if (!(in >> marker) || marker != "PORTAL") {
          return false;
        }
        if (!ReadPortal(in, world)) {
          return false;
        }
      }
    } else if (token == "PLAYER") {
      int hasPlayer = 0;
      if (!(in >> hasPlayer)) return false;
      if (!ReadPlayer(in, player, hasPlayer)) {
        return false;
      }
    } else {
      return false;
    }
  }

  return true;
}
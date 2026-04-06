#include "Mesh.h"
#include "Vector.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "stb_image.h"

Mesh::Mesh() : is3DTex(false) {
}

Mesh::Mesh(const char* fname) : is3DTex(false) {
  sourceName = fname ? fname : "";
  std::string nameStr(sourceName);
  bool isObj = (nameStr.size() >= 4 &&
                nameStr.compare(nameStr.size() - 4, 4, ".obj") == 0);

  if (!isObj) {
    // Use Assimp for non-obj formats (GLB, FBX, etc.)
    std::string fullPath = std::string("Meshes/") + sourceName;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(fullPath,
      aiProcess_Triangulate | aiProcess_FlipUVs |
      aiProcess_JoinIdenticalVertices);
    // Note: aiProcess_PreTransformVertices is NOT used — we walk the node tree
    // manually so we can detect portal nodes by their Blender object name.

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
      return;
    }

    // Extract embedded textures from materials
    std::unordered_map<unsigned int, uint32_t> materialToTexIdx;
    for (unsigned int mi = 0; mi < scene->mNumMaterials; ++mi) {
      aiMaterial* mat = scene->mMaterials[mi];
      aiString texPath;
      bool found = (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS);
      if (!found) {
        found = (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS);
      }
      if (!found) continue;

      const aiTexture* aiTex = scene->GetEmbeddedTexture(texPath.C_Str());
      if (!aiTex) continue;

      std::shared_ptr<Texture> tex;
      if (aiTex->mHeight == 0) {
        int w, h, channels;
        unsigned char* data = stbi_load_from_memory(
          reinterpret_cast<const unsigned char*>(aiTex->pcData),
          aiTex->mWidth, &w, &h, &channels, 4);
        if (data) {
          tex = std::make_shared<Texture>(w, h, data);
          stbi_image_free(data);
        }
      } else {
        int w = aiTex->mWidth, h = aiTex->mHeight;
        std::vector<uint8_t> rgba(w * h * 4);
        for (int pi = 0; pi < w * h; pi++) {
          rgba[pi * 4 + 0] = aiTex->pcData[pi].r;
          rgba[pi * 4 + 1] = aiTex->pcData[pi].g;
          rgba[pi * 4 + 2] = aiTex->pcData[pi].b;
          rgba[pi * 4 + 3] = aiTex->pcData[pi].a;
        }
        tex = std::make_shared<Texture>(w, h, rgba.data());
      }
      if (tex) {
        materialToTexIdx[mi] = (uint32_t)embeddedTextures.size();
        embeddedTextures.push_back(tex);
      }
    }

    // Walk the node tree iteratively, applying transforms manually.
    // Nodes named "Portal {id}" become portal markers; their geometry is excluded.
    struct NodeEntry {
      const aiNode* node;
      Matrix4 worldTransform;
    };
    auto aiToMat4 = [](const aiMatrix4x4& ai) -> Matrix4 {
      Matrix4 m;
      const float* p = &ai.a1;
      for (int i = 0; i < 16; i++)
        m.m[i] = p[i];
      return m;
    };

    std::vector<NodeEntry> nodeStack;
    {
      NodeEntry rootEntry = {scene->mRootNode, aiToMat4(scene->mRootNode->mTransformation)};
      nodeStack.push_back(rootEntry);
    }

    while (!nodeStack.empty()) {
      NodeEntry entry = nodeStack.back();
      nodeStack.pop_back();
      const aiNode* node = entry.node;
      const Matrix4& world = entry.worldTransform;

      // Push children with accumulated transforms
      for (unsigned int ci = 0; ci < node->mNumChildren; ci++) {
        const aiNode* child = node->mChildren[ci];
        NodeEntry childEntry = {child, world * aiToMat4(child->mTransformation)};
        nodeStack.push_back(childEntry);
      }

      // Check if this node is a portal marker
      std::string nodeName(node->mName.C_Str());
      if (nodeName.compare(0, 7, "Portal ") == 0 && node->mNumMeshes > 0) {
        std::string idPart = nodeName.substr(7);
        std::string::size_type dotPos = idPart.find('.');
        if (dotPos != std::string::npos) idPart = idPart.substr(0, dotPos);
        int portalId = 0;
        try { portalId = std::stoi(idPart); } catch (...) { continue; }

        // Gather all world-space triangle vertices from this node's meshes
        PortalMarker pm;
        pm.id = portalId;
        for (unsigned int mi = 0; mi < node->mNumMeshes; mi++) {
          aiMesh* pmesh = scene->mMeshes[node->mMeshes[mi]];
          for (unsigned int fi = 0; fi < pmesh->mNumFaces; ++fi) {
            aiFace& face = pmesh->mFaces[fi];
            if (face.mNumIndices != 3) continue;
            for (unsigned int ki = 0; ki < 3; ++ki) {
              unsigned int idx = face.mIndices[ki];
              Vector3 lp(pmesh->mVertices[idx].x, pmesh->mVertices[idx].y, pmesh->mVertices[idx].z);
              pm.wsVerts.push_back(world.MulPoint(lp));
            }
          }
        }
        if (pm.wsVerts.size() < 3) continue;

        // Centroid
        Vector3 center(0, 0, 0);
        for (size_t vi = 0; vi < pm.wsVerts.size(); ++vi) center = center + pm.wsVerts[vi];
        pm.center = center * (1.0f / (float)pm.wsVerts.size());

        // Normal from first triangle
        Vector3 edge1 = pm.wsVerts[1] - pm.wsVerts[0];
        Vector3 edge2 = pm.wsVerts[2] - pm.wsVerts[0];
        pm.forward = edge1.Cross(edge2).Normalized();

        portalMarkers.push_back(pm);
        continue; // skip portal geometry
      }

      // Check if this node is a light marker (name starts with "light", case-insensitive)
      bool isLightNode = false;
      {
        std::string lower = nodeName;
        for (size_t ci = 0; ci < lower.size(); ++ci) lower[ci] = (char)tolower(lower[ci]);
        isLightNode = (lower.compare(0, 5, "light") == 0) && node->mNumMeshes > 0;
      }

      // Choose target arrays: light nodes go into a separate mesh,
      // everything else goes into the main arrays.
      std::vector<float>* dstVerts   = &verts;
      std::vector<float>* dstUvs     = &uvs;
      std::vector<float>* dstNormals = &normals;
      std::vector<float>* dstColors  = &colors;
      std::vector<uint32_t>* dstTexIndices = &texIndices;

      std::shared_ptr<Mesh> lightMesh;
      if (isLightNode) {
        lightMesh.reset(new Mesh());
        lightMesh->is3DTex = false;
        dstVerts      = &lightMesh->verts;
        dstUvs        = &lightMesh->uvs;
        dstNormals    = &lightMesh->normals;
        dstColors     = &lightMesh->colors;
        dstTexIndices = &lightMesh->texIndices;
      }

      size_t nodeVertStart = verts.size() / 3;
      for (unsigned int mi = 0; mi < node->mNumMeshes; mi++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[mi]];

        if (mesh->mTextureCoords[0] && mesh->mNumUVComponents[0] == 3) {
          is3DTex = true;
          if (lightMesh) lightMesh->is3DTex = true;
        }

        float matR = 1.0f, matG = 1.0f, matB = 1.0f;
        if (scene->mNumMaterials > mesh->mMaterialIndex) {
          aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
          aiColor4D diffuse;
          if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
            matR = diffuse.r; matG = diffuse.g; matB = diffuse.b;
          }
          aiColor4D baseColor;
          if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
            matR = baseColor.r; matG = baseColor.g; matB = baseColor.b;
          }
        }

        uint32_t subMeshTexIdx = UINT32_MAX;
        {
          auto texIt = materialToTexIdx.find(mesh->mMaterialIndex);
          if (texIt != materialToTexIdx.end()) {
            subMeshTexIdx = texIt->second;
          }
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
          aiFace face = mesh->mFaces[j];
          if (face.mNumIndices != 3) continue;

          for (unsigned int k = 0; k < 3; ++k) {
            unsigned int idx = face.mIndices[k];

            // Transform position to world space via node transform
            Vector3 lp(mesh->mVertices[idx].x, mesh->mVertices[idx].y, mesh->mVertices[idx].z);
            Vector3 wp = world.MulPoint(lp);
            dstVerts->push_back(wp.x);
            dstVerts->push_back(wp.y);
            dstVerts->push_back(wp.z);

            if (mesh->mTextureCoords[0]) {
              dstUvs->push_back(mesh->mTextureCoords[0][idx].x);
              dstUvs->push_back(mesh->mTextureCoords[0][idx].y);
              if (is3DTex) {
                dstUvs->push_back(mesh->mTextureCoords[0][idx].z);
              }
            } else {
              dstUvs->push_back(0.0f);
              dstUvs->push_back(0.0f);
              if (is3DTex) {
                dstUvs->push_back(0.0f);
              }
            }

            if (mesh->HasNormals()) {
              Vector3 ln(mesh->mNormals[idx].x, mesh->mNormals[idx].y, mesh->mNormals[idx].z);
              Vector3 wn = world.MulDirection(ln).NormalizedSafe();
              dstNormals->push_back(wn.x);
              dstNormals->push_back(wn.y);
              dstNormals->push_back(wn.z);
            } else {
              dstNormals->push_back(0.0f);
              dstNormals->push_back(1.0f);
              dstNormals->push_back(0.0f);
            }

            if (mesh->HasVertexColors(0)) {
              dstColors->push_back(mesh->mColors[0][idx].r);
              dstColors->push_back(mesh->mColors[0][idx].g);
              dstColors->push_back(mesh->mColors[0][idx].b);
            } else {
              dstColors->push_back(matR);
              dstColors->push_back(matG);
              dstColors->push_back(matB);
            }

            dstTexIndices->push_back(subMeshTexIdx);
          }
        }
      }

      if (isLightNode && lightMesh && !lightMesh->verts.empty()) {
        // Build LightMarker from the separated mesh data
        LightMarker lm;
        lm.name = nodeName;
        lm.mesh = lightMesh;
        size_t numVerts = lightMesh->verts.size() / 3;
        Vector3 center(0, 0, 0);
        for (size_t vi = 0; vi < numVerts; ++vi) {
          center = center + Vector3(lightMesh->verts[vi*3], lightMesh->verts[vi*3+1], lightMesh->verts[vi*3+2]);
        }
        lm.center = center * (1.0f / (float)numVerts);
        // Compute bounding sphere radius from center
        float maxR2 = 0.0f;
        for (size_t vi = 0; vi < numVerts; ++vi) {
          Vector3 v(lightMesh->verts[vi*3], lightMesh->verts[vi*3+1], lightMesh->verts[vi*3+2]);
          float r2 = (v - lm.center).MagSq();
          if (r2 > maxR2) maxR2 = r2;
        }
        lm.radius = sqrtf(maxR2);
        lm.color = Vector3(lightMesh->colors[0], lightMesh->colors[1], lightMesh->colors[2]);
        lightMarkers.push_back(lm);
      } else {
        size_t nodeVertEnd = verts.size() / 3;
        if (nodeVertEnd > nodeVertStart) {
          nodeVertexRanges[nodeName] = std::make_pair(nodeVertStart, nodeVertEnd);
        }
      }
    }

    // Rebuild flat (per-face) normals so low-poly meshes look faceted
    // instead of overly smooth.  Matches the OBJ code-path behaviour.
    RebuildFlatNormals();

    // Build colliders from all faces, then accelerate with BVH
    BuildCollidersFromMesh();
    bvh.Build(colliders);
    return;
  }

  //Open the file for reading
  std::ifstream fin(std::string("Meshes/") + fname);
  if (!fin) {
    return;
  }

  //Temporaries
  std::vector<float> vert_palette;
  std::vector<float> uv_palette;

  //Read the file
  std::string line;
  while (!fin.eof()) {
    std::getline(fin, line);
    if (line.find("v ") == 0) {
      std::stringstream ss(line.c_str() + 2);
      float x, y, z;
      ss >> x >> y >> z;
      vert_palette.push_back(x);
      vert_palette.push_back(y);
      vert_palette.push_back(z);
    } else if (line.find("vt ") == 0) {
      std::stringstream ss(line.c_str() + 3);
      float u, v, w;
      ss >> u >> v >> w;
      uv_palette.push_back(u);
      uv_palette.push_back(v);
      if (!ss.fail()) {
        uv_palette.push_back(w);
        is3DTex = true;
      }
    } else if (line.find("c ") == 0) {
      uint32_t a = 0, b = 0, c = 0;
      if (line[2] == '*') {
        const uint32_t v_ix = (uint32_t)vert_palette.size() / 3;
        a = v_ix - 2; b = v_ix - 1; c = v_ix;
      } else {
        std::stringstream ss(line.c_str() + 2);
        ss >> a >> b >> c;
      }
      const Vector3 v1(&vert_palette[(a - 1) * 3]);
      const Vector3 v2(&vert_palette[(b - 1) * 3]);
      const Vector3 v3(&vert_palette[(c - 1) * 3]);
      colliders.push_back(Collider(v1, v2, v3));
    } else if (line.find("f ") == 0) {
      //Count the slashes
      int num_slashes = 0;
      size_t last_slash_ix = 0;
      bool doubleslash = false;
      for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '/') {
          line[i] = ' ';
          if (last_slash_ix == i - 1) {
            assert(vert_palette.size() == uv_palette.size() || uv_palette.empty());
            doubleslash = true;
          }
          last_slash_ix = i;
          num_slashes++;
        }
      }
      uint32_t a=0, b=0, c=0, d=0;
      uint32_t at=0, bt=0, ct=0, dt=0;
      uint32_t _tmp;
      std::stringstream ss(line.c_str() + 2);
      const bool wild = (line[2] == '*');
      const bool wild2 = (line[3] == '*');
      bool isQuad = false;

      //Interpret face based on slash
      if (wild) {
        assert(num_slashes == 0);
        const uint32_t v_ix = (uint32_t)vert_palette.size() / 3;
        const uint32_t t_ix = (uint32_t)uv_palette.size() / (is3DTex ? 3 : 2);
        if (wild2) {
          a = v_ix - 3; b = v_ix - 2; c = v_ix - 1; d = v_ix - 0;
          at = t_ix - 3; bt = t_ix - 2; ct = t_ix - 1; dt = t_ix - 0;
          isQuad = true;
        } else {
          a = v_ix - 2; b = v_ix - 1; c = v_ix;
          at = t_ix - 2; bt = t_ix - 1; ct = t_ix;
        }
      } else if (num_slashes == 0) {
        ss >> a >> b >> c >> d;
        at = a; bt = b; ct = c; dt = d;
        if (!ss.fail()) {
          isQuad = true;
        }
      } else if (num_slashes == 3) {
        ss >> a >> at >> b >> bt >> c >> ct;
      } else if (num_slashes == 4) {
        isQuad = true;
        ss >> a >> at >> b >> bt >> c >> ct >> d >> dt;
      } else if (num_slashes == 6) {
        if (doubleslash) {
          ss >> a >> _tmp >> b >> _tmp >> c >> _tmp;
          at = a; bt = b; ct = c;
        } else {
          ss >> a >> at >> _tmp >> b >> bt >> _tmp >> c >> ct >> _tmp;
        }
      } else if (num_slashes == 8) {
        isQuad = true;
        if (doubleslash) {
          ss >> a >> _tmp >> b >> _tmp >> c >> _tmp >> d >> _tmp;
          at = a; bt = b; ct = c; dt = d;
        } else {
          ss >> a >> at >> _tmp >> b >> bt >> _tmp >> c >> ct >> _tmp >> d >> dt >> _tmp;
        }
      } else {
        assert(false);
        continue;
      }

      //Add face to list
      AddFace(vert_palette, uv_palette, a, at, b, bt, c, ct, is3DTex);
      if (isQuad) {
        AddFace(vert_palette, uv_palette, c, ct, d, dt, a, at, is3DTex);
      }
    }
  }
  // CPU-only: no GL upload
}

Mesh::Mesh(const std::vector<float>& inVerts, const std::vector<float>& inUVs, bool inIs3DTex)
  : verts(inVerts), uvs(inUVs), is3DTex(inIs3DTex) {
  const size_t vertexCount = verts.size() / 3;
  const size_t uvStride = is3DTex ? 3 : 2;

  if (uvs.empty()) {
    uvs.resize(vertexCount * uvStride, 0.0f);
  }

  assert(verts.size() % 9 == 0);
  assert(uvs.size() == vertexCount * uvStride);
  RebuildFlatNormals();
}

Mesh::~Mesh() {
}

void Mesh::BuildCollidersFromMesh() {
  colliders.clear();
  for (size_t i = 0; i + 8 < verts.size(); i += 9) {
    const Vector3 v1(&verts[i]);
    const Vector3 v2(&verts[i + 3]);
    const Vector3 v3(&verts[i + 6]);
    colliders.push_back(Collider(v1, v2, v3));
  }
}

void Mesh::RebuildFlatNormals() {
  normals.clear();
  normals.reserve(verts.size());

  for (size_t i = 0; i + 8 < verts.size(); i += 9) {
    const Vector3 v1(&verts[i]);
    const Vector3 v2(&verts[i + 3]);
    const Vector3 v3(&verts[i + 6]);
    Vector3 normal = (v2 - v1).Cross(v3 - v1).NormalizedSafe();
    for (int vertex = 0; vertex < 3; ++vertex) {
      normals.push_back(normal.x);
      normals.push_back(normal.y);
      normals.push_back(normal.z);
    }
  }
}

void Mesh::AddFace(
  const std::vector<float>& vert_palette, const std::vector<float>& uv_palette,
  uint32_t a, uint32_t at, uint32_t b, uint32_t bt, uint32_t c, uint32_t ct, bool faceIs3DTex)
{
  //Merge texture and vertex indicies
  assert(a > 0 && b > 0 && c > 0);
  assert(at > 0 && bt > 0 && ct > 0);
  a -= 1; b -= 1; c -= 1;
  at -= 1; bt -= 1; ct -= 1;
  const uint32_t v_ix[3] = { a, b, c };
  const uint32_t uv_ix[3] = { at, bt, ct };

  //Calcuate the normal for this face
  const Vector3 v1(&vert_palette[a * 3]);
  const Vector3 v2(&vert_palette[b * 3]);
  const Vector3 v3(&vert_palette[c * 3]);
  const Vector3 normal = (v2 - v1).Cross(v3 - v1).Normalized();

  for (int i = 0; i < 3; ++i) {
    const uint32_t v = v_ix[i];
    const uint32_t vt = uv_ix[i];
    assert(v < vert_palette.size() / 3);
    verts.push_back(vert_palette[v * 3]);
    verts.push_back(vert_palette[v * 3 + 1]);
    verts.push_back(vert_palette[v * 3 + 2]);
    if (!uv_palette.empty()) {
      if (faceIs3DTex) {
        assert(vt < uv_palette.size() / 3);
        uvs.push_back(uv_palette[vt * 3]);
        uvs.push_back(uv_palette[vt * 3 + 1]);
        uvs.push_back(uv_palette[vt * 3 + 2]);
      } else {
        assert(vt < uv_palette.size() / 2);
        uvs.push_back(uv_palette[vt * 2]);
        uvs.push_back(uv_palette[vt * 2 + 1]);
      }
    } else {
      uvs.push_back(0.0f);
      uvs.push_back(0.0f);
    }
    normals.push_back(normal.x);
    normals.push_back(normal.y);
    normals.push_back(normal.z);
  }
}

std::shared_ptr<Mesh> Mesh::ExtractSubMesh(size_t startVert, size_t endVert) const {
  std::shared_ptr<Mesh> sub(new Mesh());
  if (endVert <= startVert) return sub;
  int uvStride = is3DTex ? 3 : 2;

  sub->verts.assign(verts.begin() + startVert * 3, verts.begin() + endVert * 3);
  if (normals.size() >= endVert * 3) {
    sub->normals.assign(normals.begin() + startVert * 3, normals.begin() + endVert * 3);
  }
  if (uvs.size() >= endVert * uvStride) {
    sub->uvs.assign(uvs.begin() + startVert * uvStride, uvs.begin() + endVert * uvStride);
  }
  if (colors.size() >= endVert * 3) {
    sub->colors.assign(colors.begin() + startVert * 3, colors.begin() + endVert * 3);
  }
  sub->is3DTex = is3DTex;
  return sub;
}

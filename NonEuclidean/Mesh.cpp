#include "Mesh.h"
#include "Vector.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>

Mesh::Mesh(const char* fname) {
  //Open the file for reading
  std::ifstream fin(std::string("Meshes/") + fname);
  if (!fin) {
    return;
  }

  //Temporaries
  std::vector<float> vert_palette;
  std::vector<float> uv_palette;
  bool is3DTex = false;

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

  //Setup GL
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(NUM_VBOS, vbo);
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(verts[0]), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  }
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(uvs[0]), uvs.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, (is3DTex ? 3 : 2), GL_FLOAT, GL_FALSE, 0, 0);
  }
  {
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(normals[0]), normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
  }
}

Mesh::~Mesh() {
  glDeleteBuffers(NUM_VBOS, vbo);
  glDeleteVertexArrays(1, &vao);
}

void Mesh::Draw() {
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
}

void Mesh::DebugDraw(const Camera& cam, const Matrix4& objMat) {
  for (size_t i = 0; i < colliders.size(); ++i) {
    colliders[i].DebugDraw(cam, objMat);
  }
}

void Mesh::AddFace(
  const std::vector<float>& vert_palette, const std::vector<float>& uv_palette,
  uint32_t a, uint32_t at, uint32_t b, uint32_t bt, uint32_t c, uint32_t ct, bool is3DTex)
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
      if (is3DTex) {
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

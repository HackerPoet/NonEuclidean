#include "Resources.h"
#include <unordered_map>

std::shared_ptr<Mesh> AquireMesh(const char* name) {
  static std::unordered_map<std::string, std::weak_ptr<Mesh>> map;
  std::weak_ptr<Mesh>& mesh = map[std::string(name)];
  if (mesh.expired()) {
    std::shared_ptr<Mesh> newMesh(new Mesh(name));
    mesh = newMesh;
    return newMesh;
  } else {
    return mesh.lock();
  }
}

std::shared_ptr<Shader> AquireShader(const char* name) {
  static std::unordered_map<std::string, std::weak_ptr<Shader>> map;
  std::weak_ptr<Shader>& shader = map[std::string(name)];
  if (shader.expired()) {
    std::shared_ptr<Shader> newShader(new Shader(name));
    shader = newShader;
    return newShader;
  } else {
    return shader.lock();
  }
}

std::shared_ptr<Texture> AquireTexture(const char* name, int rows, int cols) {
  static std::unordered_map<std::string, std::weak_ptr<Texture>> map;
  std::weak_ptr<Texture>& tex = map[std::string(name)];
  if (tex.expired()) {
    std::shared_ptr<Texture> newTex(new Texture(name, rows, cols));
    tex = newTex;
    return newTex;
  } else {
    return tex.lock();
  }
}

#include <windows.h>
#include "Meshes/Meshes.h"
#include <fstream>

int LoadMeshes()
{
    if (CreateDirectory("Meshes", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        FILE* file;

        std::list<std::string> FileName = {
            "bunny",
            "double_quad",
            "floorplan",
            "ground",
            "ground_slope",
            "pillar",
            "pillar_room",
            "quad",
            "square_rooms",
            "suzanne",
            "teapot",
            "tunnel",
            "tunnel_scale",
            "tunnel_slope",
        };

        std::list<int> charSize = {
            205917,
            120,
            24564,
            96,
            287,
            30910,
            1913,
            102,
            5479,
            81336,
            162511,
            1445,
            1278,
            1379,
        };

        for (int i = 0; i < 14; i++)
        {
            auto name = std::next(FileName.begin(), i);
            std::string path = "./Meshes/";
            path += *name;
            path += ".obj";
            
            auto Buffer = bunny_obj;
            if (i == 0)
                Buffer = bunny_obj;
            else if (i == 1)
                Buffer = double_quad_obj;
            else if (i == 2)
                Buffer = floorplan_obj;
            else if (i == 3)
                Buffer = ground_obj;
            else if (i == 4)
                Buffer = ground_slope_obj;
            else if (i == 5)
                Buffer = pillar_obj;
            else if (i == 6)
                Buffer = pillar_room_obj;
            else if (i == 7)
                Buffer = quad_obj;
            else if (i == 8)
                Buffer = square_rooms_obj;
            else if (i == 9)
                Buffer = suzanne_obj;
            else if (i == 10)
                Buffer = teapot_obj;
            else if (i == 11)
                Buffer = tunnel_obj;
            else if (i == 12)
                Buffer = tunnel_scale_obj;
            else if (i == 13)
                Buffer = tunnel_slope_obj;

            auto size = std::next(charSize.begin(), i);

            std::fstream fileStream;
            fileStream.open(path);
            if (fileStream.fail())
                if (fopen_s(&file, path.c_str(), "wb") == 0)
                    if (fwrite(Buffer, 1, *size, file))
                        fclose(file);
                    else
                        return 3;
                else
                    return 2;
            else
                fileStream.close();
        }
        return 0;
    }
    else
    {
        return 1;
    }
}
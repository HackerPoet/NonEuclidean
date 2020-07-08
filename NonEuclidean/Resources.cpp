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
#include <fstream>
#include "Meshes/Meshes.h"
#include "Shaders/Shaders.h"
#include "Textures/Textures.h"

int LoadMeshes()
{
    if (CreateDirectory("Meshes", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        FILE* file;

        std::list<std::string> FileName = {
            "./Meshes/bunny.obj",
            "./Meshes/double_quad.obj",
            "./Meshes/floorplan.obj",
            "./Meshes/ground.obj",
            "./Meshes/ground_slope.obj",
            "./Meshes/pillar.obj",
            "./Meshes/pillar_room.obj",
            "./Meshes/quad.obj",
            "./Meshes/square_rooms.obj",
            "./Meshes/suzanne.obj",
            "./Meshes/teapot.obj",
            "./Meshes/tunnel.obj",
            "./Meshes/tunnel_scale.obj",
            "./Meshes/tunnel_slope.obj",
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
            std::string path = *name;
            
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

int LoadShaders()
{
    if (CreateDirectory("Shaders", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        FILE* file;

        std::list<std::string> FileName = {
            "./Shaders/pink.frag",
            "./Shaders/pink.vert",
            "./Shaders/portal.frag",
            "./Shaders/portal.vert",
            "./Shaders/sky.frag",
            "./Shaders/sky.vert",
            "./Shaders/texture.frag",
            "./Shaders/texture.vert",
            "./Shaders/texture_array.frag",
            "./Shaders/texture_array.vert",
        };

        std::list<int> charSize = {
            178,
            156,
            258,
            209,
            496,
            325,
            313,
            320,
            320,
            320,
        };

        for (int i = 0; i < 10; i++)
        {
            auto name = std::next(FileName.begin(), i);
            std::string path = *name;            

            auto Buffer = pink_frag;
            if (i == 0)
                Buffer = pink_frag;
            else if (i == 1)
                Buffer = pink_vert;
            else if (i == 2)
                Buffer = portal_frag;
            else if (i == 3)
                Buffer = portal_vert;
            else if (i == 4)
                Buffer = sky_frag;
            else if (i == 5)
                Buffer = sky_vert;
            else if (i == 6)
                Buffer = texture_frag;
            else if (i == 7)
                Buffer = texture_vert;
            else if (i == 8)
                Buffer = texture_array_frag;
            else if (i == 9)
                Buffer = texture_array_vert;

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

int LoadTextures()
{
    if (CreateDirectory("Textures", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        FILE* file;

        std::list<std::string> FileName = {
            "./Textures/checker_gray.bmp",
            "./Textures/checker_green.bmp",
            "./Textures/floorplan_textures.bmp",
            "./Textures/gold.bmp",
            "./Textures/three_room.bmp",
            "./Textures/three_room2.bmp",
            "./Textures/white.bmp",
        };

        std::list<int> charSize = {
            102,
            102,
            786486,
            58,
            3126,
            3126,
            58,
        };

        for (int i = 0; i < 7; i++)
        {
            auto name = std::next(FileName.begin(), i);
            std::string path = *name;

            auto Buffer = checker_gray_bmp;
            if (i == 0)
                Buffer = checker_gray_bmp;
            else if (i == 1)
                Buffer = checker_green_bmp;
            else if (i == 2)
                Buffer = floorplan_textures_bmp;
            else if (i == 3)
                Buffer = gold_bmp;
            else if (i == 4)
                Buffer = three_room_bmp;
            else if (i == 5)
                Buffer = three_room2_bmp;
            else if (i == 6)
                Buffer = white_bmp;

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
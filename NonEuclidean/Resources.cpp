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

FILE* file;

int LoadMeshes()
{
    #ifdef _DEBUG
    puts("Creating \"Meshes\" directory");
    #endif

    if (CreateDirectory("Meshes", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        #ifdef _DEBUG
        if (ERROR_ALREADY_EXISTS == GetLastError())
            puts("\"Meshes\" directory, already exists");
        else
            puts("Created \"Meshes\" directory");
        #endif

        for (int i = 0; i < 14; i++)
        {
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

            std::string path = *std::next(MeshName.begin(), i);
            size_t size = *std::next(MeshSize.begin(), i);

            std::fstream fileStream;
            fileStream.open(path);
            if (fileStream.fail())
                if (fopen_s(&file, path.c_str(), "wb") == 0)
                    if (fwrite(Buffer, 1, size, file))
                    {
                        fclose(file);

                        #ifdef _DEBUG
                        std::cout << path << ", writed" << std::endl;
                        #endif
                    }
                    else
                    {
                        #ifdef _DEBUG
                        std::cout << "Can't write: " << path << std::endl;
                        #endif

                        return 3;
                    }
                else
                {
                    #ifdef _DEBUG
                    std::cout << "Can't open to write: " << path << std::endl;
                    #endif

                    return 2;
                }
            else
            {
                fileStream.close();

                #ifdef _DEBUG
                std::cout << path << ", exists" << std::endl;
                #endif
            }
        }
        #ifdef _DEBUG
        puts("Meshes loaded\n");
        #endif
        return 0;
    }
    else
    {
        #ifdef _DEBUG
        puts("Can't create \"Meshes\" directory");
        #endif

        return 1;
    }
}

int LoadShaders()
{
    #ifdef _DEBUG
    puts("Creating \"Shaders\" directory");
    #endif

    if (CreateDirectory("Shaders", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        #ifdef _DEBUG
        if (ERROR_ALREADY_EXISTS == GetLastError())
            puts("\"Shaders\" directory, already exists");
        else
            puts("Created \"Shaders\" directory");
        #endif

        for (int i = 0; i < 10; i++)
        {
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

            std::string path = *std::next(ShadName.begin(), i);
            size_t size = *std::next(ShadSize.begin(), i);

            std::fstream fileStream;
            fileStream.open(path);
            if (fileStream.fail())
                if (fopen_s(&file, path.c_str(), "wb") == 0)
                    if (fwrite(Buffer, 1, size, file))
                    {
                        fclose(file);

                        #ifdef _DEBUG
                        std::cout << path << ", writed" << std::endl;
                        #endif
                    }
                    else
                    {
                        #ifdef _DEBUG
                        std::cout << "Can't write: " << path << std::endl;
                        #endif

                        return 3;
                    }
                else
                {
                    #ifdef _DEBUG
                    std::cout << "Can't open to write: " << path << std::endl;
                    #endif

                    return 2;
                }
            else
            {
                fileStream.close();

                #ifdef _DEBUG
                std::cout << path << ", exists" << std::endl;
                #endif
            }
        }
        #ifdef _DEBUG
        puts("Shaders loaded\n");
        #endif
        return 0;
    }
    else
    {
        #ifdef _DEBUG
        puts("Can't create \"Shaders\" directory");
        #endif

        return 1;
    }
}

int LoadTextures()
{
    #ifdef _DEBUG
    puts("Creating \"Textures\" directory");
    #endif

    if (CreateDirectory("Textures", NULL) || ERROR_ALREADY_EXISTS == GetLastError())
    {
        #ifdef _DEBUG
        if (ERROR_ALREADY_EXISTS == GetLastError())
            puts("\"Textures\" directory, already exists");
        else
            puts("Created \"Textures\" directory");
        #endif

        for (int i = 0; i < 7; i++)
        {
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

            std::string path = *std::next(TexturesName.begin(), i);
            size_t size = *std::next(TexturesSize.begin(), i);

            std::fstream fileStream;
            fileStream.open(path);
            if (fileStream.fail())
                if (fopen_s(&file, path.c_str(), "wb") == 0)
                    if (fwrite(Buffer, 1, size, file))
                    {
                        fclose(file);

                        #ifdef _DEBUG
                        std::cout << path << ", writed" << std::endl;
                        #endif
                    }
                    else
                    {
                        #ifdef _DEBUG
                        std::cout << "Can't write: " << path << std::endl;
                        #endif

                        return 3;
                    }
                else
                {
                    #ifdef _DEBUG
                    std::cout << "Can't open to write: " << path << std::endl;
                    #endif

                    return 2;
                }
            else
            {
                fileStream.close();

                #ifdef _DEBUG
                std::cout << path << ", exists" << std::endl;
                #endif
            }
        }
        #ifdef _DEBUG
        puts("Textures loaded\n");
        #endif
        return 0;
    }
    else
    {
        #ifdef _DEBUG
        puts("Can't create \"Textures\" directory");
        #endif

        return 1;
    }
}
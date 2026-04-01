#pragma once
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include <memory>

std::shared_ptr<Mesh> AquireMesh(const char* name);
std::shared_ptr<Shader> AquireShader(const char* name);
std::shared_ptr<Texture> AquireTexture(const char* name, int rows=1, int cols=1);

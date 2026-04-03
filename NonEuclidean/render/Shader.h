#pragma once
#include <string>

class Shader {
public:
  Shader(const char* /*name*/) {}
  ~Shader() {}

  void Use() {}
  void SetMVP(const float*, const float*) {}
  void SetUniform(const char*, float) {}
  void SetUniform(const char*, float, float, float) {}
  void SetUniformMat4(const char*, const float*) {}
  void SetUniformInt(const char*, int) {}
};

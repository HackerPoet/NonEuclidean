#pragma once
#include <GL/glew.h>

class GBuffer {
public:
  enum Tex { POSITION = 0, NORMAL, ALBEDO, NUM_TEXTURES };

  GBuffer();
  ~GBuffer();

  bool Init(int width, int height);
  void Destroy();

  void BindForWriting();
  void BindForReading();     // binds textures to units 0,1,2
  void UnbindFramebuffer();

  GLuint GetFBO() const { return fbo; }

private:
  GLuint fbo;
  GLuint textures[NUM_TEXTURES];
  GLuint depthBuf;
  int width;
  int height;
};

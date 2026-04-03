#include "GBuffer.h"
#include <cstring>

GBuffer::GBuffer() : fbo(0), depthBuf(0), width(0), height(0) {
  memset(textures, 0, sizeof(textures));
}

GBuffer::~GBuffer() {
  Destroy();
}

bool GBuffer::Init(int w, int h) {
  Destroy();
  width = w;
  height = h;

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // Create G-buffer textures
  glGenTextures(NUM_TEXTURES, textures);

  // Position (world-space, RGB16F)
  glBindTexture(GL_TEXTURE_2D, textures[POSITION]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[POSITION], 0);

  // Normal (world-space, RGB16F)
  glBindTexture(GL_TEXTURE_2D, textures[NORMAL]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textures[NORMAL], 0);

  // Albedo (RGBA8)
  glBindTexture(GL_TEXTURE_2D, textures[ALBEDO]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, textures[ALBEDO], 0);

  // Depth renderbuffer
  glGenRenderbuffers(1, &depthBuf);
  glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);

  // Set draw buffers
  GLenum drawBuffers[NUM_TEXTURES] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2
  };
  glDrawBuffers(NUM_TEXTURES, drawBuffers);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return status == GL_FRAMEBUFFER_COMPLETE;
}

void GBuffer::Destroy() {
  if (fbo) {
    glDeleteFramebuffers(1, &fbo);
    fbo = 0;
  }
  for (int i = 0; i < NUM_TEXTURES; ++i) {
    if (textures[i]) {
      glDeleteTextures(1, &textures[i]);
      textures[i] = 0;
    }
  }
  if (depthBuf) {
    glDeleteRenderbuffers(1, &depthBuf);
    depthBuf = 0;
  }
}

void GBuffer::BindForWriting() {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  GLenum drawBuffers[NUM_TEXTURES] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2
  };
  glDrawBuffers(NUM_TEXTURES, drawBuffers);
}

void GBuffer::BindForReading() {
  for (int i = 0; i < NUM_TEXTURES; ++i) {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, textures[i]);
  }
}

void GBuffer::UnbindFramebuffer() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

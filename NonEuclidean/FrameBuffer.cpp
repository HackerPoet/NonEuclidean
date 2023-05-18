#include "FrameBuffer.h"
#include "GameHeader.h"
#include "Engine.h"
#include <iostream>

FrameBuffer::FrameBuffer() {
  glGenTextures(1, &texId);
  glBindTexture(GL_TEXTURE_2D, texId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GH_FBO_SIZE, GH_FBO_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  //-------------------------
  glGenFramebuffersEXT(1, &fbo);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texId, 0);
  //-------------------------
  glGenRenderbuffersEXT(1, &renderBuf);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuf);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16, GH_FBO_SIZE, GH_FBO_SIZE);
  //-------------------------
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderBuf);
  //-------------------------

  //Does the GPU support current FBO configuration?
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
    return;
  }

  //Unbind so future rendering can proceed normally
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

FrameBuffer::~FrameBuffer()
{
  glDeleteRenderbuffersEXT(1, &renderBuf);
  glDeleteFramebuffersEXT(1, &fbo);
  glDeleteTextures(1, &texId);
}

void FrameBuffer::Use() {
  glBindTexture(GL_TEXTURE_2D, texId);
}

void FrameBuffer::Render(const Camera& cam, GLuint curFBO, const Portal* skipPortal) {
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  glViewport(0, 0, GH_FBO_SIZE, GH_FBO_SIZE);
  GH_ENGINE->Render(cam, fbo, skipPortal);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, curFBO);
}

#pragma once
#include "GameHeader.h"
#include "Object.h"
#include "FrameBuffer.h"
#include "Mesh.h"
#include "Resources.h"
#include "Shader.h"
#include <memory>

class Portal : public Object {
public:
  //Subclass that represents a warp
  struct Warp {
    Warp(const Portal* fromPortal) : fromPortal(fromPortal), toPortal(nullptr) {
      delta.MakeIdentity();
      deltaInv.MakeIdentity();
    }

    Matrix4 delta;
    Matrix4 deltaInv;
    const Portal* fromPortal;
    const Portal* toPortal;
  };

  Portal();
  virtual ~Portal() {}

  virtual void Draw(const Camera& cam, GLuint curFBO) override;
  void DrawPink(const Camera& cam);

  Vector3 GetBump(const Vector3& a) const;
  const Warp* Intersects(const Vector3& a, const Vector3& b, const Vector3& bump) const;
  float DistTo(const Vector3& pt) const;

  static void Connect(std::shared_ptr<Portal>& a, std::shared_ptr<Portal>& b);
  static void Connect(Warp& a, Warp& b);

  Warp front;
  Warp back;

private:
  std::shared_ptr<Shader> errShader;
  FrameBuffer frameBuf[GH_MAX_RECURSION <= 1 ? 1 : GH_MAX_RECURSION - 1];
};
typedef std::vector<std::shared_ptr<Portal>> PPortalVec;
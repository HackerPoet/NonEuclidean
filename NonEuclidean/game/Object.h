#pragma once
#include "GameHeader.h"
#include "Vector.h"
#include "Camera.h"
#include "Sphere.h"
#include <vector>
#include <memory>

//Forward declarations
class Physical;
class Mesh;
class Texture;
class Shader;

class Object {
public:
  Object();
  virtual ~Object() {}

  virtual void Reset();
  virtual void Draw(const Camera& cam, uint32_t curFBO);
  virtual void Update() {};
  virtual void OnHit(Object& other, Vector3& push) {};

  //Casts
  virtual Physical* AsPhysical() { return nullptr; }
  const Physical* AsPhysical() const { return const_cast<Object*>(this)->AsPhysical(); }

  void DebugDraw(const Camera& cam);

  Matrix4 LocalToWorld() const;
  Matrix4 WorldToLocal() const;
  Vector3 Forward() const;

  Vector3 pos;
  Vector3 euler;
  Vector3 scale;

  // Physical scale, only updated by portal scale changes
  float p_scale;

  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Texture> texture;
  std::shared_ptr<Shader> shader;
};
typedef std::vector<std::shared_ptr<Object>> PObjectVec;

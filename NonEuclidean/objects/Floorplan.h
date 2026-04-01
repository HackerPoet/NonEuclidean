#pragma once
#include "Object.h"
#include "Resources.h"
#include "Portal.h"

class Floorplan : public Object {
public:
  Floorplan() {
    mesh = AquireMesh("floorplan.obj");
    shader = AquireShader("texture_array");
    texture = AquireTexture("floorplan_textures.bmp", 4, 4);
    scale = Vector3(0.1524f); //6-inches to meters
  }
  virtual ~Floorplan() {}

  void AddPortals(PPortalVec& pvec) const {
    std::shared_ptr<Portal> p1(new Portal);
    std::shared_ptr<Portal> p2(new Portal);
    std::shared_ptr<Portal> p3(new Portal);
    std::shared_ptr<Portal> p4(new Portal);
    std::shared_ptr<Portal> p5(new Portal);
    std::shared_ptr<Portal> p6(new Portal);

    p1->pos = Vector3(33, 10, 25.5f) * scale;
    p1->scale = Vector3(4, 10, 1) * scale;

    p2->pos = Vector3(74, 10, 25.5f) * scale;
    p2->scale = Vector3(4, 10, 1) * scale;

    p3->pos = Vector3(33, 10, 66.5f) * scale;
    p3->scale = Vector3(4, 10, 1) * scale;

    p4->pos = Vector3(63.5f, 10, 48) * scale;
    p4->scale = Vector3(4, 10, 1) * scale;
    p4->euler.y = GH_PI/2;

    p5->pos = Vector3(63.5f, 10, 7) * scale;
    p5->scale = Vector3(4, 10, 1) * scale;
    p5->euler.y = GH_PI / 2;

    p6->pos = Vector3(22.5f, 10, 48) * scale;
    p6->scale = Vector3(4, 10, 1) * scale;
    p6->euler.y = GH_PI / 2;

    Portal::Connect(p1->front, p3->back);
    Portal::Connect(p1->back, p2->front);
    Portal::Connect(p3->front, p2->back);

    Portal::Connect(p4->front, p6->back);
    Portal::Connect(p4->back, p5->front);
    Portal::Connect(p6->front, p5->back);

    pvec.push_back(p1);
    pvec.push_back(p2);
    pvec.push_back(p3);
    pvec.push_back(p4);
    pvec.push_back(p5);
    pvec.push_back(p6);
  }
};

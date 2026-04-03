#include "Level1.h"
#include "DeadTree.h"
#include "Object.h"
#include "Resources.h"
#include "Sky.h"
#include "GameHeader.h"
#include <cmath>

namespace {

float Saturate(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

float Lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

float SmoothStep(float edge0, float edge1, float value) {
  if (edge0 == edge1) {
    return value < edge0 ? 0.0f : 1.0f;
  }
  float t = Saturate((value - edge0) / (edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

float Fract(float value) {
  return value - std::floor(value);
}

float Hash2(float x, float y) {
  return Fract(std::sin(x * 127.1f + y * 311.7f) * 43758.5453f);
}

float ValueNoise2D(float x, float y) {
  float x0 = std::floor(x);
  float y0 = std::floor(y);
  float x1 = x0 + 1.0f;
  float y1 = y0 + 1.0f;

  float sx = SmoothStep(0.0f, 1.0f, x - x0);
  float sy = SmoothStep(0.0f, 1.0f, y - y0);

  float n00 = Hash2(x0, y0);
  float n10 = Hash2(x1, y0);
  float n01 = Hash2(x0, y1);
  float n11 = Hash2(x1, y1);

  float nx0 = Lerp(n00, n10, sx);
  float nx1 = Lerp(n01, n11, sx);
  return Lerp(nx0, nx1, sy);
}

float FBm2D(float x, float y) {
  float value = 0.0f;
  float amplitude = 0.5f;
  float frequency = 1.0f;
  float norm = 0.0f;

  for (int octave = 0; octave < 5; ++octave) {
    value += ValueNoise2D(x * frequency, y * frequency) * amplitude;
    norm += amplitude;
    frequency *= 2.01f;
    amplitude *= 0.5f;
  }

  return norm > 0.0f ? value / norm : 0.0f;
}

float EllipseMask(float x, float z, float cx, float cz, float rx, float rz, float rotation) {
  float dx = x - cx;
  float dz = z - cz;
  float c = std::cos(rotation);
  float s = std::sin(rotation);
  float lx = c * dx + s * dz;
  float lz = -s * dx + c * dz;
  float radial = std::sqrt((lx * lx) / (rx * rx) + (lz * lz) / (rz * rz));
  return 1.0f - SmoothStep(0.78f, 1.02f, radial);
}

float SampleForestPaintMask(float xNorm, float zNorm) {
  float radial = std::sqrt(xNorm * xNorm + zNorm * zNorm);
  if (radial >= 1.0f) {
    return 0.0f;
  }

  float mask = 0.0f;

  float brokenRim = SmoothStep(0.70f, 0.90f, radial) * (1.0f - SmoothStep(0.93f, 1.02f, radial));
  brokenRim *= 0.45f + 0.55f * ValueNoise2D(xNorm * 4.5f + 11.0f, zNorm * 4.5f - 5.0f);
  if (brokenRim > mask) mask = brokenRim;

  float patch = 0.0f;
  patch = EllipseMask(xNorm, zNorm, -0.42f,  0.58f, 0.25f, 0.18f, -0.45f); if (patch > mask) mask = patch;
  patch = 0.85f * EllipseMask(xNorm, zNorm, -0.66f,  0.16f, 0.15f, 0.10f, -0.25f); if (patch > mask) mask = patch;
  patch = 0.95f * EllipseMask(xNorm, zNorm,  0.34f,  0.48f, 0.25f, 0.21f,  0.20f); if (patch > mask) mask = patch;
  patch = 0.75f * EllipseMask(xNorm, zNorm,  0.02f,  0.84f, 0.20f, 0.10f,  0.00f); if (patch > mask) mask = patch;
  patch = 0.95f * EllipseMask(xNorm, zNorm, -0.42f, -0.47f, 0.31f, 0.22f, -0.90f); if (patch > mask) mask = patch;
  patch = 0.70f * EllipseMask(xNorm, zNorm,  0.02f, -0.78f, 0.19f, 0.11f,  0.00f); if (patch > mask) mask = patch;
  patch = 0.55f * EllipseMask(xNorm, zNorm,  0.12f, -0.42f, 0.18f, 0.16f,  0.35f); if (patch > mask) mask = patch;

  float carve = 0.0f;
  patch = EllipseMask(xNorm, zNorm, -0.08f,  0.42f, 0.26f, 0.15f,  0.00f); if (patch > carve) carve = patch;
  patch = 0.90f * EllipseMask(xNorm, zNorm,  0.12f,  0.02f, 0.32f, 0.22f, -0.10f); if (patch > carve) carve = patch;
  patch = 0.75f * EllipseMask(xNorm, zNorm, -0.16f, -0.58f, 0.20f, 0.14f,  0.25f); if (patch > carve) carve = patch;

  mask = Saturate(mask - 0.55f * carve);
  mask *= SmoothStep(0.18f, 0.30f, radial);
  return Saturate(mask);
}

float SampleTreeNoise(float worldX, float worldZ) {
  float coarse = FBm2D(worldX * 0.035f + 13.0f, worldZ * 0.035f - 9.0f);
  float detail = FBm2D(worldX * 0.085f - 21.0f, worldZ * 0.085f + 17.0f);
  return Saturate(coarse * 0.7f + detail * 0.3f);
}

bool IsInsideCircle(const Vector3& pos, float radius) {
  return pos.x * pos.x + pos.z * pos.z <= radius * radius;
}

void CreateBorderTreeRing(World& world, float radius) {
  const int borderTreeCount = 72;
  const float borderTreeScale = 16.0f;

  for (int i = 0; i < borderTreeCount; ++i) {
    float angle = (2.0f * GH_PI * static_cast<float>(i)) / static_cast<float>(borderTreeCount);
    std::shared_ptr<DeadTree> tree(new DeadTree());
    tree->scale = Vector3(borderTreeScale, borderTreeScale, borderTreeScale);
    tree->pos = Vector3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
    tree->euler.y = angle + GH_PI + (Hash2(static_cast<float>(i) * 1.7f, 19.0f) - 0.5f) * 0.35f;
    world.objects.push_back(tree);
  }
}

}

void Level1::Load(World &world, Player &player)
{
  const Vector3 playerSpawn(0.0f, GH_PLAYER_HEIGHT, 8.0f);

  world.ambientScale = 0.7f;

  std::shared_ptr<Object> ground(new Object());
  ground->mesh = AquireMesh("ground_large.obj");
  ground->shader = AquireShader("texture");
  ground->texture = AquireTexture("proto_dark_01.bmp");
  ground->scale = Vector3(500, 1, 500);
  world.objects.push_back(ground);

  std::shared_ptr<Object> cube(new Object());
  cube->mesh = AquireMesh("cube.obj");
  cube->shader = AquireShader("texture");
  cube->texture = AquireTexture("proto_orange_02.bmp");
  cube->scale = Vector3(2, 2, 2);
  cube->pos = Vector3(0.0f, cube->scale.y * 0.5f, 0.0f);
  cube->castShadow = false;
  cube->emissive = Vector3(4.0f, 3.8f, 3.2f);
  world.objects.push_back(cube);

  world.lightPos = cube->pos;
  world.lightColor = cube->emissive;

  std::shared_ptr<Object> markerCube(new Object());
  markerCube->mesh = AquireMesh("cube.obj");
  markerCube->shader = AquireShader("texture");
  markerCube->texture = AquireTexture("white.bmp");
  markerCube->scale = Vector3(10, 10, 10);
  markerCube->pos = Vector3(0.0f, markerCube->scale.y * 0.5f, playerSpawn.z - 50.0f);
  world.objects.push_back(markerCube);

  CreateDeadForest(world);

  world.sky = std::make_shared<Sky>("sky");

  player.SetPosition(playerSpawn);
  player.euler.y = 0.0f;
}

void Level1::CreateDeadForest(World &world)
{
  const float forestRadius = 108.0f;
  const float wallRadius = 116.0f;
  const float sampleStep = 8.0f;
  const Vector3 centerClear(0.0f, 0.0f, 0.0f);
  const Vector3 markerClear(0.0f, 0.0f, -42.0f);

  CreateBorderTreeRing(world, wallRadius);

  for (float x = -forestRadius; x <= forestRadius; x += sampleStep) {
    for (float z = -forestRadius; z <= forestRadius; z += sampleStep) {
      float jitterX = (Hash2(x * 0.17f + 4.0f, z * 0.29f - 2.0f) - 0.5f) * sampleStep * 0.8f;
      float jitterZ = (Hash2(x * 0.41f - 8.0f, z * 0.13f + 7.0f) - 0.5f) * sampleStep * 0.8f;
      Vector3 treePos(x + jitterX, 0.0f, z + jitterZ);

      if (!IsInsideCircle(treePos, forestRadius - 4.0f)) {
        continue;
      }
      if ((treePos - centerClear).MagSq() < 17.0f * 17.0f) {
        continue;
      }
      if ((treePos - markerClear).MagSq() < 11.0f * 11.0f) {
        continue;
      }

      float xNorm = treePos.x / forestRadius;
      float zNorm = treePos.z / forestRadius;
      float paintMask = SampleForestPaintMask(xNorm, zNorm);
      if (paintMask < 0.18f) {
        continue;
      }

      float noise = SampleTreeNoise(treePos.x, treePos.z);
      float threshold = Lerp(0.78f, 0.34f, paintMask);
      if (noise < threshold) {
        continue;
      }

      float normalizedStrength = SmoothStep(threshold, 1.0f, noise);
      float treeSize = Lerp(2.5f, 9.0f, normalizedStrength);

      std::shared_ptr<DeadTree> tree(new DeadTree());
      tree->scale = Vector3(treeSize, treeSize, treeSize);
      tree->pos = treePos;
      world.objects.push_back(tree);
    }
  }
}

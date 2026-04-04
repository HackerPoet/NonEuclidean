#pragma once
#include "GameHeader.h"
#include "Camera.h"
#include "Input.h"
#include "Object.h"
#include "Portal.h"
#include "Player.h"
#include "Timer.h"
#include "Scene.h"
#include "Sky.h"
#include "world.h"
#include "VulkanRenderer.h"
#include <windows.h>
#include <string>
#include <memory>
#include <vector>

class Engine {
public:
  Engine();
  ~Engine();

  int Run();
  void Update();
  void LoadScene(int ix);
   bool LoadSceneFromFile(const char* filename);
  bool SaveSceneToFile(const char* filename) const;

  LRESULT WindowProc(HWND hCurWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  const Player& GetPlayer() const { return *player; }
  float NearestPortalDist() const;
  const Vector3& GetLightPos() const { return curWorld.lightPos; }
  const Vector3& GetLightColor() const { return curWorld.lightColor; }
  const Matrix4& GetLightVP() const { return curWorld.lightVP; }
  bool HasLightCookie() const { return curWorld.lightCookie != nullptr; }

private:
  void CreateWindow_();
  void SetupInputs();
  void ConfineCursor();
  void ToggleFullscreen();
  void BeginMarkerNaming();
  void CommitMarkerNaming();
  void CancelMarkerNaming();
  void AddMarker(const std::string& name);
  void UndoMarker();
  void UpdateWindowTitle();
  void UploadCurrentWorld();

  HWND  hWnd;
  HINSTANCE hInstance;

  LONG iWidth;
  LONG iHeight;
  bool isFullscreen;
  bool cursorCaptured;
  bool markerNamingActive;
  bool markerNamingRestoreCapture;
  std::string pendingMarkerName;

  Camera main_cam;
  Input input;
  Timer timer;

  World curWorld;
  std::shared_ptr<Sky> defaultSky;
  std::shared_ptr<Player> player;

  VulkanRenderer renderer;

  std::vector<std::shared_ptr<Scene>> vScenes;
  std::shared_ptr<Scene> curScene;
};

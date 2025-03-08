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
#include <GL/glew.h>
#if defined(_WIN32)
  #include <windows.h>
#else
  #include <SDL2/SDL.h>
#endif
#include <memory>
#include <vector>

class Engine {
public:
  Engine();
  ~Engine();

  int Run();
  void Update();
  void Render(const Camera& cam, GLuint curFBO, const Portal* skipPortal);
  void LoadScene(int ix);

#if defined(_WIN32)
  LRESULT WindowProc(HWND hCurWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

  const Player& GetPlayer() const { return *player; }
  float NearestPortalDist() const;

private:
  bool InitOSWrapper();
  void CreateGLWindow();
  void DestroyGLWindow();
  void InitGLObjects();
  void DestroyGLObjects();
  void SetupInputs();
  void ConfineCursor();
  void ToggleFullscreen();
  int EnterMessageLoop();
  void PeriodicRender(int64_t &cur_ticks);
  void EnableVSync();

#if defined(_WIN32)
  HWND  hWnd = nullptr;         // window
  HDC   hDC = nullptr;          // device context
  HGLRC hRC = nullptr;          // opengl context
  HINSTANCE hInstance;          // process id

  LONG iWidth;         // window width
  LONG iHeight;        // window height
#else
  SDL_Window* window = nullptr;
  SDL_GLContext glContext = nullptr;
  int iWidth = 0;
  int iHeight = 0;
#endif

  int64_t ticks_per_step = 0;

  bool isGood = false;          // initialized without problems
  bool isWindowGood = false;    // window successfully created and initialized
  bool isFullscreen;   // fullscreen state

  Camera main_cam;
  Input input;
  Timer timer;

  std::vector<std::shared_ptr<Object>> vObjects;
  std::vector<std::shared_ptr<Portal>> vPortals;
  std::shared_ptr<Sky> sky;
  std::shared_ptr<Player> player;

  GLint occlusionCullingSupported;

  std::vector<std::shared_ptr<Scene>> vScenes;
  std::shared_ptr<Scene> curScene;
};

#include "Engine.h"
#include "Physical.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "Level4.h"
#include "Level5.h"
#include "Level6.h"
#include <GL/wglew.h>
#include <cmath>
#include <iostream>
#include <algorithm>

Engine* GH_ENGINE = nullptr;
Player* GH_PLAYER = nullptr;
const Input* GH_INPUT = nullptr;
int GH_REC_LEVEL = 0;
int64_t GH_FRAME = 0;

LRESULT WINAPI StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  Engine* eng = (Engine*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if (eng) {
    return eng->WindowProc(hWnd, uMsg, wParam, lParam);
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Engine::Engine() : hWnd(NULL), hDC(NULL), hRC(NULL) {
  GH_ENGINE = this;
  GH_INPUT = &input;
  isFullscreen = false;

  SetProcessDPIAware();
  CreateGLWindow();
  InitGLObjects();
  SetupInputs();

  player.reset(new Player);
  GH_PLAYER = player.get();

  vScenes.push_back(std::shared_ptr<Scene>(new Level1));
  vScenes.push_back(std::shared_ptr<Scene>(new Level2(3)));
  vScenes.push_back(std::shared_ptr<Scene>(new Level2(6)));
  vScenes.push_back(std::shared_ptr<Scene>(new Level3));
  vScenes.push_back(std::shared_ptr<Scene>(new Level4));
  vScenes.push_back(std::shared_ptr<Scene>(new Level5));
  vScenes.push_back(std::shared_ptr<Scene>(new Level6));

  LoadScene(0);

  sky.reset(new Sky);
}

Engine::~Engine() {
  ClipCursor(NULL);
  wglMakeCurrent(NULL, NULL);
  ReleaseDC(hWnd, hDC);
  wglDeleteContext(hRC);
  DestroyWindow(hWnd);
}

int Engine::Run() {
  if (!hWnd || !hDC || !hRC) {
    return 1;
  }

  //Recieve events from this window
  SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

  //Setup the timer
  const int64_t ticks_per_step = timer.SecondsToTicks(GH_DT);
  int64_t cur_ticks = timer.GetTicks();
  GH_FRAME = 0;

  //Game loop
  MSG msg;
  while (true) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      //Handle windows messages
      if (msg.message == WM_QUIT) {
        break;
      } else {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    } else {
      //Confine the cursor
      ConfineCursor();

      if (input.key_press['1']) {
        LoadScene(0);
      } else if (input.key_press['2']) {
        LoadScene(1);
      } else if (input.key_press['3']) {
        LoadScene(2);
      } else if (input.key_press['4']) {
        LoadScene(3);
      } else if (input.key_press['5']) {
        LoadScene(4);
      } else if (input.key_press['6']) {
        LoadScene(5);
      } else if (input.key_press['7']) {
        LoadScene(6);
      }

      //Used fixed time steps for updates
      const int64_t new_ticks = timer.GetTicks();
      for (int i = 0; cur_ticks < new_ticks && i < GH_MAX_STEPS; ++i) {
        Update();
        cur_ticks += ticks_per_step;
        GH_FRAME += 1;
        input.EndFrame();
      }
      cur_ticks = (cur_ticks < new_ticks ? new_ticks: cur_ticks);

      //Setup camera for rendering
      const float n = GH_CLAMP(NearestPortalDist() * 0.5f, GH_NEAR_MIN, GH_NEAR_MAX);
      main_cam.worldView = player->WorldToCam();
      main_cam.SetSize(iWidth, iHeight, n, GH_FAR);
      main_cam.UseViewport();

      //Render scene
      GH_REC_LEVEL = GH_MAX_RECURSION;
      Render(main_cam, 0, nullptr);
      SwapBuffers(hDC);
    }
  }

  DestroyGLObjects();
  return 0;
}

void Engine::LoadScene(int ix) {
  //Clear out old scene
  if (curScene) { curScene->Unload(); }
  vObjects.clear();
  vPortals.clear();
  player->Reset();

  //Create new scene
  curScene = vScenes[ix];
  curScene->Load(vObjects, vPortals, *player);
  vObjects.push_back(player);
}

void Engine::Update() {
  //Update
  for (size_t i = 0; i < vObjects.size(); ++i) {
    assert(vObjects[i].get());
    vObjects[i]->Update();
  }

  //Collisions
  //For each physics object
  for (size_t i = 0; i < vObjects.size(); ++i) {
    Physical* physical = vObjects[i]->AsPhysical();
    if (!physical) { continue; }
    Matrix4 worldToLocal = physical->WorldToLocal();

    //For each object to collide with
    for (size_t j = 0; j < vObjects.size(); ++j) {
      if (i == j) { continue; }
      Object& obj = *vObjects[j];
      if (!obj.mesh) { continue; }

      //For each hit sphere
      for (size_t s = 0; s < physical->hitSpheres.size(); ++s) {
        //Brings point from collider's local coordinates to hits's local coordinates.
        const Sphere& sphere = physical->hitSpheres[s];
        Matrix4 worldToUnit = sphere.LocalToUnit() * worldToLocal;
        Matrix4 localToUnit = worldToUnit * obj.LocalToWorld();
        Matrix4 unitToWorld = worldToUnit.Inverse();

        //For each collider
        for (size_t c = 0; c < obj.mesh->colliders.size(); ++c) {
          Vector3 push;
          const Collider& collider = obj.mesh->colliders[c];
          if (collider.Collide(localToUnit, push)) {
            //If push is too small, just ignore
            push = unitToWorld.MulDirection(push);
            vObjects[j]->OnHit(*physical, push);
            physical->OnCollide(*vObjects[j], push);

            worldToLocal = physical->WorldToLocal();
            worldToUnit = sphere.LocalToUnit() * worldToLocal;
            localToUnit = worldToUnit * obj.LocalToWorld();
            unitToWorld = worldToUnit.Inverse();
          }
        }
      }
    }
  }

  //Portals
  for (size_t i = 0; i < vObjects.size(); ++i) {
    Physical* physical = vObjects[i]->AsPhysical();
    if (physical) {
      for (size_t j = 0; j < vPortals.size(); ++j) {
        if (physical->TryPortal(*vPortals[j])) {
          break;
        }
      }
    }
  }
}

void Engine::Render(const Camera& cam, GLuint curFBO, const Portal* skipPortal) {
  //Clear buffers
  if (GH_USE_SKY) {
    glClear(GL_DEPTH_BUFFER_BIT);
    sky->Draw(cam);
  } else {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  //Create queries (if applicable)
  GLuint queries[GH_MAX_PORTALS];
  GLuint drawTest[GH_MAX_PORTALS];
  assert(vPortals.size() <= GH_MAX_PORTALS);
  if (occlusionCullingSupported) {
    glGenQueriesARB((GLsizei)vPortals.size(), queries);
  }

  //Draw scene
  for (size_t i = 0; i < vObjects.size(); ++i) {
    vObjects[i]->Draw(cam, curFBO);
  }

  //Draw portals if possible
  if (GH_REC_LEVEL > 0) {
    //Draw portals
    GH_REC_LEVEL -= 1;
    if (occlusionCullingSupported && GH_REC_LEVEL > 0) {
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glDepthMask(GL_FALSE);
      for (size_t i = 0; i < vPortals.size(); ++i) {
        if (vPortals[i].get() != skipPortal) {
          glBeginQueryARB(GL_SAMPLES_PASSED_ARB, queries[i]);
          vPortals[i]->DrawPink(cam);
          glEndQueryARB(GL_SAMPLES_PASSED_ARB);
        }
      }
      for (size_t i = 0; i < vPortals.size(); ++i) {
        if (vPortals[i].get() != skipPortal) {
          glGetQueryObjectuivARB(queries[i], GL_QUERY_RESULT_ARB, &drawTest[i]);
        }
      };
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDepthMask(GL_TRUE);
      glDeleteQueriesARB((GLsizei)vPortals.size(), queries);
    }
    for (size_t i = 0; i < vPortals.size(); ++i) {
      if (vPortals[i].get() != skipPortal) {
        if (occlusionCullingSupported && (GH_REC_LEVEL > 0) && (drawTest[i] == 0)) {
          continue;
        } else {
          vPortals[i]->Draw(cam, curFBO);
        }
      }
    }
    GH_REC_LEVEL += 1;
  }
  
#if 0
  //Debug draw colliders
  for (size_t i = 0; i < vObjects.size(); ++i) {
    vObjects[i]->DebugDraw(cam);
  }
#endif
}

LRESULT Engine::WindowProc(HWND hCurWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  static PAINTSTRUCT ps;
  static BYTE lpb[256];
  static UINT dwSize = sizeof(lpb);

  switch (uMsg) {
  case WM_SYSCOMMAND:
    if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) {
      return 0;
    }
    break;

  case WM_PAINT:
    BeginPaint(hCurWnd, &ps);
    EndPaint(hCurWnd, &ps);
    return 0;

  case WM_SIZE:
    iWidth = LOWORD(lParam);
    iHeight = HIWORD(lParam);
    PostMessage(hCurWnd, WM_PAINT, 0, 0);
    return 0;

  case WM_KEYDOWN:
    //Ignore repeat keys
    if (lParam & 0x40000000) { return 0; }
    input.key[wParam & 0xFF] = true;
    input.key_press[wParam & 0xFF] = true;
    if (wParam == VK_ESCAPE) {
      PostQuitMessage(0);
    }
    return 0;

  case WM_SYSKEYDOWN:
    if (wParam == VK_RETURN) {
      ToggleFullscreen();
      return 0;
    }
    break;

  case WM_KEYUP:
    input.key[wParam & 0xFF] = false;
    return 0;

  case WM_INPUT:
    dwSize = sizeof(lpb);
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
    input.UpdateRaw((const RAWINPUT*)lpb);
    break;

  case WM_CLOSE:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hCurWnd, uMsg, wParam, lParam);
}

void Engine::CreateGLWindow() {
  WNDCLASSEX wc;
  hInstance = GetModuleHandle(NULL);
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = (WNDPROC)StaticWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = GH_CLASS;
  wc.hIconSm = NULL;

  if (!RegisterClassEx(&wc)) {
    MessageBoxEx(NULL, "RegisterClass() failed: Cannot register window class.", "Error", MB_OK, 0);
    return;
  }

  //Always start in windowed mode
  iWidth = GH_SCREEN_WIDTH;
  iHeight = GH_SCREEN_HEIGHT;

  //Create the window
  hWnd = CreateWindowEx(
    WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
    GH_CLASS,
    GH_TITLE,
    WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
    GH_SCREEN_X,
    GH_SCREEN_Y,
    iWidth,
    iHeight,
    NULL,
    NULL,
    hInstance,
    NULL);

  if (hWnd == NULL) {
    MessageBoxEx(NULL, "CreateWindow() failed:  Cannot create a window.", "Error", MB_OK, 0);
    return;
  }

  hDC = GetDC(hWnd);

  PIXELFORMATDESCRIPTOR pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 32;
  pfd.iLayerType = PFD_MAIN_PLANE;

  const int pf = ChoosePixelFormat(hDC, &pfd);
  if (pf == 0) {
    MessageBoxEx(NULL, "ChoosePixelFormat() failed: Cannot find a suitable pixel format.", "Error", MB_OK, 0);
    return;
  }

  if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
    MessageBoxEx(NULL, "SetPixelFormat() failed: Cannot set format specified.", "Error", MB_OK, 0);
    return;
  }

  DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

  hRC = wglCreateContext(hDC);
  wglMakeCurrent(hDC, hRC);

  if (GH_START_FULLSCREEN) {
    ToggleFullscreen();
  }
  if (GH_HIDE_MOUSE) {
    ShowCursor(FALSE);
  }

  ShowWindow(hWnd, SW_SHOW);
  SetForegroundWindow(hWnd);
  SetFocus(hWnd);
}

void Engine::InitGLObjects() {
  //Initialize extensions
  glewInit();

  //Basic global variables
  glClearColor(0.6f, 0.9f, 1.0f, 1.0f);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  //Check GL functionality
  glGetQueryiv(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &occlusionCullingSupported);

  //Attempt to enalbe vsync (if failure then oh well)
  wglSwapIntervalEXT(1);
}

void Engine::DestroyGLObjects() {
  curScene->Unload();
  vObjects.clear();
  vPortals.clear();
}

void Engine::SetupInputs() {
  static const int HID_USAGE_PAGE_GENERIC     = 0x01;
  static const int HID_USAGE_GENERIC_MOUSE    = 0x02;
  static const int HID_USAGE_GENERIC_JOYSTICK = 0x04;
  static const int HID_USAGE_GENERIC_GAMEPAD  = 0x05;

  RAWINPUTDEVICE Rid[3];

  //Mouse
  Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
  Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
  Rid[0].dwFlags = RIDEV_INPUTSINK;
  Rid[0].hwndTarget = hWnd;

  //Joystick
  Rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
  Rid[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;
  Rid[1].dwFlags = 0;
  Rid[1].hwndTarget = 0;

  //Gamepad
  Rid[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
  Rid[2].usUsage = HID_USAGE_GENERIC_GAMEPAD;
  Rid[2].dwFlags = 0;
  Rid[2].hwndTarget = 0;

  RegisterRawInputDevices(Rid, 3, sizeof(Rid[0]));
}

void Engine::ConfineCursor() {
  if (GH_HIDE_MOUSE) {
    RECT rect;
    GetWindowRect(hWnd, &rect);
    SetCursorPos((rect.right + rect.left) / 2, (rect.top + rect.bottom) / 2);
  }
}

float Engine::NearestPortalDist() const {
  float dist = FLT_MAX;
  for (size_t i = 0; i < vPortals.size(); ++i) {
    dist = GH_MIN(dist, vPortals[i]->DistTo(player->pos));
  }
  return dist;
}

void Engine::ToggleFullscreen() {
  isFullscreen = !isFullscreen;
  if (isFullscreen) {
    iWidth = GetSystemMetrics(SM_CXSCREEN);
    iHeight = GetSystemMetrics(SM_CYSCREEN);
    SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0,
      iWidth, iHeight, SWP_SHOWWINDOW);
  } else {
    iWidth = GH_SCREEN_WIDTH;
    iHeight = GH_SCREEN_HEIGHT;
    SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    SetWindowPos(hWnd, HWND_TOP, GH_SCREEN_X, GH_SCREEN_Y,
      iWidth, iHeight, SWP_SHOWWINDOW);
  }
}

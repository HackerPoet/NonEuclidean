#include "Engine.h"
#include "Physical.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "Level4.h"
#include "Level5.h"
#include "Level6.h"
#include "Level7.h"
#include "Level8.h"
#include "Resources.h"
#include "SceneFile.h"
#include <cctype>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

Engine *GH_ENGINE = nullptr;
Player *GH_PLAYER = nullptr;
const Input *GH_INPUT = nullptr;
bool GH_CURSOR_CAPTURED = false;
bool GH_LOCK_CAMERA = false;
int GH_REC_LEVEL = 0;
int64_t GH_FRAME = 0;

static const char GH_QUICK_SCENE_FILE[] = "quick_save.scene";
static const float GH_MARKER_DISTANCE = 10.0f;
static const float GH_MARKER_SIZE = 1.0f;
static const size_t GH_MARKER_NAME_LIMIT = 48;

namespace {

std::string TrimWhitespace(const std::string& value)
{
  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
  {
    ++start;
  }

  size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
  {
    --end;
  }

  return value.substr(start, end - start);
}

bool HasObjectName(const World& world, const std::string& name)
{
  for (size_t i = 0; i < world.objects.size(); ++i)
  {
    if (world.objects[i] && world.objects[i]->name == name)
    {
      return true;
    }
  }
  return false;
}

std::string MakeUniqueObjectName(const World& world, const std::string& baseName)
{
  if (baseName.empty() || !HasObjectName(world, baseName))
  {
    return baseName;
  }

  for (int suffix = 2; ; ++suffix)
  {
    std::string candidate = baseName + "_" + std::to_string(suffix);
    if (!HasObjectName(world, candidate))
    {
      return candidate;
    }
  }
}

std::string BuildAutoMarkerName(const World& world)
{
  for (int index = 1; ; ++index)
  {
    std::ostringstream name;
    name << "marker_" << std::setfill('0') << std::setw(3) << index;
    if (!HasObjectName(world, name.str()))
    {
      return name.str();
    }
  }
}

}

static bool IsEnvFlagEnabled(const char* name)
{
  char value[8] = {};
  DWORD len = GetEnvironmentVariableA(name, value, static_cast<DWORD>(sizeof(value)));
  if (len == 0 || len >= sizeof(value))
  {
    return false;
  }

  return value[0] == '1' || value[0] == 't' || value[0] == 'T' ||
         value[0] == 'y' || value[0] == 'Y';
}

LRESULT WINAPI StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  Engine *eng = (Engine *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if (eng)
  {
    return eng->WindowProc(hWnd, uMsg, wParam, lParam);
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Engine::Engine() : hWnd(NULL), markerNamingActive(false), markerNamingRestoreCapture(false)
{
  GH_ENGINE = this;
  GH_INPUT = &input;
  isFullscreen = false;
  GH_LOCK_CAMERA = IsEnvFlagEnabled("NONEUCLIDEAN_LOCK_CAMERA");

  SetProcessDPIAware();
  CreateWindow_();
  SetupInputs();

  // Init Vulkan renderer
  if (!renderer.Init(hWnd, hInstance, iWidth, iHeight))
  {
    MessageBoxA(nullptr, "Failed to initialize Vulkan renderer.", "Error", MB_OK);
  }

  player.reset(new Player);
  GH_PLAYER = player.get();

  vScenes.push_back(std::shared_ptr<Scene>(new Level1));
  vScenes.push_back(std::shared_ptr<Scene>(new Level2(3)));
  vScenes.push_back(std::shared_ptr<Scene>(new Level2(6)));
  vScenes.push_back(std::shared_ptr<Scene>(new Level3));
  vScenes.push_back(std::shared_ptr<Scene>(new Level4));
  vScenes.push_back(std::shared_ptr<Scene>(new Level5));
  vScenes.push_back(std::shared_ptr<Scene>(new Level6));
  vScenes.push_back(std::shared_ptr<Scene>(new Level7));
  vScenes.push_back(std::shared_ptr<Scene>(new Level8));

  LoadScene(0);

  defaultSky.reset(new Sky);
}

Engine::~Engine()
{
  ClipCursor(NULL);
  renderer.Shutdown();
  DestroyWindow(hWnd);
}

int Engine::Run()
{
  if (!hWnd)
  {
    return 1;
  }

  SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

  const int64_t ticks_per_step = timer.SecondsToTicks(GH_DT);
  int64_t cur_ticks = timer.GetTicks();
  GH_FRAME = 0;

  MSG msg;
  while (true)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (msg.message == WM_QUIT)
      {
        break;
      }
      else
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    else
    {
      ConfineCursor();

      if (!markerNamingActive)
      {
        if (input.key_press['1'])
        {
          LoadScene(0);
        }
        else if (input.key_press['2'])
        {
          LoadScene(1);
        }
        else if (input.key_press['3'])
        {
          LoadScene(2);
        }
        else if (input.key_press['4'])
        {
          LoadScene(3);
        }
        else if (input.key_press['5'])
        {
          LoadScene(4);
        }
        else if (input.key_press['6'])
        {
          LoadScene(5);
        }
        else if (input.key_press['7'])
        {
          LoadScene(6);
        }
        else if (input.key_press['8'])
        {
          LoadScene(7);
        }
        else if (input.key_press['9'])
        {
          LoadScene(8);
        }
        else if (input.key_press['R'])
        {
          renderer.ReloadShaders();
        }
        else if (input.key_press[VK_F5])
        {
          SaveSceneToFile(GH_QUICK_SCENE_FILE);
        }
        else if (input.key_press[VK_F9])
        {
          LoadSceneFromFile(GH_QUICK_SCENE_FILE);
        }
        else if (input.key_press['M'])
        {
          BeginMarkerNaming();
        }
        else if (input.key_press['U'] || (input.key[VK_CONTROL] && input.key_press['Z']))
        {
          UndoMarker();
        }
      }

    const int64_t new_ticks = timer.GetTicks();
    for (int i = 0; cur_ticks < new_ticks && i < GH_MAX_STEPS; ++i)
    {
      Update();
      cur_ticks += ticks_per_step;
      GH_FRAME += 1;
      input.EndFrame();
    }
    cur_ticks = (cur_ticks < new_ticks ? new_ticks : cur_ticks);

    // Setup camera
    const float n = GH_CLAMP(NearestPortalDist() * 0.5f, GH_NEAR_MIN, GH_NEAR_MAX);
    main_cam.worldView = player->WorldToCam();
    main_cam.worldPos = player->CamToWorld().Translation();
    main_cam.SetSize(iWidth, iHeight, n, GH_FAR);

    // Set light state
    main_cam.lightPos = curWorld.lightPos;
    main_cam.lightColor = curWorld.lightColor;

    // Render via Vulkan RT
    renderer.Render(main_cam, curWorld);
  }
}

return 0;
}

void Engine::LoadScene(int ix)
{
  CancelMarkerNaming();

  // Clear out old scene
  if (curScene)
  {
    curScene->Unload();
  }
  curWorld.Clear();
  player->Reset();

  // Create new scene
  curScene = vScenes[ix];
  curScene->Load(curWorld, *player);
  curWorld.objects.push_back(player);

  // Upload scene to Vulkan RT
  UploadCurrentWorld();
}

bool Engine::SaveSceneToFile(const char* filename) const {
  return SaveSceneFile(filename, curWorld, player.get());
}

bool Engine::LoadSceneFromFile(const char* filename) {
  CancelMarkerNaming();

  World loadedWorld;
  Player loadedPlayer;
  if (!LoadSceneFile(filename, loadedWorld, &loadedPlayer)) {
    return false;
  }

  if (curScene)
  {
    curScene->Unload();
  }
  curScene.reset();
  curWorld = loadedWorld;
  *player = loadedPlayer;

  UploadCurrentWorld();
  return true;
}

void Engine::BeginMarkerNaming()
{
  if (markerNamingActive)
  {
    return;
  }

  markerNamingActive = true;
  pendingMarkerName.clear();
  markerNamingRestoreCapture = cursorCaptured;

  memset(input.key, 0, sizeof(input.key));
  memset(input.key_press, 0, sizeof(input.key_press));

  if (cursorCaptured)
  {
    cursorCaptured = false;
    GH_CURSOR_CAPTURED = false;
    if (GH_HIDE_MOUSE)
    {
      ShowCursor(TRUE);
    }
    ClipCursor(NULL);
  }

  UpdateWindowTitle();
}

void Engine::CommitMarkerNaming()
{
  if (!markerNamingActive)
  {
    return;
  }

  std::string markerName = TrimWhitespace(pendingMarkerName);
  if (markerName.empty())
  {
    markerName = BuildAutoMarkerName(curWorld);
  }
  else
  {
    markerName = MakeUniqueObjectName(curWorld, markerName);
  }

  CancelMarkerNaming();
  AddMarker(markerName);
}

void Engine::CancelMarkerNaming()
{
  if (!markerNamingActive)
  {
    return;
  }

  markerNamingActive = false;
  pendingMarkerName.clear();
  memset(input.key, 0, sizeof(input.key));
  memset(input.key_press, 0, sizeof(input.key_press));

  cursorCaptured = markerNamingRestoreCapture;
  GH_CURSOR_CAPTURED = cursorCaptured;
  if (markerNamingRestoreCapture && GH_HIDE_MOUSE)
  {
    ShowCursor(FALSE);
  }
  markerNamingRestoreCapture = false;

  UpdateWindowTitle();
}

void Engine::AddMarker(const std::string& name)
{
  if (name.empty())
  {
    return;
  }

  std::shared_ptr<Object> marker(new Object());
  marker->mesh = AquireMesh("cube.obj");
  marker->shader = AquireShader("texture");
  marker->texture = AquireTexture("white.bmp");
  marker->scale = Vector3(GH_MARKER_SIZE * player->p_scale);
  marker->name = name;
  marker->isMarker = true;

  Vector3 forward = player->CamToWorld().MulDirection(Vector3(0.0f, 0.0f, -1.0f));
  forward.y = 0.0f;
  if (forward.MagSq() < 1e-6f)
  {
    forward = Vector3(0.0f, 0.0f, -1.0f);
  }
  else
  {
    forward.Normalize();
  }

  const float markerGroundY = player->pos.y - GH_PLAYER_HEIGHT * player->p_scale;
  marker->pos = player->pos + forward * (GH_MARKER_DISTANCE * player->p_scale);
  marker->pos.y = markerGroundY + marker->scale.y * 0.5f;

  curWorld.objects.push_back(marker);
  UploadCurrentWorld();
}

void Engine::UndoMarker()
{
  for (PObjectVec::reverse_iterator it = curWorld.objects.rbegin(); it != curWorld.objects.rend(); ++it)
  {
    if (*it && (*it)->isMarker)
    {
      curWorld.objects.erase((it + 1).base());
      UploadCurrentWorld();
      return;
    }
  }
}

void Engine::UpdateWindowTitle()
{
  std::string title = GH_TITLE;
  if (markerNamingActive)
  {
    title += " - Marker name: ";
    title += (pendingMarkerName.empty() ? "<auto>" : pendingMarkerName);
    title += " [Enter place, Esc cancel, Backspace edit]";
  }
  SetWindowTextA(hWnd, title.c_str());
}

void Engine::UploadCurrentWorld() {
  curWorld.materializePortalMarkers();
  bool hasPlayer = false;
  for (size_t i = 0; i < curWorld.objects.size(); ++i) {
    if (curWorld.objects[i].get() == player.get()) {
      hasPlayer = true;
      break;
    }
  }
  if (!hasPlayer) {
    curWorld.objects.push_back(player);
  }
  renderer.UploadScene(curWorld);
}

void Engine::Update()
{
  // Update
  for (size_t i = 0; i < curWorld.objects.size(); ++i)
  {
    assert(curWorld.objects[i].get());
    curWorld.objects[i]->Update();
  }

  // Collisions
  // For each physics object
  for (size_t i = 0; i < curWorld.objects.size(); ++i)
  {
    Physical *physical = curWorld.objects[i]->AsPhysical();
    if (!physical)
    {
      continue;
    }
    Matrix4 worldToLocal = physical->WorldToLocal();

    // For each object to collide with
    for (size_t j = 0; j < curWorld.objects.size(); ++j)
    {
      if (i == j)
      {
        continue;
      }
      Object &obj = *curWorld.objects[j];
      if (!obj.mesh)
      {
        continue;
      }

      // For each hit sphere
      for (size_t s = 0; s < physical->hitSpheres.size(); ++s)
      {
        // Brings point from collider's local coordinates to hits's local coordinates.
        const Sphere &sphere = physical->hitSpheres[s];
        Matrix4 worldToUnit = sphere.LocalToUnit() * worldToLocal;
        Matrix4 localToUnit = worldToUnit * obj.LocalToWorld();
        Matrix4 unitToWorld = worldToUnit.Inverse();

        // For each collider
        for (size_t c = 0; c < obj.mesh->colliders.size(); ++c)
        {
          Vector3 push;
          const Collider &collider = obj.mesh->colliders[c];
          if (collider.Collide(localToUnit, push))
          {
            // If push is too small, just ignore
            push = unitToWorld.MulDirection(push);
            curWorld.objects[j]->OnHit(*physical, push);
            physical->OnCollide(*curWorld.objects[j], push);

            worldToLocal = physical->WorldToLocal();
            worldToUnit = sphere.LocalToUnit() * worldToLocal;
            localToUnit = worldToUnit * obj.LocalToWorld();
            unitToWorld = worldToUnit.Inverse();
          }
        }
      }
    }
  }

  // Portals
  for (size_t i = 0; i < curWorld.objects.size(); ++i)
  {
    Physical *physical = curWorld.objects[i]->AsPhysical();
    if (physical)
    {
      for (size_t j = 0; j < curWorld.portals.size(); ++j)
      {
        if (physical->TryPortal(*curWorld.portals[j]))
        {
          break;
        }
      }
    }
  }
}

LRESULT Engine::WindowProc(HWND hCurWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static PAINTSTRUCT ps;
  static BYTE lpb[256];
  static UINT dwSize = sizeof(lpb);

  switch (uMsg)
  {
  case WM_SYSCOMMAND:
    if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)
    {
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
    renderer.OnResize(iWidth, iHeight);
    PostMessage(hCurWnd, WM_PAINT, 0, 0);
    return 0;

  case WM_KEYDOWN:
    if (lParam & 0x40000000)
    {
      return 0;
    }

    if (markerNamingActive)
    {
      if (wParam == VK_ESCAPE)
      {
        CancelMarkerNaming();
      }
      else if (wParam == VK_RETURN)
      {
        CommitMarkerNaming();
      }
      else if (wParam == VK_BACK)
      {
        if (!pendingMarkerName.empty())
        {
          pendingMarkerName.pop_back();
          UpdateWindowTitle();
        }
      }
      return 0;
    }

    input.key[wParam & 0xFF] = true;
    input.key_press[wParam & 0xFF] = true;
    if (wParam == VK_ESCAPE)
    {
      PostQuitMessage(0);
    }
    else if (wParam == VK_TAB)
    {
      cursorCaptured = !cursorCaptured;
      GH_CURSOR_CAPTURED = cursorCaptured;
      ShowCursor(cursorCaptured ? FALSE : TRUE);
      if (cursorCaptured)
      {
        ClipCursor(NULL);
      }
    }
    return 0;

  case WM_SYSKEYDOWN:
    if (wParam == VK_RETURN)
    {
      ToggleFullscreen();
      return 0;
    }
    break;

  case WM_KEYUP:
    if (markerNamingActive)
    {
      return 0;
    }
    input.key[wParam & 0xFF] = false;
    return 0;

  case WM_CHAR:
    if (markerNamingActive)
    {
      if (wParam >= 32 && wParam <= 126 && pendingMarkerName.size() < GH_MARKER_NAME_LIMIT)
      {
        pendingMarkerName.push_back(static_cast<char>(wParam));
        UpdateWindowTitle();
      }
      return 0;
    }
    break;

  case WM_INPUT:
    dwSize = sizeof(lpb);
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
    input.UpdateRaw((const RAWINPUT *)lpb);
    break;

  case WM_CLOSE:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hCurWnd, uMsg, wParam, lParam);
}

void Engine::CreateWindow_()
{
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

  if (!RegisterClassEx(&wc))
  {
    MessageBoxEx(NULL, "RegisterClass() failed: Cannot register window class.", "Error", MB_OK, 0);
    return;
  }

  iWidth = GH_SCREEN_WIDTH;
  iHeight = GH_SCREEN_HEIGHT;

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

  if (hWnd == NULL)
  {
    MessageBoxEx(NULL, "CreateWindow() failed: Cannot create a window.", "Error", MB_OK, 0);
    return;
  }

  if (GH_START_FULLSCREEN)
  {
    ToggleFullscreen();
  }
  cursorCaptured = !GH_LOCK_CAMERA;
  GH_CURSOR_CAPTURED = true;
  GH_CURSOR_CAPTURED = cursorCaptured;
  if (GH_HIDE_MOUSE && cursorCaptured)
  {
    ShowCursor(FALSE);
  }

  ShowWindow(hWnd, SW_SHOW);
  SetForegroundWindow(hWnd);
  SetFocus(hWnd);
}

void Engine::SetupInputs()
{
  static const int HID_USAGE_PAGE_GENERIC = 0x01;
  static const int HID_USAGE_GENERIC_MOUSE = 0x02;
  static const int HID_USAGE_GENERIC_JOYSTICK = 0x04;
  static const int HID_USAGE_GENERIC_GAMEPAD = 0x05;

  RAWINPUTDEVICE Rid[3];

  // Mouse
  Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
  Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
  Rid[0].dwFlags = RIDEV_INPUTSINK;
  Rid[0].hwndTarget = hWnd;

  // Joystick
  Rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
  Rid[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;
  Rid[1].dwFlags = 0;
  Rid[1].hwndTarget = 0;

  // Gamepad
  Rid[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
  Rid[2].usUsage = HID_USAGE_GENERIC_GAMEPAD;
  Rid[2].dwFlags = 0;
  Rid[2].hwndTarget = 0;

  RegisterRawInputDevices(Rid, 3, sizeof(Rid[0]));
}

void Engine::ConfineCursor()
{
  if (GH_HIDE_MOUSE && cursorCaptured)
  {
    RECT rect;
    GetWindowRect(hWnd, &rect);
    SetCursorPos((rect.right + rect.left) / 2, (rect.top + rect.bottom) / 2);
  }
}

float Engine::NearestPortalDist() const
{
  float dist = FLT_MAX;
  for (size_t i = 0; i < curWorld.portals.size(); ++i)
  {
    dist = GH_MIN(dist, curWorld.portals[i]->DistTo(player->pos));
  }
  return dist;
}

void Engine::ToggleFullscreen()
{
  isFullscreen = !isFullscreen;
  if (isFullscreen)
  {
    iWidth = GetSystemMetrics(SM_CXSCREEN);
    iHeight = GetSystemMetrics(SM_CYSCREEN);
    SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0,
                 iWidth, iHeight, SWP_SHOWWINDOW);
  }
  else
  {
    iWidth = GH_SCREEN_WIDTH;
    iHeight = GH_SCREEN_HEIGHT;
    SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    SetWindowPos(hWnd, HWND_TOP, GH_SCREEN_X, GH_SCREEN_Y,
                 iWidth, iHeight, SWP_SHOWWINDOW);
  }
}

#include "Engine.h"
#include "Physical.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "Level4.h"
#include "Level5.h"
#include "Level6.h"
#if defined(_WIN32)
  #include <GL/wglew.h>
#endif
#include <cmath>
#include <iostream>
#include <algorithm>

// --- Windows-specific code ------------------------------------------------

#if defined(_WIN32)

LRESULT WINAPI StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  Engine* eng = (Engine*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  if (eng) {
    return eng->WindowProc(hWnd, uMsg, wParam, lParam);
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
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

bool Engine::InitOSWrapper() {
  return true;  // there is no wrapper, we are using Windows API directly
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

void Engine::CreateGLWindow() {
  SetProcessDPIAware();

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

void Engine::DestroyGLWindow() {
  ClipCursor(NULL);
  wglMakeCurrent(NULL, NULL);
  ReleaseDC(hWnd, hDC);
  wglDeleteContext(hRC);
  DestroyWindow(hWnd);
}

int Engine::EnterMessageLoop() {
  if (!hWnd || !hDC || !hRC) {
    return 1;
  }

  //Recieve events from this window
  SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

  //Setup the timer
  ticks_per_step = timer.SecondsToTicks(GH_DT);
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

      PeriodicRender(cur_ticks);
      SwapBuffers(hDC);
    }
  }

  return 0;
}

void Engine::EnableVSync() {
  //Attempt to enalbe vsync (if failure then oh well)
  wglSwapIntervalEXT(1);
}

#endif	// _WIN32

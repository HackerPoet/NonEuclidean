#if defined(_WIN32)
	#define _CRT_SECURE_NO_WARNINGS
#endif
#include "Engine.h"

#if defined(_WIN32)

// --- Windows --------------------------------------------------------------

int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow) {
  //Open console in debug mode
#ifdef _DEBUG
  AllocConsole();
  //SetWindowPos(GetConsoleWindow(), 0, 1920, 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  AttachConsole(GetCurrentProcessId());
  freopen("CON", "w", stdout);
#endif

  //Run the main engine
  Engine engine;
  return engine.Run();
}

#else

// --- non-Windows ----------------------------------------------------------

int main(int argc, char** argv) {
	Engine engine;
	return engine.Run();
}

#endif

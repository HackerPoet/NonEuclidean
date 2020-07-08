#define _CRT_SECURE_NO_WARNINGS
#include "Engine.h"

int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow) {
  //Open console in debug mode
#ifdef _DEBUG
  AllocConsole();
  //SetWindowPos(GetConsoleWindow(), 0, 1920, 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  AttachConsole(GetCurrentProcessId());
  freopen("CON", "w", stdout);
#endif

  //Load Meshes files
  if (LoadMeshes() == 0)
	  if (LoadShaders() == 0)
		  if (LoadTextures() == 0)
		  {
			  //Run the main engine
			  Engine engine;
			  return engine.Run();
		  }
		  else
			  return 0;
	  else
		  return 0;
  else
	  return 0;
}

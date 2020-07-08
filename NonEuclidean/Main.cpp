#include "Engine.h"

int APIENTRY WinMain(HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow) {
	//Open console in debug mode
#ifdef _DEBUG
	AllocConsole();
	//SetWindowPos(GetConsoleWindow(), 0, 1920, 200, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	AttachConsole(GetCurrentProcessId());
	errno_t dbg;
	FILE* dummyFile;
	dbg = freopen_s(&dummyFile, "CON", "w", stdout);
#endif

	//Load Meshes files
	if (LoadMeshes() != 0) {
		system("pause");
		return 0;
	}
	if (LoadShaders() != 0) {
		system("pause");
		return 0;
	}
	if (LoadTextures() != 0) {
		system("pause");
		return 0;
	}

	//Run the main engine
	Engine engine;
	return engine.Run();
}
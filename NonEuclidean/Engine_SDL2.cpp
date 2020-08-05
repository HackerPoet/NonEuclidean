#include "Engine.h"
#include "Physical.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "Level4.h"
#include "Level5.h"
#include "Level6.h"
#include "SDL.h"
#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include <algorithm>

// --- SDL2-specific code ------------------------------------------------

bool Engine::InitOSWrapper() {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    return false;
  }

  atexit(SDL_Quit); // SDL will be shut down automatically on app exit
  return true;
}

void Engine::SetupInputs() {
  // not needed
}

void Engine::CreateGLWindow() {
  iWidth = GH_SCREEN_WIDTH;
  iHeight = GH_SCREEN_HEIGHT;

  window = SDL_CreateWindow(
    GH_TITLE,
    GH_SCREEN_X, GH_SCREEN_Y,
    iWidth, iHeight,
    SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI|
      (GH_START_FULLSCREEN ? SDL_WINDOW_FULLSCREEN : 0)
  );
  if (!window) {
    SDL_Log("Unable to create GL window: %s", SDL_GetError());
    return;
  }

  // window successfully created

  glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    SDL_Log("Unable to create GL context: %s", SDL_GetError());
    return;
  }

  // GL context is created and is current for the calling thread

  isWindowGood = true;
}

void Engine::DestroyGLWindow() {
  if (glContext) {
    SDL_GL_DeleteContext(glContext);
  }
  if (window) {
    SDL_DestroyWindow(window);
  }
}

void Engine::EnableVSync() {

  // try adaptive vsync first, if not supported, go full vsync
  if (SDL_GL_SetSwapInterval(-1) != 0) {
    SDL_GL_SetSwapInterval(1);
  }
}

int Engine::EnterMessageLoop() {

  //Setup the timer
  ticks_per_step = timer.SecondsToTicks(GH_DT);
  int64_t cur_ticks = timer.GetTicks();
  GH_FRAME = 0;

  SDL_Event event;
  while(true) {
    if (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        break;
      }
    }
    else {

      input.UpdateRaw();

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
      SwapBuffers();
    }
  }

  return 0;
}

void Engine::ConfineCursor()
{
  // TODO
}

void Engine::SwapBuffers()
{
  SDL_GL_SwapWindow(window);
}

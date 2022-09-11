#include "Engine.h"
#include "Physical.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "Level4.h"
#include "Level5.h"
#include "Level6.h"
#include <SDL2/SDL.h>
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
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  atexit(SDL_Quit); // SDL will be shut down automatically on app exit
  return true;
}

void Engine::SetupInputs() {
  // not needed
}

// from https://github.com/g8kig/LiteXL-PPC
static double query_surface_scale(SDL_Window *window) {
  int w_pixels, h_pixels;
  int w_points, h_points;
  SDL_GL_GetDrawableSize(window, &w_pixels, &h_pixels);
  SDL_GetWindowSize(window, &w_points, &h_points);
  return double(w_pixels) / double(w_points);
}

void Engine::ToggleFullscreen() {
  if (isFullscreen) {
    iWidth=GH_SCREEN_WIDTH;
    iHeight=GH_SCREEN_HEIGHT;
    SDL_SetWindowFullscreen(window,0);
    double scale = query_surface_scale(window);
    SDL_SetWindowSize(window,int(iWidth/scale),int(iHeight/scale));
  }
  else {
    double scale = query_surface_scale(window);
    SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_GetWindowSize(window,&iWidth,&iHeight);
    iWidth=int(iWidth*scale);
    iHeight=int(iHeight*scale);
    SDL_SetWindowSize(window,iWidth,iHeight);
  }
  isFullscreen = !isFullscreen;
}

void Engine::CreateGLWindow() {
  iWidth = GH_SCREEN_WIDTH;
  iHeight = GH_SCREEN_HEIGHT;
  window = SDL_CreateWindow(
    GH_TITLE,
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    iWidth, iHeight,
    SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI|
      (GH_START_FULLSCREEN ? SDL_WINDOW_FULLSCREEN : 0)
  );
    double scale = query_surface_scale(window);
    SDL_SetWindowSize(window,int(iWidth/scale),int(iHeight/scale));
    SDL_SetWindowGrab(window,SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
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
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        return 0;
      }
      else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        auto keycode = event.key.keysym.sym;
        auto mod = event.key.keysym.mod;
        if (keycode == SDLK_ESCAPE) {
          return 0;
        }
        else if (keycode == SDLK_RETURN && (mod & KMOD_LALT || mod & KMOD_RALT)  ) {
          ToggleFullscreen();
        }
        else if (keycode >= 'a' && keycode <= 'z')
        {
          input.key_press[toupper(keycode)] = true;
          input.key[toupper(keycode)] = true;
        }
        else if (keycode >= '0' && keycode <= '9') {
          input.key_press[keycode] = true;
          input.key[keycode] = true;
        }
      }
      else if (event.type == SDL_KEYUP) {
        auto keycode = event.key.keysym.sym;
        if (keycode >= 'a' && keycode <= 'z')
        {
          input.key[toupper(keycode)] = false;
        }
        else if (keycode >= '0' && keycode <= '9') {
          input.key[keycode] = false;
        }
      }
      else if (event.type==SDL_MOUSEMOTION){
        input.UpdateRaw(event.motion.state,event.motion.xrel,event.motion.yrel);
      }
    }
    
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
    SDL_GL_SwapWindow(window);
  }
}

void Engine::ConfineCursor()
{ 
  //not needed
}

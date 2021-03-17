#pragma once

#if defined(_WIN32)

// --- Windows --------------------------------------------------------------

#include <Windows.h>

class Timer {
public:
  Timer() {
    QueryPerformanceFrequency(&frequency);
  }

  void Start() {
    QueryPerformanceCounter(&t1);
  }

  float Stop() {
    QueryPerformanceCounter(&t2);
    return float(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
  }

  int64_t GetTicks() {
    QueryPerformanceCounter(&t2);
    return t2.QuadPart;
  }

  int64_t SecondsToTicks(float s) {
    return int64_t(float(frequency.QuadPart) * s);
  }

  float StopStart() {
    const float result = Stop();
    t1 = t2;
    return result;
  }

private:
  LARGE_INTEGER frequency;        // ticks per second
  LARGE_INTEGER t1, t2;           // ticks
};

#else

// --- non-Windows ----------------------------------------------------------

#include <SDL2/SDL.h>

class Timer {
public:
  Timer() {
    frequency = SDL_GetPerformanceFrequency();
  }

  void Start() {
    t1 = SDL_GetPerformanceCounter();
  }

  float Stop() {
    t2 = SDL_GetPerformanceCounter();
    return float(t2 - t1) / frequency;
  }

  int64_t GetTicks() {
    t2 = SDL_GetPerformanceCounter();
    return t2;
  }

  int64_t SecondsToTicks(float s) {
    return int64_t(float(frequency) * s);
  }

  float StopStart() {
    const float result = Stop();
    t1 = t2;
    return result;
  }

private:
  int64_t frequency;        // ticks per second
  int64_t t1, t2;           // ticks
};

#endif

#pragma once
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

#pragma once

struct tagRAWINPUT;
class Input {
public:
  Input();

  void EndFrame();

#if defined(_WIN32)
  void UpdateRaw(const tagRAWINPUT* raw);
#else
  void UpdateRaw();
#endif

  //Keyboard
  bool key[256];
  bool key_press[256];

  //Mouse
  bool mouse_button[3];
  bool mouse_button_press[3];
  float mouse_dx;
  float mouse_dy;
  float mouse_ddx;
  float mouse_ddy;

  //Joystick
  //TODO:

  //Bindings
  //TODO:

  //Calibration
  //TODO:
};

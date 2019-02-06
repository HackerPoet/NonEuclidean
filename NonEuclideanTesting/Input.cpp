#include "Input.h"
#include "GameHeader.h"
#include <Windows.h>
#include <memory>

Input::Input() {
  memset(this, 0, sizeof(Input));
}

void Input::EndFrame() {
  memset(key_press, 0, sizeof(key_press));
  memset(mouse_button_press, 0, sizeof(mouse_button_press));
  mouse_dx = mouse_dx * GH_MOUSE_SMOOTH + mouse_ddx * (1.0f - GH_MOUSE_SMOOTH);
  mouse_dy = mouse_dy * GH_MOUSE_SMOOTH + mouse_ddy * (1.0f - GH_MOUSE_SMOOTH);
  mouse_ddx = 0.0f;
  mouse_ddy = 0.0f;
}

void Input::UpdateRaw(const tagRAWINPUT* raw) {
  static BYTE buffer[2048];
  static UINT buffer_size = sizeof(buffer);

  if (raw->header.dwType == RIM_TYPEMOUSE) {
    if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
      mouse_ddx += raw->data.mouse.lLastX;
      mouse_ddy += raw->data.mouse.lLastY;
    }
    if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
      mouse_button[0] = true;
      mouse_button_press[0] = true;
    }
    if (raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
      mouse_button[1] = true;
      mouse_button_press[1] = true;
    }
    if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
      mouse_button[2] = true;
      mouse_button_press[2] = true;
    }
    if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) mouse_button[0] = false;
    if (raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) mouse_button[1] = false;
    if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) mouse_button[2] = false;
  } else if (raw->header.dwType == RIM_TYPEHID) {
    //TODO:
  }
}

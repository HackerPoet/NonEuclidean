# NonEuclidean
A NonEuclidean rendering engine for Windows, written in C++ OpenGL.
To see what this code is about, check out this video:
https://youtu.be/kEB11PQ9Eo8

## Controls
* **Mouse** - Look around
* **AWSD** - Movement
* **1 - 7** - Switch between different demo rooms
* **Alt + Enter** - Toggle Fullscreen
* **Esc** - Exit demo

## BEST OF ALL: BLACK SCREEN FIX!
For whoever out there is trying to play this demo but having a black screen (like mine)
Enjoy! :)

(Btw the fix is really easy. Just find the "out vec4 gl_FragColor;" line in these 5 files: pink.frag, portal.frag, sky.frag, texture.frag, texture_array.frag and comment out the line mentioned earlier (which basically means adding "//" in front of the line, for example, "//out vec4 gl_FragColor;"). This fix was found by @multivac3834 at #45. This is just a longer explanation.)

## Thanks
Thank you @ehx-v1 for telling me how to ACTUALLY fix this issue. I also thank @multivac3834 for finding the fix.

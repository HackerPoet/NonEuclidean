# NonEuclidean

A non-Euclidean rendering engine for Windows, written in C++ and OpenGL. This fork adds a WebGL 2 browser port in [`web/`](web/) with all seven original demos.

## Play in a browser

Use Node.js 24.15 or newer on the 24.x line. From the repository root:

```sh
cd web
npx --yes --package=npm@12.2.3 npm ci
npm run dev
```

Open the URL printed by Vite and select an experiment. On desktop, use WASD or the arrow keys to move and the mouse to look. On a touchscreen, drag the joystick to move and drag the scene to look. Each demo has a **Fullscreen** button. Tap **Exit fullscreen** to return to the experiment list on mobile.

For production builds and Vercel setup, see the [web README](web/README.md).

## Windows desktop engine

Add GLEW 2.1.0 to the repository root and build the Visual Studio solution.

- **Mouse** to look around
- **WASD** to move
- **1–7** to switch demos
- **Alt + Enter** to toggle fullscreen
- **Esc** to exit

## Credits

The original engine, demos, meshes, and textures are by CodeParade and are distributed under the [MIT license](LICENSE). This repository is a fork of [HackerPoet/NonEuclidean](https://github.com/HackerPoet/NonEuclidean). See [CodeParade’s original demo video](https://youtu.be/kEB11PQ9Eo8).

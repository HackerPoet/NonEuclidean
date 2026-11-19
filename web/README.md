# Non-Euclidean Engine for the web

A WebGL 2 browser port of [CodeParade’s NonEuclideanEngine](https://github.com/HackerPoet/NonEuclidean). All seven demos use the upstream scene layouts, meshes, textures, portal transforms, and collision shapes. The desktop OpenGL shaders have been adapted to WebGL GLSL. Offscreen portal views use oblique near-plane clipping and nested render targets.

## Run locally

Use Node.js 24.15 or newer on the 24.x line and npm 12.2.3.

```sh
npx --yes --package=npm@12.2.3 npm ci
npm run dev
```

Select an experiment and click **Enter experiment**. Use WASD or the arrow keys to move, the mouse to look, Escape to release the pointer, and keys 1–7 to switch demos. On touchscreens, drag the joystick to move and drag the scene to look. Use **Fullscreen** to fill the screen and **Exit fullscreen** to return to the page. **Pause** releases the touch controls so you can scroll.

## Deploy to Vercel

Import the project with the **Vite** preset. Set **Root Directory** to `web` when deploying from this repository. When deploying the standalone web folder, use the default root. Vercel installs with npm 12.2.3, runs `npm run build`, and serves `dist/`. The app needs no environment variables or backend.

For a local production check, run `npm run build` and then `npm run preview`.

The upstream code and assets are distributed under the MIT license. See [LICENSE](LICENSE).

# Non-Euclidean Engine for the web

A WebGL 2 browser port of [CodeParade’s NonEuclideanEngine](https://github.com/HackerPoet/NonEuclidean). All seven demos use the upstream scene layouts, meshes, textures, portal transforms, and collision shapes. The desktop OpenGL shaders have been adapted to WebGL GLSL. Offscreen portal views use oblique near-plane clipping and nested render targets.

## Run locally

Use Node.js 26.x and npm 12.2.3.

```sh
npx --yes --package=npm@12.2.3 npm ci
npm run dev
```

Select an experiment and click **Enter experiment**. Use WASD or the arrow keys to move, the mouse to look, Escape to release the pointer, and keys 1–7 to switch demos. On touchscreens, drag the joystick to move and drag the scene to look. Use **Fullscreen** to fill the screen and **Exit fullscreen** to return to the page. **Pause** releases the touch controls so you can scroll.

## Deploy to Vercel

Import the project with the **Vite** preset. Set **Root Directory** to `web` when deploying from this repository. When deploying the standalone web folder, use the default root. Vercel installs with npm 12.2.3, runs `npm run build`, and serves `dist/`. The app needs no environment variables or backend.

For a local production check, run `npm run build` and then `npm run preview`.

The upstream code and assets are distributed under the MIT license. See [LICENSE](LICENSE).

## Level editor

Open **Level editor** beside Reset to build directly on a 3D grid. Start on the central plane, then tap or click to place textured floor tiles, walls, and doors. Walls snap to tile edges. With Door selected, click one broad wall face per endpoint. Orbit to attach another endpoint to the opposite face. Both faces can share a pair or connect to different pairs. **Rotate 90°** changes the placement direction. Place two doors to link them. A lone door stays solid until its partner is placed. Removing either endpoint closes the surviving door. Use **Camera** to orbit, pan, or zoom, **Erase** to remove a piece, and **Undo** to reverse a placement or removal. **Playtest** loads the construction into the engine. Reopen the editor to keep building. The construction draft saves on this device.

Use **Export code** to copy a self-contained grid level. In another browser, open the level editor, choose **Import code**, paste the text, and select **Replace draft**. **Undo** restores the previous draft during the same session.

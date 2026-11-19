import { Mesh, Scene } from 'three';
import { createAssets, makeCollider, makeSurface } from './assets.js';
import { buildLevel, levels } from './levels.js';
import { Player } from './player.js';
import { PortalRenderer } from './renderer.js';
import './style.css';

const ui = Object.fromEntries([
  'levels', 'title', 'description', 'scene', 'viewport', 'overlay', 'message', 'enter',
  'reset', 'fullscreen', 'status', 'crossings', 'scale', 'joystick', 'pause', 'exit-fullscreen',
].map(id => [id, document.getElementById(id)]));

const player = new Player();
const assets = createAssets();
const cachedLevels = new Map();
const keys = new Set();
const coarsePointer = matchMedia('(pointer: coarse)');
const movement = { id: null, right: 0, forward: 0 };
let touchMode = false;
let renderer;
let active;
let selected = 0;
let loadVersion = 0;
let playing = false;
let dragging = null;
let dirty = true;
ui.fullscreen.setAttribute('aria-pressed', 'false');

const buttons = levels.map(([name], index) => {
  const button = document.createElement('button');
  button.type = 'button';
  const shortcut = document.createElement('kbd');
  shortcut.textContent = String(index + 1);
  button.append(shortcut, document.createTextNode(name));
  button.addEventListener('click', () => select(index));
  ui.levels.append(button);
  return button;
});

function showStatus(message, ready = false) {
  ui.message.textContent = message;
  ui.message.hidden = !message;
  ui.enter.hidden = !ready;
  ui.overlay.hidden = !message && !ready;
}

function updateReadout() {
  ui.crossings.textContent = player.crossings;
  ui.scale.textContent = `${player.scale.toFixed(2)}×`;
}

async function loadLevel(index) {
  const level = buildLevel(index);
  level.scene = new Scene();
  level.colliders = [];
  const loaded = await Promise.all(level.objects.map(async ({ model, texture: name, matrix }) => {
    const [asset, texture] = await Promise.all([assets.mesh(model), assets.texture(name)]);
    return { asset, texture, name, matrix };
  }));
  for (const { asset, texture, name, matrix } of loaded) {
    const mesh = new Mesh(asset.geometry, makeSurface(texture, name === 'floorplan_textures', matrix));
    mesh.matrixAutoUpdate = false;
    mesh.matrix.copy(matrix);
    mesh.matrixWorldNeedsUpdate = true;
    level.scene.add(mesh);
    level.colliders.push(...asset.collision.map(points => makeCollider(points, matrix)));
  }
  return level;
}

async function select(index) {
  const version = ++loadVersion;
  selected = index;
  active = undefined;
  clearInput();
  for (const [i, button] of buttons.entries()) {
    if (i === index) button.setAttribute('aria-current', 'true');
    else button.removeAttribute('aria-current');
  }
  [ui.title.textContent, ui.description.textContent] = levels[index];
  ui.status.textContent = 'Loading';
  showStatus('Loading experiment…');
  try {
    if (!cachedLevels.has(index)) {
      const pending = loadLevel(index);
      cachedLevels.set(index, pending);
      pending.catch(() => cachedLevels.delete(index));
    }
    const level = await cachedLevels.get(index);
    if (version !== loadVersion) return;
    active = level;
    player.reset(level.spawn);
    updateReadout();
    ui.status.textContent = playing ? 'Exploring' : 'Ready';
    showStatus('', !playing);
    dirty = true;
  } catch (error) {
    if (version !== loadVersion) return;
    console.error(error);
    ui.status.textContent = 'Unavailable';
    showStatus('Could not load this experiment. Select it again to retry.');
  }
}

function enter(event) {
  if (!active) return;
  touchMode = event.pointerType === 'touch' || coarsePointer.matches;
  if (touchMode || !ui.scene.requestPointerLock) {
    start();
    return;
  }
  try {
    const pending = ui.scene.requestPointerLock();
    pending?.catch(start);
  } catch {
    start();
  }
}

function start() {
  playing = true;
  showStatus('');
  ui.status.textContent = 'Exploring';
  ui.viewport.classList.toggle('touch-active', touchMode);
  ui.scene.focus({ preventScroll: true });
}

function pause() {
  playing = false;
  clearInput();
  ui.viewport.classList.remove('touch-active');
  if (active) {
    ui.status.textContent = 'Paused';
    showStatus('', true);
  }
}

function immersive(enabled) {
  ui.viewport.classList.toggle('immersive', enabled);
  document.body.classList.toggle('immersive-open', enabled);
  ui.fullscreen.setAttribute('aria-pressed', String(enabled));
  ui['exit-fullscreen'].hidden = !enabled;
  (enabled ? ui['exit-fullscreen'] : ui.fullscreen).focus({ preventScroll: true });
}

async function fullscreen() {
  if (document.fullscreenElement === ui.viewport) {
    await document.exitFullscreen();
    return;
  }
  if (ui.viewport.classList.contains('immersive')) {
    immersive(false);
    return;
  }
  if (document.fullscreenEnabled && ui.viewport.requestFullscreen) {
    try {
      await ui.viewport.requestFullscreen();
      return;
    } catch {
      // Some mobile browsers restrict fullscreen to video or disallow it in embeds.
    }
  }
  immersive(true);
}

function reset() {
  if (!active) return;
  player.reset(active.spawn);
  updateReadout();
  dirty = true;
}

function onKey(event) {
  if (event.altKey && event.code === 'Enter') {
    if (!event.repeat && !ui.fullscreen.disabled) fullscreen();
    event.preventDefault();
    return;
  }
  if (event.code === 'Escape' && ui.viewport.classList.contains('immersive')) {
    immersive(false);
    return;
  }
  if (event.code === 'Escape' && !document.pointerLockElement && playing) {
    pause();
    ui.enter.focus({ preventScroll: true });
  }
  if (event.altKey || event.ctrlKey || event.metaKey) return;
  if (event.code.match(/^Digit[1-7]$/)) {
    if (renderer && !event.repeat) select(Number(event.code.at(-1)) - 1);
    return;
  }
  if (!playing || !/^(Key[WASD]|Arrow(Up|Down|Left|Right))$/.test(event.code)) return;
  keys.add(event.code);
  event.preventDefault();
}

function look(dx, dy) {
  player.look(dx * 0.003, dy * 0.003);
  dirty = true;
}

function wireControls() {
  ui.enter.addEventListener('click', enter);
  ui.scene.addEventListener('click', event => {
    if (!playing) enter(event);
  });
  ui.pause.addEventListener('click', () => {
    pause();
    ui.enter.focus({ preventScroll: true });
  });
  ui.reset.addEventListener('click', reset);
  ui['exit-fullscreen'].addEventListener('click', fullscreen);
  document.addEventListener('fullscreenchange', () => {
    const enabled = document.fullscreenElement === ui.viewport;
    ui.fullscreen.setAttribute('aria-pressed', String(enabled));
    ui['exit-fullscreen'].hidden = !enabled;
    if (enabled) ui['exit-fullscreen'].focus({ preventScroll: true });
    else if (!document.pointerLockElement) ui.fullscreen.focus({ preventScroll: true });
  });
  ui.fullscreen.addEventListener('click', fullscreen);
  document.addEventListener('keydown', onKey);
  document.addEventListener('keyup', event => keys.delete(event.code));
  window.addEventListener('blur', clearInput);
  document.addEventListener('pointerlockchange', () => {
    if (document.pointerLockElement === ui.scene) start();
    else pause();
  });
  document.addEventListener('pointerlockerror', () => {
    if (active && !playing) start();
  });
  document.addEventListener('mousemove', event => {
    if (document.pointerLockElement === ui.scene) look(event.movementX, event.movementY);
  });
  ui.scene.addEventListener('pointerdown', event => {
    if (!active || !playing || dragging || document.pointerLockElement) return;
    ui.scene.setPointerCapture(event.pointerId);
    dragging = { id: event.pointerId, x: event.clientX, y: event.clientY };
  });
  ui.scene.addEventListener('pointermove', event => {
    if (dragging?.id !== event.pointerId) return;
    look(event.clientX - dragging.x, event.clientY - dragging.y);
    dragging.x = event.clientX;
    dragging.y = event.clientY;
  });
  for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
    ui.scene.addEventListener(type, event => {
      if (dragging?.id === event.pointerId) dragging = null;
    });
    ui.joystick.addEventListener(type, event => {
      if (movement.id === event.pointerId) stopMovement();
    });
  }
  ui.joystick.addEventListener('pointerdown', event => {
    if (!active || !playing || movement.id !== null) return;
    ui.joystick.setPointerCapture(event.pointerId);
    movement.id = event.pointerId;
    moveJoystick(event);
  });
  ui.joystick.addEventListener('pointermove', event => {
    if (movement.id === event.pointerId) moveJoystick(event);
  });
}

function moveJoystick(event) {
  const { left, top, width, height } = ui.joystick.getBoundingClientRect();
  const radius = width * 0.3;
  const x = (event.clientX - left - width / 2) / radius;
  const y = (event.clientY - top - height / 2) / radius;
  const distance = Math.hypot(x, y);
  const limit = Math.max(1, distance);
  movement.right = distance < 0.14 ? 0 : x / limit;
  movement.forward = distance < 0.14 ? 0 : -y / limit;
  ui.joystick.style.setProperty('--x', (movement.right * radius) + 'px');
  ui.joystick.style.setProperty('--y', (-movement.forward * radius) + 'px');
}

function stopMovement() {
  movement.id = null;
  movement.right = movement.forward = 0;
  ui.joystick.style.removeProperty('--x');
  ui.joystick.style.removeProperty('--y');
}

function clearInput() {
  keys.clear();
  dragging = null;
  stopMovement();
}

function animate(timestamp) {
  const dt = Math.min((timestamp - (animate.previous ?? timestamp)) / 1000, 0.064);
  animate.previous = timestamp;
  if (active && playing && dt > 0) {
    const right = Number(keys.has('KeyD') || keys.has('ArrowRight'))
      - Number(keys.has('KeyA') || keys.has('ArrowLeft')) + movement.right;
    const forward = Number(keys.has('KeyW') || keys.has('ArrowUp'))
      - Number(keys.has('KeyS') || keys.has('ArrowDown')) + movement.forward;
    const previous = player.position.clone();
    const crossings = player.crossings;
    const steps = Math.ceil(dt / 0.004);
    for (let i = 0; i < steps; i++) player.step(dt / steps, right, forward, active);
    if (player.crossings !== crossings) updateReadout();
    dirty ||= previous.distanceToSquared(player.position) > 1e-12;
  }
  if (active && dirty) {
    renderer.draw(active, player);
    dirty = false;
  }
  requestAnimationFrame(animate);
}

try {
  renderer = new PortalRenderer(ui.scene);
  wireControls();
  new ResizeObserver(([entry]) => {
    const { width, height } = entry.contentRect;
    if (width < 1 || height < 1) return;
    renderer.resize(width, height);
    dirty = true;
  }).observe(ui.viewport);
  select(selected);
  requestAnimationFrame(animate);
} catch (error) {
  console.error(error);
  ui.status.textContent = 'WebGL unavailable';
  showStatus('This experiment requires a browser with WebGL 2 enabled.');
  for (const button of [...buttons, ui.reset, ui.fullscreen]) button.disabled = true;
}

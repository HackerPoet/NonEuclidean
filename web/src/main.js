import { Mesh, Scene } from 'three';
import { createAssets, makeCollider, makeSurface } from './assets.js';
import { buildLevel, levels } from './levels.js';
import { Player } from './player.js';
import { PortalRenderer } from './renderer.js';
import './style.css';

const ui = Object.fromEntries([
  'levels', 'title', 'description', 'scene', 'viewport', 'overlay', 'message', 'enter',
  'reset', 'fullscreen', 'status', 'crossings', 'scale', 'touch-controls',
].map(id => [id, document.getElementById(id)]));

const player = new Player();
const assets = createAssets();
const cachedLevels = new Map();
const keys = new Set();
const touch = matchMedia('(pointer: coarse)').matches;
let renderer;
let active;
let selected = 0;
let loadVersion = 0;
let playing = false;
let dragging = null;
let dirty = true;
ui.fullscreen.disabled = !document.fullscreenEnabled;

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
  keys.clear();
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

function enter() {
  if (!active) return;
  if (touch || !ui.scene.requestPointerLock) {
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
  ui.viewport.classList.toggle('touch-active', touch);
  ui.scene.focus({ preventScroll: true });
}

function pause() {
  playing = false;
  keys.clear();
  ui.viewport.classList.remove('touch-active');
  if (active) {
    ui.status.textContent = 'Paused';
    showStatus('', true);
  }
}

async function fullscreen() {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await ui.viewport.requestFullscreen();
  } catch {
    ui.status.textContent = 'Fullscreen unavailable';
  }
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
  ui.scene.addEventListener('click', () => {
    if (!playing) enter();
  });
  ui.reset.addEventListener('click', reset);
  ui.fullscreen.addEventListener('click', fullscreen);
  document.addEventListener('keydown', onKey);
  document.addEventListener('keyup', event => keys.delete(event.code));
  window.addEventListener('blur', () => keys.clear());
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
    if (!playing || document.pointerLockElement) return;
    ui.scene.setPointerCapture(event.pointerId);
    dragging = { id: event.pointerId, x: event.clientX, y: event.clientY };
  });
  ui.scene.addEventListener('pointermove', event => {
    if (dragging?.id !== event.pointerId) return;
    look(event.clientX - dragging.x, event.clientY - dragging.y);
    dragging.x = event.clientX;
    dragging.y = event.clientY;
  });
  ui.scene.addEventListener('lostpointercapture', () => { dragging = null; });
  for (const button of ui['touch-controls'].querySelectorAll('button')) {
    const key = button.dataset.key;
    button.addEventListener('pointerdown', event => {
      button.setPointerCapture(event.pointerId);
      keys.add(key);
    });
    button.addEventListener('lostpointercapture', () => keys.delete(key));
  }
}

function animate(timestamp) {
  const dt = Math.min((timestamp - (animate.previous ?? timestamp)) / 1000, 0.064);
  animate.previous = timestamp;
  if (active && playing && dt > 0) {
    const right = Number(keys.has('KeyD') || keys.has('ArrowRight'))
      - Number(keys.has('KeyA') || keys.has('ArrowLeft'));
    const forward = Number(keys.has('KeyW') || keys.has('ArrowUp'))
      - Number(keys.has('KeyS') || keys.has('ArrowDown'));
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

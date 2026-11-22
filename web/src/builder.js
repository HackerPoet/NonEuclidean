import { AmbientLight, BoxGeometry, Color, DirectionalLight, GridHelper, Group, Matrix4, Mesh,
  MeshLambertMaterial, MeshBasicMaterial, OrthographicCamera, Plane, Raycaster, Scene,
  Vector2, Vector3, WebGLRenderer } from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const key = 'nonEuclidean.builder.v1';
const models = { floor: 'build_floor', wall: 'build_wall', door: 'tunnel' };
const textures = { floor: 'checker_green', wall: 'checker_gray', door: 'gold' };

function sameCell(a, b) {
  return a.x === b.x && a.z === b.z && (a.height ?? 0) === (b.height ?? 0);
}

// The wall face is 0.08 units from its center. Bias the decal to avoid z-fighting.
function facePosition(piece) {
  const yaw = piece.turn * Math.PI / 2;
  const offset = piece.mounted ? 0.082 : 0;
  return new Vector3(piece.x - Math.sin(yaw) * offset, piece.height ?? 0, piece.z - Math.cos(yaw) * offset);
}

function matrix(piece) {
  const scale = piece.type === 'door' && !piece.mounted ? new Vector3(1.25, 1.2, 0.11) : new Vector3(1, 1, 1);
  const transform = new Matrix4().makeRotationY(piece.turn * Math.PI / 2);
  const position = facePosition(piece);
  if (piece.type === 'floor') {
    const rise = piece.rise ?? 0;
    transform.multiply(new Matrix4().makeRotationX(-Math.atan(rise / 2)));
    scale.z = Math.hypot(1, rise / 2);
    position.y += rise / 2 + 0.008;
  }
  if (piece.type === 'door') scale.multiplyScalar(piece.size ?? 1);
  return transform.scale(scale).setPosition(position);
}

export function buildDraft(pieces) {
  const objects = [{ model: 'ground', texture: 'checker_green', matrix: new Matrix4().makeScale(3, 1, 3) }];
  const portals = [];
  const pairs = new Map();
  for (const [pieceIndex, piece] of pieces.entries()) {
    objects.push({ pieceIndex, model: piece.mounted ? 'wall_frame' : models[piece.type], texture: textures[piece.type], matrix: matrix(piece) });
    if (piece.type !== 'door') continue;
    const size = piece.size ?? 1;
    const halfHeight = (piece.mounted ? 1.1 : 1.2) * size;
    const position = facePosition(piece).add(new Vector3(0, halfHeight, 0));
    const transform = new Matrix4().makeRotationY(piece.turn * Math.PI / 2)
      .scale(new Vector3(0.75 * size, halfHeight, size)).setPosition(position);
    const portal = { mounted: !!piece.mounted, pieceIndex, position, matrix: transform, inverse: transform.clone().invert(),
      normal: new Vector3(0, 0, -1).transformDirection(transform), sides: {} };
    const mate = pairs.get(piece.pair);
    if (!mate) { pairs.set(piece.pair, portal); continue; }
    for (const [entry, exit] of [[portal, mate], [mate, portal]]) {
      const transfer = exit.matrix.clone();
      if (exit.mounted) transfer.multiply(new Matrix4().makeRotationY(Math.PI));
      const warp = { exit, matrix: transfer.multiply(entry.inverse), exitSign: exit.mounted ? 1 : -1 };
      entry.sides = entry.mounted ? { front: warp } : { front: warp, back: warp };
      portals.push(entry);
    }
    pairs.delete(piece.pair);
  }
  // A single endpoint is a solid door until its partner exists.
  for (const door of pairs.values()) {
    const piece = pieces[door.pieceIndex];
    if (piece.mounted) continue; // The host wall already seals this endpoint.
    objects.push({ pieceIndex: door.pieceIndex, model: 'build_wall', texture: textures.wall,
      matrix: new Matrix4().makeRotationY(piece.turn * Math.PI / 2)
        .scale(new Vector3(0.75, 1, 1)).setPosition(facePosition(piece)) });
  }
  return { objects, portals, spawn: [0, 1.5, 2] };
}

export function mountBuilder(assets, play, pause) {
  const dialog = document.querySelector('#builder');
  const canvas = dialog.querySelector('canvas');
  const status = dialog.querySelector('output');
  let pieces = [];
  try { pieces = JSON.parse(localStorage.getItem(key)) ?? []; } catch { /* New draft */ }
  let tool = 'floor', turn = 0, engine, controls;
  const height = dialog.querySelector('#tile-height input');
  const rise = dialog.querySelector('#tile-rise input');
  const size = dialog.querySelector('#door-scale input');
  const history = [];
  const scene = new Scene();
  scene.background = new Color(0x212a25);
  const camera = new OrthographicCamera(-10, 10, 8, -8, 0.1, 500);
  camera.position.set(12, 15, 14);
  const placed = new Group();
  const grid = new GridHelper(200, 100, 0x6e8767, 0x445347);
  grid.position.set(1, 0.02, 1);
  scene.add(placed, grid, new AmbientLight(0xffffff, 1.7));
  const sun = new DirectionalLight(0xffffff, 2);
  sun.position.set(4, 10, 5);
  scene.add(sun);
  const resources = {};
  const ghost = new Mesh(new BoxGeometry(1, 1, 1), new MeshBasicMaterial({
    color: 0xbbff81, opacity: 0.42, transparent: true, depthWrite: false,
  }));
  ghost.visible = false;
  scene.add(ghost);
  function redraw() {
    if (dialog.open && engine) engine.render(scene, camera);
  }
  function populate() {
    placed.clear();
    const layout = buildDraft(pieces);
    for (const object of layout.objects) {
      const { geometry, material } = resources[object.model];
      const mesh = new Mesh(geometry, material);
      mesh.userData.pieceIndex = object.pieceIndex;
      mesh.applyMatrix4(object.matrix);
      placed.add(mesh);
    }
    redraw();
  }
  let loading;
  async function init() {
    engine = new WebGLRenderer({ canvas, antialias: true });
    engine.setPixelRatio(Math.min(devicePixelRatio, 1.5));
    controls = new OrbitControls(camera, canvas);
    controls.enabled = tool === "orbit";
    controls.maxPolarAngle = Math.PI * 0.47;
    controls.minZoom = 0.25; controls.maxZoom = 6;
    controls.addEventListener('change', redraw);
    await Promise.all(Object.entries({ ground: 'checker_green', wall_frame: 'gold', ...Object.fromEntries(
      Object.keys(models).map(type => [models[type], textures[type]])) }).map(async ([model, texture]) => {
      const [asset, map] = await Promise.all([assets.mesh(model), assets.texture(texture)]);
      resources[model] = { geometry: asset.geometry, material: new MeshLambertMaterial({ map }) };
    }));
    populate();
  }
  new ResizeObserver(() => {
    if (!dialog.open || !engine || !canvas.clientHeight) return;
    const ratio = canvas.clientWidth / canvas.clientHeight;
    camera.left = -8 * ratio; camera.right = 8 * ratio;
    camera.updateProjectionMatrix();
    engine.setSize(canvas.clientWidth, canvas.clientHeight, false);
    redraw();
  }).observe(canvas);
  document.querySelector('#edit').onclick = async () => {
    pause(); dialog.showModal();
    try { loading ??= init(); await loading; redraw(); }
    catch { status.textContent = 'Could not load the building view. Reload to retry.'; }
  };
  function save() {
    try { localStorage.setItem(key, JSON.stringify(pieces)); }
    catch { status.textContent = 'Device storage unavailable'; }
    dialog.querySelector('[data-action=undo]').disabled = !history.length;
    populate();
  }
  function point(event) {
    const rect = canvas.getBoundingClientRect();
    const pointer = new Vector2(2 * (event.clientX - rect.left) / rect.width - 1,
      1 - 2 * (event.clientY - rect.top) / rect.height);
    const ray = new Raycaster();
    ray.setFromCamera(pointer, camera);
    if (tool === 'door') {
      const hit = ray.intersectObjects(placed.children)[0];
      const wall = pieces[hit?.object.userData.pieceIndex];
      if (wall?.type === 'door' && wall.mounted) return { ...wall, size: size.valueAsNumber };
      if (wall?.type !== 'wall' || Math.abs(hit.face.normal.z) < 0.99) return;
      const facing = (wall.turn + (hit.face.normal.z > 0 ? 2 : 0)) % 4;
      return { type: 'door', mounted: true, height: wall.height ?? 0, size: size.valueAsNumber, x: wall.x, z: wall.z, turn: facing };
    }
    const placementHeight = tool === 'wall' || tool === 'floor' ? height.valueAsNumber : 0;
    const hit = ray.ray.intersectPlane(new Plane(new Vector3(0, 1, 0), -placementHeight), new Vector3());
    if (!hit) return;
    const edge = tool === 'wall' || tool === 'door';
    const dx = edge && turn % 2 ? 1 : 0, dz = edge && !(turn % 2) ? 1 : 0;
    return { x: Math.round((hit.x - dx) / 2) * 2 + dx,
      z: Math.round((hit.z - dz) / 2) * 2 + dz, type: tool, turn, height: height.valueAsNumber, rise: tool === 'floor' ? rise.valueAsNumber : 0 };
  }
  function preview(event) {
    const piece = point(event);
    ghost.visible = !!piece && !['orbit', 'erase'].includes(tool);
    if (ghost.visible) {
      const geometry = resources[piece.mounted ? 'wall_frame' : models[tool]]?.geometry;
      if (geometry) ghost.geometry = geometry;
      ghost.matrixAutoUpdate = false;
      ghost.matrix.copy(matrix(piece));
      ghost.matrix.elements[13] += 0.012;
      ghost.matrixWorldNeedsUpdate = true;
    }
    redraw();
  }
  canvas.addEventListener('pointermove', preview);
  canvas.addEventListener('pointerleave', () => { ghost.visible = false; redraw(); });
  canvas.addEventListener('pointerdown', event => {
    if (!resources.wall_frame || !resources.ground || !resources.build_wall || !resources.tunnel || !resources.build_floor || tool === 'orbit' || event.button !== 0) return;
    const piece = point(event);
    if (!piece) return;
    const inputs = tool === 'door' ? [size] : tool === 'floor' ? [height, rise] : tool === 'wall' ? [height] : [];
    if (!inputs.every(input => input.reportValidity())) return;
    let index;
    if (tool === 'erase') {
      const rect = canvas.getBoundingClientRect();
      const ray = new Raycaster();
      ray.setFromCamera(new Vector2(2*(event.clientX-rect.left)/rect.width-1,
        1-2*(event.clientY-rect.top)/rect.height), camera);
      const hit = ray.intersectObjects(placed.children.slice(1))[0];
      if (!hit) return;
      index = hit.object.userData.pieceIndex;
      const wall = pieces[index];
      if (wall.type === 'wall' && Math.abs(hit.face.normal.z) > 0.99) {
        const facing = (wall.turn + (hit.face.normal.z > 0 ? 2 : 0)) % 4;
        const endpoint = pieces.findIndex(p => p.mounted && sameCell(p, wall) && p.turn === facing);
        if (endpoint >= 0) index = endpoint;
      }
    } else if (tool === 'door') {
      const existing = pieces.findIndex(p => p.mounted && sameCell(p, piece) && p.turn === piece.turn);
      if (existing >= 0) {
        if ((pieces[existing].size ?? 1) !== piece.size) {
          history.push(pieces.slice());
          pieces[existing] = { ...pieces[existing], size: piece.size };
          save();
        }
        status.textContent = 'Door pair ' + pieces[existing].pair + ' - endpoint size ' + piece.size + 'x';
        return;
      }
      index = -1;
    } else {
      index = pieces.findIndex(p => !p.mounted && sameCell(p, piece));
      if (index >= 0 && pieces[index].type === tool && pieces[index].turn === turn && (pieces[index].rise ?? 0) === piece.rise) return;
    }
    history.push(pieces.slice());
    if (index >= 0) {
      const [removed] = pieces.splice(index, 1);
      if (removed.type === 'wall') pieces = pieces.filter(p => !p.mounted || !sameCell(p, removed));
    }
    if (tool !== 'erase') {
      if (tool === 'door') {
        const counts = new Map();
        for (const p of pieces.filter(p => p.type === 'door' && p.mounted)) counts.set(p.pair, (counts.get(p.pair) ?? 0) + 1);
        piece.pair = [...counts].find(([, n]) => n === 1)?.[0] ?? Math.max(0, ...pieces.filter(p => p.type === 'door').map(p => p.pair)) + 1;
        status.textContent = counts.has(piece.pair) ? 'Door pair ' + piece.pair + ' linked' : 'Place the other end of door pair ' + piece.pair;
      }
      pieces.push(piece);
    }
    save();
  });
  dialog.onclick = event => {
    const button = event.target.closest('button');
    if (!button) return;
    if (button.dataset.tool) {
      tool = button.dataset.tool;
      height.parentElement.hidden = !['wall', 'floor'].includes(tool);
      rise.parentElement.hidden = tool !== 'floor';
      size.parentElement.hidden = tool !== 'door';
      if (controls) controls.enabled = tool === 'orbit';
      for (const b of dialog.querySelectorAll('[data-tool]')) b.setAttribute('aria-pressed', String(b === button));
      ghost.visible = false; redraw();
      status.textContent = tool === 'door' ? 'Set endpoint size (0.1-1), then click a wall face or an existing door.' : tool === 'orbit' ? 'Drag to orbit. Pinch or scroll to zoom. Right-drag or use two fingers to pan.' : tool === 'floor' ? 'Set base height and rise, rotate the uphill direction, then click a tile.' : 'Tap the grid to ' + (tool === 'erase' ? 'erase a piece' : 'place a ' + tool);
    }
    switch (button.dataset.action) {
      case 'rotate': turn = (turn + 1) % 4; status.textContent = 'Placement rotation ' + turn * 90 + '°'; ghost.visible = false; redraw(); break;
      case 'undo': if (history.length) { pieces = history.pop(); save(); } break;
      case 'close': dialog.close(); break;
      case 'play': if (resources.ground) { dialog.close(); play(buildDraft(pieces)); } break;
    }
  };
}

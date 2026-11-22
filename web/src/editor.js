import { Matrix4, Vector3 } from 'three';
import { buildLevel, levels } from './levels.js';

const storageKey = 'nonEuclidean.editor.v1';
const pose = (x, y, z, angle, size) => new Matrix4().makeRotationY(angle * Math.PI / 180)
  .multiply(new Matrix4().makeScale(size, size, size)).setPosition(x, y, z);

export function compileLayout(parts) {
  const layout = { objects: [], portals: [], spawn: [0, 1.5, 5] };
  parts.forEach((part, index) => {
    const level = buildLevel(part.demo);
    const group = pose(...part.pose);
    if (!index) layout.spawn = new Vector3(...level.spawn).applyMatrix4(group).toArray();
    for (const object of level.objects) {
      object.matrix.premultiply(group);
      layout.objects.push(object);
    }
    for (const [i, door] of level.portals.entries()) {
      const [x, y, z, angle, size] = part.doors[i].pose;
      door.matrix.premultiply(pose(0, 0, 0, angle, size));
      door.position.add(new Vector3(x, y, z));
      door.matrix.setPosition(door.position).premultiply(group);
      door.position.setFromMatrixPosition(door.matrix);
      door.inverse.copy(door.matrix).invert();
      door.normal.set(0, 0, -1).transformDirection(door.matrix);
      layout.portals.push(door);
    }
  });
  const destinations = parts.flatMap(part => part.doors.map(door => door.to));
  destinations.forEach((destination, i) => {
    const a = layout.portals[i];
    const b = layout.portals[destination];
    a.sides.front = { exit: b, matrix: b.matrix.clone().multiply(a.inverse) };
    b.sides.back = { exit: a, matrix: a.matrix.clone().multiply(b.inverse) };
  });
  return layout;
}

export function mountEditor(play) {
  const dialog = document.querySelector('#editor');
  const find = id => dialog.querySelector('#' + id);
  const template = find('template');
  const partList = find('parts');
  const doorList = find('doors');
  const target = find('destination');
  const fields = find('pose');
  const notice = find('editor-status');
  let parts = [];
  let partIndex = 0;
  let doorIndex = -1;
  const allDoors = () => parts.flatMap(part => part.doors);
  const options = (select, names) => select.replaceChildren(...names.map((name, i) => new Option(name, i)));
  options(template, levels.map(([name]) => name));
  function append(demo) {
    const level = buildLevel(demo);
    const offset = allDoors().length;
    parts.push({ demo, pose: [parts.length * 25, 0, 0, 0, 1], doors: level.portals.map(door => ({
      pose: [0, 0, 0, 0, 1], to: offset + level.portals.indexOf(door.sides.front.exit),
    })) });
    partIndex = parts.length - 1;
    doorIndex = -1;
  }
  function save() {
    try { localStorage.setItem(storageKey, JSON.stringify(parts)); notice.textContent = 'Saved on this device'; }
    catch { notice.textContent = 'Device storage unavailable'; }
  }
  function render() {
    const part = parts[partIndex];
    options(partList, parts.map((item, i) => (i + 1) + '. ' + levels[item.demo][0]));
    partList.value = partIndex;
    options(doorList, ['Whole section', ...part.doors.map((_, i) => 'Door ' + (i + 1))]);
    doorList.value = doorIndex + 1;
    const door = part.doors[doorIndex];
    const values = (door ?? part).pose;
    Array.from(fields.querySelectorAll('input')).forEach((input, i) => { input.value = values[i]; });
    target.parentElement.hidden = !door;
    if (door) {
      options(target, parts.flatMap((item, p) => item.doors.map((_, d) => 'Section ' + (p + 1) + ' · Door ' + (d + 1))));
      target.value = door.to;
    }
    find('remove').disabled = parts.length === 1;
  }
  try {
    const saved = JSON.parse(localStorage.getItem(storageKey));
    if (saved?.length) { compileLayout(saved); parts = saved; }
  } catch { /* Start with a fresh draft if stored data is unreadable. */ }
  if (!parts.length) append(0);
  document.querySelector('#advanced-editor').onclick = () => { document.querySelector('#builder').close(); render(); dialog.showModal(); };
  find('close').onclick = () => dialog.close();
  find('add').onclick = () => { append(Number(template.value)); save(); render(); };
  partList.onchange = () => { partIndex = Number(partList.value); doorIndex = -1; render(); };
  doorList.onchange = () => { doorIndex = Number(doorList.value) - 1; render(); };
  target.onchange = () => {
    const door = parts[partIndex].doors[doorIndex];
    const destination = Number(target.value);
    const previous = allDoors().find(other => other.to === destination);
    previous.to = door.to;
    door.to = destination;
    save();
  };
  find('remove').onclick = () => {
    const old = allDoors();
    const removed = new Set(parts.splice(partIndex, 1)[0].doors);
    const remaining = allDoors();
    const next = remaining.map(door => {
      let exit = old[door.to];
      while (removed.has(exit)) exit = old[exit.to];
      return remaining.indexOf(exit);
    });
    remaining.forEach((door, i) => { door.to = next[i]; });
    partIndex = 0; doorIndex = -1;
    save(); render();
  };
  for (const [i, label] of ['X offset', 'Height offset', 'Z offset', 'Rotation (°)', 'Scale'].entries()) {
    const wrapper = document.createElement('label');
    const input = document.createElement('input');
    input.type = 'number'; input.step = 'any'; input.required = true;
    if (i === 4) input.min = '0.01';
    wrapper.append(label, input); fields.append(wrapper);
    input.onchange = () => {
      const part = parts[partIndex];
      const values = (part.doors[doorIndex] ?? part).pose;
      if (input.reportValidity()) { values[i] = input.valueAsNumber; save(); }
      else input.value = values[i];
    };
  }
  find('play-layout').onclick = () => { dialog.close(); play(compileLayout(parts)); };
}

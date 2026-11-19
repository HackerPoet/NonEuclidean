import { Vector3, Matrix4 } from 'three';

export const levels = [
  ['Unequal tunnels', 'Two tunnels trade entrances. Follow one through to find out where it ends.'],
  ['Three rooms', 'A shortcut joins two doorways inside the same square building.'],
  ['Six rooms', 'Doorways connect two distant buildings in a three-way loop.'],
  ['Around the pillar', 'Circle the pillar and watch the sculpture change.'],
  ['Endless descent', 'Follow the sloped hallway to keep walking downhill.'],
  ['Changing scale', 'Walk through both ends of the tapering tunnel, then try the small doorway.'],
  ['Impossible floor plan', 'Two portal loops rearrange a single floor plan as you walk.'],
];

const pi = Math.PI;
export const eyeHeight = 1.5;

export function buildLevel(index) {
  const objects = [];
  const portals = [];
  let spawn;
  function object(model, texture, pos = [0, 0, 0], scale = [1, 1, 1], yaw = 0) {
    const matrix = new Matrix4().makeRotationY(yaw).scale(new Vector3(...scale))
      .setPosition(...pos);
    objects.push({ model, texture, matrix });
    return matrix;
  }
  function portal(position, yaw, scale) {
    const matrix = new Matrix4().makeRotationY(yaw).scale(new Vector3(...scale))
      .setPosition(position);
    const inverse = matrix.clone().invert();
    // The upstream engine defines the front as local -Z.
    const normal = new Vector3(0, 0, -1).transformDirection(matrix);
    const entry = { position, matrix, inverse, normal, sides: {} };
    portals.push(entry);
    return entry;
  }
  function link(a, sideA, b, sideB) {
    a.sides[sideA] = { exit: b, matrix: b.matrix.clone().multiply(a.inverse) };
    b.sides[sideB] = { exit: a, matrix: a.matrix.clone().multiply(b.inverse) };
  }
  function pair(a, b) {
    link(a, 'front', b, 'back');
    link(b, 'front', a, 'back');
  }
  function cycle(a, b, c) {
    link(a, 'front', b, 'back');
    link(b, 'front', c, 'back');
    link(c, 'front', a, 'back');
  }
  function ground(x, size, slope = false, yaw = 0) {
    object(slope ? 'ground_slope' : 'ground', 'checker_green', [x, 0, 0],
      [size, slope ? 2 : size / 10, size], yaw);
  }
  function tunnel(model, pos, length, yaw = 0, size = 1, doorYaw = yaw) {
    const transform = object(model, 'checker_gray', pos, [size, size, length], yaw);
    const narrow = model === 'tunnel_scale';
    const slope = model === 'tunnel_slope';
    const first = new Vector3(0, 1, 1).applyMatrix4(transform);
    const second = new Vector3(0, narrow ? 0.5 : slope ? -1 : 1, -1).applyMatrix4(transform);
    return [
      portal(first, doorYaw, [0.6 * size, 0.999 * size, size]),
      portal(second, doorYaw, narrow ? [0.3, 0.499, 0.5] : [0.6 * size, 0.999 * size, size]),
    ];
  }
  function house(x, texture) {
    const transform = object('square_rooms', texture, [x, 0, -20], [1, 3, 1]);
    return (door) => {
      const [px, pz] = [[4, 10], [10, 4], [16, 10], [10, 16]][door];
      return portal(new Vector3(px, 0.5, pz).applyMatrix4(transform), -door * pi / 2, [2, 1.5, 1]);
    };
  }

  switch (index) {
    case 0: {
      const [a, b] = tunnel('tunnel', [-2.4, 0, -1.8], 4.8);
      const [c, d] = tunnel('tunnel', [2.4, 0, 0], 0.6);
      pair(a, c);
      pair(b, d);
      ground(0, 12);
      spawn = [0, eyeHeight, 5];
      break;
    }
    case 1:
    case 2: {
      const first = house(0, 'three_room');
      if (index === 1) {
        pair(first(2), first(3));
      } else {
        const second = house(200, 'three_room2');
        cycle(first(3), second(2), second(0));
      }
      spawn = [3, eyeHeight, 3];
      break;
    }
    case 3: {
      const exhibits = [
        ['teapot', 0.5, 0.5, pi / 2],
        ['bunny', -0.4, 14, pi],
        ['suzanne', 0.9, 1.2, pi],
      ];
      exhibits.forEach(([model, y, size, yaw], i) => {
        const x = i * 200;
        object('pillar', 'white', [x, 0, 0], [0.1, 0.1, 0.1]);
        object('pillar_room', 'three_room', [x, 0, 0], [1.1, 1.1, 1.1]);
        object(model, 'gold', [x, y, 9], [size, size, size], yaw);
        ground(x, 20);
        portal(new Vector3(x, 1.65, -1.1), -pi / 2, [1.1, 1.65, 1.1]);
      });
      cycle(...portals);
      spawn = [0, eyeHeight, 3];
      break;
    }
    case 4: {
      const [a, b] = tunnel('tunnel_slope', [0, 0, 0], 5, pi);
      const [c, d] = tunnel('tunnel_slope', [200, 0, 0], 5, 0, 1, -pi);
      pair(a, d);
      pair(b, c);
      ground(0, 10, true);
      ground(200, 10, true, pi);
      spawn = [0, eyeHeight - 2, 8];
      break;
    }
    case 5: {
      const [a, b] = tunnel('tunnel_scale', [-1.2, 0, 0], 2.4);
      const [c, d] = tunnel('tunnel', [201.2, 0, 0], 2.4);
      pair(a, c);
      pair(b, d);
      ground(0, 12);
      ground(200, 12);
      object('tunnel', 'checker_gray', [-1, 0, -4.2], [0.25, 0.25, 0.6], pi / 2);
      spawn = [0, eyeHeight, 5];
      break;
    }
    case 6: {
      const unit = 0.1524;
      object('floorplan', 'floorplan_textures', [0, 0, 0], [unit, unit, unit]);
      const positions = [[33, 25.5], [74, 25.5], [33, 66.5], [63.5, 48], [63.5, 7], [22.5, 48]];
      positions.forEach(([x, z], i) => portal(new Vector3(x, 10, z).multiplyScalar(unit),
        i < 3 ? 0 : pi / 2, [4 * unit, 10 * unit, unit]));
      const [a, b, c, d, e, f] = portals;
      cycle(a, c, b);
      cycle(d, f, e);
      spawn = [2, eyeHeight, 2];
      break;
    }
    default:
      throw new RangeError('Unknown demo');
  }
  return { objects, portals, spawn };
}

import { Euler, Vector3 } from 'three';
import { eyeHeight } from './levels.js';

export class Player {
  position = new Vector3();
  velocity = new Vector3();
  rotation = new Euler(0, 0, 0, 'YXZ');
  scale = 1;
  crossings = 0;

  reset(spawn) {
    this.position.set(...spawn);
    this.velocity.set(0, 0, 0);
    this.rotation.set(0, 0, 0);
    this.scale = 1;
    this.crossings = 0;
  }

  look(x, y) {
    this.rotation.y -= x;
    this.rotation.x = Math.max(-1.55, Math.min(1.55, this.rotation.x - y));
  }

  step(dt, right, forward, level) {
    const before = this.position.clone();
    const input = new Vector3(right, 0, -forward).clampLength(0, 1)
      .applyAxisAngle(new Vector3(0, 1, 0), this.rotation.y);
    const response = 1 - Math.exp(-18 * dt);
    this.velocity.x += (input.x * 2.9 * this.scale - this.velocity.x) * response;
    this.velocity.z += (input.z * 2.9 * this.scale - this.velocity.z) * response;
    this.velocity.y -= 9.8 * this.scale * dt;
    this.position.addScaledVector(this.velocity, dt);

    const radius = 0.2 * this.scale;
    const center = new Vector3();
    const closest = new Vector3();
    const delta = new Vector3();
    for (const height of [0, radius - eyeHeight * this.scale]) {
      for (const collider of level.colliders) {
        center.copy(this.position);
        center.y += height;
        if (collider.bounds.distanceToPoint(center) >= radius) continue;
        const { u, v, uLengthSq, vLengthSq } = collider;
        delta.subVectors(center, collider.center);
        const alongU = Math.max(-1, Math.min(1, delta.dot(u) / uLengthSq));
        const alongV = Math.max(-1, Math.min(1, delta.dot(v) / vLengthSq));
        closest.copy(collider.center).addScaledVector(u, alongU).addScaledVector(v, alongV);
        delta.subVectors(center, closest);
        const distance = delta.length();
        if (distance >= radius || distance < 1e-8) continue;
        delta.divideScalar(distance);
        if (delta.y > 0.7) {
          this.position.y += (radius - distance) / delta.y;
          this.velocity.y = Math.max(0, this.velocity.y);
        } else {
          this.position.addScaledVector(delta, radius - distance);
          const towardWall = this.velocity.dot(delta);
          if (towardWall < 0) this.velocity.addScaledVector(delta, -towardWall);
        }
      }
    }

    let nearest = 1;
    let crossing;
    for (const portal of level.portals) {
      const distanceA = delta.subVectors(before, portal.position).dot(portal.normal);
      const distanceB = delta.subVectors(this.position, portal.position).dot(portal.normal);
      if (distanceA * distanceB > 0 || Math.abs(distanceA - distanceB) < 1e-9) continue;
      const time = distanceA / (distanceA - distanceB);
      if (time >= nearest) continue;
      const hit = closest.copy(before).lerp(this.position, time).applyMatrix4(portal.inverse);
      if (Math.abs(hit.x) >= 1 || Math.abs(hit.y) >= 1) continue;
      nearest = time;
      crossing = { warp: portal.sides[distanceA > 0 ? 'front' : 'back'], portal, distanceA };
    }
    if (crossing) {
      const { warp, portal, distanceA } = crossing;
      const e = warp.matrix.elements;
      this.position.addScaledVector(portal.normal, distanceA > 0 ? -0.004 : 0.004)
        .applyMatrix4(warp.matrix);
      this.rotation.y += Math.atan2(e[8], e[10]);
      const origin = closest.set(0, 0, 0).applyMatrix4(warp.matrix);
      this.velocity.applyMatrix4(warp.matrix).sub(origin);
      this.scale *= Math.hypot(e[0], e[2]);
      this.crossings++;
    }
    if (this.position.y < -30 * this.scale) this.reset(level.spawn);
  }
}

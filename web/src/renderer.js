import {
  DoubleSide, Matrix4, Mesh, PerspectiveCamera, Plane, PlaneGeometry,
  Scene, ShaderMaterial, Vector2, Vector3, Vector4, WebGLRenderer, WebGLRenderTarget,
} from 'three';

const maxDepth = 3;
const fullView = [-1, -1, 1, 1];

const skyMaterial = () => new ShaderMaterial({
  depthTest: false,
  depthWrite: false,
  uniforms: { inverseProjection: { value: new Matrix4() }, cameraWorld: { value: new Matrix4() } },
  vertexShader: `
    varying vec2 ndc;
    void main() {
      ndc = position.xy;
      gl_Position = vec4(position.xy, 0.0, 1.0);
    }
  `,
  fragmentShader: `
    varying vec2 ndc;
    uniform mat4 inverseProjection;
    uniform mat4 cameraWorld;
    void main() {
      vec3 ray = (inverseProjection * vec4(ndc, 1.0, 1.0)).xyz;
      vec3 n = normalize(mat3(cameraWorld) * ray);
      float haze = (1.0 - n.y) * (1.0 - n.y) * 0.5;
      vec3 sky = vec3(0.2 + haze, 0.5 + haze, 1.0);
      float sun = exp(min(0.0, (dot(n, vec3(0.36, 0.80, 0.48)) - 0.998) / 0.002));
      gl_FragColor = vec4(max(sky, vec3(sun)), 1.0);
    }
  `,
});

const portalMaterial = () => new ShaderMaterial({
  side: DoubleSide,
  uniforms: { view: { value: null }, resolution: { value: new Vector2() }, cutoff: { value: true } },
  vertexShader: `
    void main() {
      gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    }
  `,
  fragmentShader: `
    uniform sampler2D view;
    uniform vec2 resolution;
    uniform bool cutoff;
    void main() {
      gl_FragColor = cutoff ? vec4(0.95, 0.24, 0.61, 1.0)
        : texture2D(view, gl_FragCoord.xy / resolution);
    }
  `,
});

// Replace the near plane with the exit plane while preserving screen X and Y.
function clipProjection(projection, view, position, normal) {
  const plane = new Plane().setFromNormalAndCoplanarPoint(normal, position).applyMatrix4(view);
  const clip = new Vector4(plane.normal.x, plane.normal.y, plane.normal.z, plane.constant);
  const e = projection.elements;
  const corner = new Vector4(Math.sign(clip.x), Math.sign(clip.y), 1, 1)
    .applyMatrix4(projection.clone().invert());
  const divisor = clip.dot(corner);
  if (divisor <= 1e-7) return;
  clip.multiplyScalar(2 / divisor);
  e[2] = clip.x;
  e[6] = clip.y;
  e[10] = clip.z + 1;
  e[14] = clip.w;
}

function apertureBounds(portal, viewProjection, bounds) {
  const transform = viewProjection.clone().multiply(portal.matrix);
  const corners = [[-1, -1], [1, -1], [1, 1], [-1, 1]].map(
    ([x, y]) => new Vector4(x, y, 0, 1).applyMatrix4(transform),
  );
  // Reject openings completely outside any homogeneous clipping plane.
  for (const [axis, sign] of [['x', 1], ['x', -1], ['y', 1], ['y', -1], ['z', 1], ['z', -1]]) {
    if (corners.every(point => sign * point[axis] > point.w)) return null;
  }
  if (corners.some(point => point.w <= 0)) return bounds;
  const xs = corners.map(point => point.x / point.w);
  const ys = corners.map(point => point.y / point.w);
  const minX = Math.max(bounds[0], Math.min(...xs));
  const minY = Math.max(bounds[1], Math.min(...ys));
  const maxX = Math.min(bounds[2], Math.max(...xs));
  const maxY = Math.min(bounds[3], Math.max(...ys));
  return minX < maxX && minY < maxY ? [minX, minY, maxX, maxY] : null;
}

export class PortalRenderer {
  constructor(canvas) {
    this.renderer = new WebGLRenderer({ canvas, antialias: true, alpha: false });
    this.renderer.autoClear = false;
    this.camera = new PerspectiveCamera(60, 1, 0.006, 100);
    this.views = Array.from({ length: maxDepth }, () => {
      const camera = new PerspectiveCamera();
      camera.matrixAutoUpdate = false;
      camera.matrixWorldAutoUpdate = false;
      return camera;
    });
    this.targets = this.views.map(() => new WebGLRenderTarget(1, 1));
    this.skyScene = new Scene();
    this.portalScene = new Scene();
    const quad = new PlaneGeometry(2, 2);
    this.sky = new Mesh(quad, skyMaterial());
    this.aperture = new Mesh(quad, portalMaterial());
    this.aperture.matrixAutoUpdate = false;
    this.aperture.frustumCulled = this.sky.frustumCulled = false;
    this.skyScene.add(this.sky);
    this.portalScene.add(this.aperture);
    this.size = new Vector2();
  }

  resize(width, height) {
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
    this.renderer.setSize(width, height, false);
    this.renderer.getDrawingBufferSize(this.size);
    const ratio = Math.min(1, 900 / Math.max(this.size.x, this.size.y));
    for (const target of this.targets) {
      target.setSize(Math.max(1, Math.round(this.size.x * ratio)),
        Math.max(1, Math.round(this.size.y * ratio)));
    }
    this.camera.aspect = width / height;
  }

  draw(level, player) {
    this.camera.position.copy(player.position);
    this.camera.rotation.copy(player.rotation);
    this.camera.near = 0.003 * player.scale;
    this.camera.far = 100 * player.scale;
    this.camera.updateProjectionMatrix();
    this.camera.updateMatrixWorld();
    this.render(level, this.camera, 0, fullView);
    this.renderer.setRenderTarget(null);
  }

  render(level, camera, depth, bounds, skipPortal) {
    const target = depth ? this.targets[depth - 1] : null;
    const { renderer } = this;
    const { width, height } = target ?? { width: this.size.x, height: this.size.y };
    renderer.setRenderTarget(target);
    renderer.clear();
    this.sky.material.uniforms.cameraWorld.value.copy(camera.matrixWorld);
    this.sky.material.uniforms.inverseProjection.value.copy(this.camera.projectionMatrixInverse);
    renderer.render(this.skyScene, camera);
    renderer.render(level.scene, camera);

    const viewProjection = camera.projectionMatrix.clone().multiply(camera.matrixWorldInverse);
    const eye = new Vector3().setFromMatrixPosition(camera.matrixWorld);
    for (const portal of level.portals) {
      if (portal === skipPortal) continue;
      const visible = apertureBounds(portal, viewProjection, bounds);
      if (!visible) continue;
      const side = eye.clone().sub(portal.position).dot(portal.normal) > 0 ? 1 : -1;
      const warp = portal.sides[side > 0 ? 'front' : 'back'];
      if (!warp) continue;
      const canRecurse = depth < maxDepth &&
        (visible[2] - visible[0]) * width > 4 && (visible[3] - visible[1]) * height > 4;

      if (canRecurse) {
        const through = this.views[depth];
        through.matrixWorld.multiplyMatrices(warp.matrix, camera.matrixWorld);
        through.matrixWorldInverse.copy(through.matrixWorld).invert();
        through.projectionMatrix.copy(this.camera.projectionMatrix);
        const normal = warp.exit.normal.clone().multiplyScalar((warp.exitSign ?? -1) * side);
        const planePoint = warp.exit.position.clone().addScaledVector(normal, warp.exit.mounted ? 0 : -0.007);
        clipProjection(through.projectionMatrix, through.matrixWorldInverse, planePoint, normal);
        this.render(level, through, depth + 1, visible, warp.exit);
        renderer.setRenderTarget(target);
      }

      const uniforms = this.aperture.material.uniforms;
      uniforms.cutoff.value = !canRecurse;
      uniforms.view.value = canRecurse ? this.targets[depth].texture : null;
      uniforms.resolution.value.set(width, height);
      this.aperture.matrix.copy(portal.matrix);
      this.aperture.matrixWorldNeedsUpdate = true;
      renderer.render(this.portalScene, camera);
    }
  }
}

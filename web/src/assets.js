import {
  Box3, BufferGeometry, Float32BufferAttribute, Matrix3, NearestFilter,
  RepeatWrapping, ShaderMaterial, TextureLoader, Vector3,
} from 'three';

function parseMesh(source) {
  const vertices = [];
  const uvs = [];
  const positions = [];
  const coordinates = [];
  const layers = [];
  const collision = [];

  for (const line of source.split(/\r?\n/)) {
    const [kind, ...fields] = line.trim().split(/\s+/);
    if (kind === 'v') vertices.push(fields.slice(0, 3).map(Number));
    if (kind === 'vt') uvs.push(fields.map(Number));
    if (kind === 'c') {
      const ids = fields[0] === '*'
        ? [vertices.length - 2, vertices.length - 1, vertices.length]
        : fields.map(Number);
      collision.push(ids.map(id => new Vector3(...vertices[id - 1])));
    }
    if (kind !== 'f') continue;
    const shorthand = fields[0].startsWith('*');
    const count = shorthand ? fields[0].length + 2 : fields.length;
    const corners = Array.from({ length: count }, (_, i) => {
      if (shorthand) return [vertices.length - count + i, uvs.length - count + i];
      const [position, uv] = fields[i].split('/');
      return [Number(position) - 1, Number(uv || position) - 1];
    });
    for (let i = 1; i < corners.length - 1; i++) {
      for (const [position, uv] of [corners[0], corners[i], corners[i + 1]]) {
        positions.push(...vertices[position]);
        const [u, v, layer = 0] = uvs[uv] ?? [0, 0];
        coordinates.push(u, v);
        layers.push(layer);
      }
    }
  }

  const geometry = new BufferGeometry();
  geometry.setAttribute('position', new Float32BufferAttribute(positions, 3));
  geometry.setAttribute('uv', new Float32BufferAttribute(coordinates, 2));
  geometry.setAttribute('tile', new Float32BufferAttribute(layers, 1));
  geometry.computeVertexNormals();
  return { geometry, collision };
}

export function makeCollider(points, matrix) {
  const corners = points.map(point => point.clone().applyMatrix4(matrix));
  // The upstream c records describe three corners of a collision rectangle.
  const pairs = [[0, 1, 2], [1, 2, 0], [2, 0, 1]];
  pairs.sort(([a, b], [c, d]) =>
    corners[c].distanceToSquared(corners[d]) - corners[a].distanceToSquared(corners[b]));
  const [a, b, c] = pairs[0].map(index => corners[index]);
  const center = a.clone().add(b).multiplyScalar(0.5);
  const u = a.clone().sub(c).multiplyScalar(0.5);
  const v = b.clone().sub(c).multiplyScalar(0.5);
  const opposite = a.clone().add(b).sub(c);

  return {
    center, u, v,
    uLengthSq: u.lengthSq(),
    vLengthSq: v.lengthSq(),
    bounds: new Box3().setFromPoints([...corners, opposite]),
  };
}

const surfaceVertex = `
  attribute float tile;
  uniform mat3 worldNormal;
  varying vec3 surfaceNormal;
  varying vec2 surfaceUV;
  varying float layer;
  void main() {
    surfaceNormal = normalize(worldNormal * normal);
    surfaceUV = uv;
    layer = tile;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
  }
`;

const surfaceFragment = `
  uniform sampler2D image;
  uniform bool atlas;
  uniform vec2 imageSize;
  varying vec3 surfaceNormal;
  varying vec2 surfaceUV;
  varying float layer;
  void main() {
    vec2 lookup = surfaceUV;
    if (atlas) {
      vec2 tile = vec2(mod(layer, 4.0), 3.0 - floor(layer / 4.0));
      vec2 inset = 0.5 / imageSize;
      lookup = tile * 0.25 + inset + fract(surfaceUV) * (0.25 - 2.0 * inset);
    }
    float light = dot(normalize(surfaceNormal), vec3(0.36, 0.80, 0.48));
    float shade = atlas ? light * 0.25 + 0.75 : light * 0.5 + 0.5;
    gl_FragColor = vec4(texture2D(image, lookup).rgb * shade, 1.0);
  }
`;

export function makeSurface(texture, atlas, matrix) {
  return new ShaderMaterial({
    uniforms: {
      image: { value: texture },
      imageSize: { value: [texture.image.width, texture.image.height] },
      atlas: { value: atlas },
      worldNormal: { value: new Matrix3().getNormalMatrix(matrix) },
    },
    vertexShader: surfaceVertex,
    fragmentShader: surfaceFragment,
  });
}

export function createAssets() {
  const meshes = new Map();
  const textures = new Map();
  const loader = new TextureLoader();
  const base = import.meta.env.BASE_URL;
  return {
    mesh(name) {
      if (!meshes.has(name)) {
        const request = fetch(`${base}geometry/${name}.obj`).then(async response => {
          if (!response.ok) throw new Error(`Could not load mesh ${name}`);
          return parseMesh(await response.text());
        });
        meshes.set(name, request);
        request.catch(() => meshes.delete(name));
      }
      return meshes.get(name);
    },
    texture(name) {
      if (!textures.has(name)) {
        const request = loader.loadAsync(`${base}textures/${name}.bmp`).then(texture => {
          texture.wrapS = texture.wrapT = RepeatWrapping;
          texture.minFilter = texture.magFilter = NearestFilter;
          texture.generateMipmaps = false;
          return texture;
        });
        textures.set(name, request);
        request.catch(() => textures.delete(name));
      }
      return textures.get(name);
    },
  };
}

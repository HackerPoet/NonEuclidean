const prefix = 'NE1.';
const types = ['floor', 'wall', 'door'];
const inRange = (n, min, max) => Number.isFinite(n) && n >= min && n <= max;

export function exportLevel(pieces) {
  const rows = pieces.map(p => {
    const row = [types.indexOf(p.type), p.x, p.z, p.turn, p.height ?? 0];
    if (p.type === 'floor') row.push(p.rise ?? 0);
    if (p.type === 'door') row.push(p.pair, p.size ?? 1, Number(!!p.mounted));
    return row;
  });
  const code = prefix + JSON.stringify(rows);
  importLevel(code);
  return code;
}

export function importLevel(text) {
  if (text.length > 500000) throw Error('Level code is too large.');
  const code = text.trim();
  if (!code.startsWith(prefix)) throw Error('Expected a level code beginning with NE1.');
  let rows;
  try { rows = JSON.parse(code.slice(prefix.length)); }
  catch { throw Error('Incomplete or invalid level code. Copy the whole code and try again.'); }
  if (!Array.isArray(rows) || rows.length > 10000) throw Error('Expected a level with at most 10,000 pieces.');
  const pairs = new Map();
  return rows.map((row, index) => {
    const invalid = () => Error('Invalid piece ' + (index + 1) + '.');
    if (!Array.isArray(row)) throw invalid();
    const [kind, x, z, turn, height, detail, size, mounted] = row;
    if (!Number.isInteger(kind) || row.length !== [6, 5, 8][kind] ||
        !Number.isSafeInteger(x) || !Number.isSafeInteger(z) ||
        !Number.isInteger(turn) || !inRange(turn, 0, 3) || !inRange(height, 0, 100)) throw invalid();
    const piece = { type: types[kind], x, z, turn, height };
    if (kind === 0) {
      if (!inRange(detail, 0, 1.5)) throw invalid();
      piece.rise = detail;
    }
    if (kind === 2) {
      if (!Number.isSafeInteger(detail) || detail < 0 || detail >= Number.MAX_SAFE_INTEGER ||
          !inRange(size, 0.1, 1) || ![0, 1].includes(mounted)) throw invalid();
      const count = (pairs.get(detail) ?? 0) + 1;
      if (count > 2) throw invalid();
      pairs.set(detail, count);
      Object.assign(piece, { pair: detail, size, mounted: !!mounted });
    }
    return piece;
  });
}

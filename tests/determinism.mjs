// Native vs WebAssembly: the same maze and seed must give the same best
// fitness after N generations. Run with `make determinism` (needs emcc and node).
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import createModule from '../build/mazefinder-node.mjs';

const mf = await createModule();
const create = mf.cwrap('mf_create', 'number', ['string', 'number']);
const GENERATIONS = 300;
let failed = 0;
for (const name of ['open', 'obstacles', 'maze', 'loops']) {
  for (const seed of [1, 2, 3]) {
    const out = execFileSync('./build/mazefinder', ['--maze', `mazes/${name}.txt`, '--seed', String(seed),
      '--generations', String(GENERATIONS), '--every', String(GENERATIONS), '--threads', '1'], { encoding: 'utf8' });
    const native = Number(out.match(new RegExp(`generation ${GENERATIONS} best (\\d+)`))[1]);
    if (create(readFileSync(`mazes/${name}.txt`, 'utf8'), seed) !== 0) throw new Error(mf.UTF8ToString(mf._mf_error()));
    mf._mf_step(GENERATIONS);
    const wasm = mf._mf_best_fitness();
    const ok = native === wasm;
    if (!ok) failed++;
    console.log(`${ok ? 'ok  ' : 'FAIL'} ${name} seed ${seed}: native ${native}, wasm ${wasm}`);
  }
}
process.exit(failed ? 1 : 0);

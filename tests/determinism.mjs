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
// Generated mazes: the same text from mf_generate and `--generate`, and the same run.
const generate = mf.cwrap('mf_generate', 'string', ['number', 'number', 'number']);
for (const [size, loops] of [[20, 0], [20, 1], [30, 0], [30, 1], [40, 1]]) {
  for (const seed of [1, 2, 3]) {
    const args = ['--seed', String(seed), ...(loops ? ['--loops'] : [])];
    const nativeText = execFileSync('./build/mazefinder', ['--generate', String(size), ...args], { encoding: 'utf8' });
    const text = generate(size, seed, loops);
    const out = execFileSync('./build/mazefinder', ['--size', String(size), ...args, '--generations', String(GENERATIONS),
      '--every', String(GENERATIONS), '--threads', '1'], { encoding: 'utf8' });
    const native = Number(out.match(new RegExp(`generation ${GENERATIONS} best (\\d+)`))[1]);
    if (create(text, seed) !== 0) throw new Error(mf.UTF8ToString(mf._mf_error()));
    mf._mf_step(GENERATIONS);
    const wasm = mf._mf_best_fitness();
    const ok = text === nativeText && native === wasm && mf._mf_optimal_length() === mf._mf_shortest_path() + 1;
    if (!ok) failed++;
    console.log(`${ok ? 'ok  ' : 'FAIL'} generated ${size}${loops ? ' loops' : ''} seed ${seed}: maze text ${text === nativeText ? 'same' : 'DIFFERS'}, native ${native}, wasm ${wasm}`);
  }
}
process.exit(failed ? 1 : 0);

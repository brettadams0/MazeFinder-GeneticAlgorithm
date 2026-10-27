# MazeFinder-GeneticAlgorithm

A genetic algorithm in C++ that evolves a route through a walled maze. Fitness is scored in parallel
on a hand-rolled thread pool in the native build. The same core also compiles to WebAssembly for the
browser.

A genome is a fixed-length sequence of moves over `{UP, DOWN, LEFT, RIGHT, STAND}`. Each generation
walks every genome from the top-left corner. A move into a wall or off the edge is blocked (the walker
stays put), and the walk stops at the bottom-right goal. The population keeps its better half and
refills it with one crossover child per survivor, each with one mutation: a replaced move, a deleted
move (a random one appended) or an inserted move (the last dropped).

- **Fitness** (lower is better): the breadth-first-search distance from the walker's final cell to the
  goal, times (genome length + 1), plus the moves used. A walk that reaches the goal scores its move
  count, so the GA keeps shortening a route that already works.
- **Genome length:** 1.5× the maze's shortest path, and never under 100 moves.
- **Population** 100, **generations** 1000 by default.
- **Randomness:** one seeded splitmix64 generator, so a seed replays the same run natively and in
  WebAssembly.

## Mazes

`mazes/*.txt` holds one row per line: `#` is a wall, `.` is open. Start is the top-left corner and the
goal is the bottom-right one. The loader rejects a maze with no path between them. New mazes load
without code changes.

| File | What it is | Shortest path |
|---|---|---|
| `open.txt` | 20x20, no walls (the original behaviour) | 38 |
| `obstacles.txt` | 20x20 with six wall bars | 38 |
| `maze.txt` | 20x20 recursive-backtracker maze (one route) | 66 |
| `loops.txt` | 20x20 backtracker maze with 15 walls removed (several routes) | 42 |

`tools/gen_mazes.py` generated them with fixed seeds. They are committed as text, not generated at run
time.

## Building and running

Needs a C++11 compiler and pthreads.

```sh
make                     # build/mazefinder
./build/mazefinder --maze mazes/maze.txt --seed 1
make test                # unit tests (plain asserts)
```

Options: `--maze FILE` (default `mazes/open.txt`), `--seed N`, `--generations N`, `--every N` (print
the best fitness every N generations, default 100), `--population N`, `--threads N` (default 4), and
`--verbose` (every fitness value, and the best route as `UDLRS` letters).

## WebAssembly

```sh
make wasm                # web/mazefinder.js and web/mazefinder.wasm (needs Emscripten 6.0.13, em++)
make determinism         # native and WASM give the same best fitness per maze and seed
```

`src/wasm_api.cpp` exposes a small C API (`mf_create`, `mf_step`, `mf_best_fitness`,
`mf_reached_goal`, and views of the maze and of every walked path, fittest first). The built `.js`
and `.wasm` are committed so a static site can copy them. Thread pool: native build only. WASM
threads need `SharedArrayBuffer`, which needs COOP/COEP headers that GitHub Pages can't send, so the
browser build scores fitness on one thread.

## Layout

| | |
|---|---|
| `src/maze.{h,cpp}` | load, validate, BFS distances |
| `src/ga.{h,cpp}` | genome, walk, fitness, selection, crossover, mutation, seeded RNG (no threads, no I/O) |
| `src/main.cpp` | native CLI and its thread pool |
| `src/wasm_api.cpp` | the C API for the browser build |
| `tests/` | unit tests and the native-vs-WASM determinism test |

## License

MIT — see [LICENSE](LICENSE).

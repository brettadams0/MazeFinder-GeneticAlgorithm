# MazeFinder-GeneticAlgorithm

A genetic algorithm in C++ that evolves a route across a 20x20 grid, with fitness evaluated in
parallel on a hand-rolled thread pool.

A genome is a fixed 100-move sequence over `{UP, DOWN, LEFT, RIGHT, STAND}`. Each generation walks
every genome from the top-left corner, scores where it ended up, keeps the better half, and refills
the population by crossing and mutating the survivors. It runs 1000 generations over a population
of 100.

The interesting part is the thread pool rather than the GA: fitness evaluation is the only
parallelisable stage, so each generation enqueues 100 independent tasks across 4 workers and blocks
on their futures before selection.

## Building and running

Needs a C++11 compiler and pthreads.

```sh
g++ -std=c++11 -pthread MazeFinder.cpp -o mazefinder
./mazefinder
```

It prints the initial population, then per generation the fitness vector and the selected genomes.
That is a lot of output — pipe it somewhere if you want to read it.

## Tunables

All compile-time constants at the top of `MazeFinder.cpp`:

| | |
|---|---|
| `MAZE_SIZE` | `20` |
| `POPULATION_SIZE` | `100` |
| `GENERATIONS` | `1000` |
| `THREAD_COUNT` | `4` |
| `GENOME_LENGTH` | `100` |

## Honest scope

The grid is allocated as all zeroes and nothing ever writes a wall into it, so what the population
actually solves is "reach the bottom-right corner of an open field" — fitness is Manhattan distance
to that corner. The maze is a `vector<vector<int>>` and `evaluateFitness` already receives it, so
adding real obstacles is the natural next step; as it stands the pathfinding is easier than the name
suggests. The thread pool and the evolutionary loop are the parts doing real work.

## License

MIT — see [LICENSE](LICENSE).

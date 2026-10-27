#ifndef MAZEFINDER_GA_H
#define MAZEFINDER_GA_H

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "maze.h"

enum Direction { UP, DOWN, LEFT, RIGHT, STAND };
using Genome = std::vector<Direction>;

// splitmix64, with ranges mapped by hand: std::uniform_int_distribution
// returns different numbers under libstdc++ and libc++, and the browser build
// must replay the native one exactly.
class Rng {
public:
    explicit Rng(uint64_t seed) : state(seed) {}
    uint64_t next();
    int below(int n) { return static_cast<int>(next() % static_cast<uint64_t>(n)); }
private:
    uint64_t state;
};

struct Walk {
    int x = 0, y = 0;
    int moves = 0;        // moves read before stopping (all of them unless the goal was reached)
    bool reached = false;
};

// Walks the genome from the top-left corner. A move into a wall or off the
// edge is blocked (the walker stays put) and the walk continues; it stops at
// the goal. `path`, if given, receives every cell visited, start included.
Walk walk(const Genome& genome, const Maze& maze, std::vector<std::pair<int, int>>* path = nullptr);

// Lower is better: the BFS distance to the goal from where the walk ended,
// times (genome length + 1), plus the moves used. Reaching the goal sooner
// still scores better, so the GA keeps shortening a path that works.
int fitnessOf(const Walk& w, const Maze& maze, int genomeLength);
int evaluateFitness(const Genome& genome, const Maze& maze);

// About 1.5x the shortest path, and never under the original 100 moves.
int genomeLengthFor(const Maze& maze);

// Fills `fitness` (already sized) for every genome. The native CLI passes one
// that runs on its thread pool; the default runs on the calling thread.
using Evaluator = std::function<void(const std::vector<Genome>&, const Maze&, std::vector<int>&)>;
void evaluateSerial(const std::vector<Genome>& population, const Maze& maze, std::vector<int>& fitness);

class Population {
public:
    Population(const Maze& maze, uint64_t seed, int size = 100, Evaluator evaluator = evaluateSerial);

    // Keeps the better half (ties broken by position), refills it with one
    // mutated crossover child per survivor (the i-th best with the i-th
    // worst survivor), and re-scores.
    void step();

    int generation() const { return generation_; }
    int best() const { return order_[0]; }  // index of the fittest genome
    int bestFitness() const { return fitness_[order_[0]]; }
    bool reachedGoal() const;
    int genomeLength() const { return length_; }
    const std::vector<Genome>& genomes() const { return genomes_; }
    const std::vector<int>& fitness() const { return fitness_; }
    const Maze& maze() const { return maze_; }

private:
    void score();
    Genome randomGenome();
    Genome crossover(const Genome& a, const Genome& b);
    void mutate(Genome& g);

    Maze maze_;
    Rng rng_;
    int length_;
    Evaluator evaluator_;
    std::vector<Genome> genomes_;
    std::vector<int> fitness_;
    std::vector<int> order_;  // genome indices, fittest first
    int generation_ = 0;
};

#endif

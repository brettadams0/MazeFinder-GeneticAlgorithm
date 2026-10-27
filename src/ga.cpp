#include "ga.h"

#include <algorithm>
#include <numeric>

uint64_t Rng::next() {
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

Walk walk(const Genome& genome, const Maze& maze, std::vector<std::pair<int, int>>* path) {
    Walk w;
    const int gx = maze.width - 1, gy = maze.height - 1;
    if (path) path->emplace_back(0, 0);
    for (Direction move : genome) {
        if (w.x == gx && w.y == gy) { w.reached = true; break; }
        int nx = w.x, ny = w.y;
        switch (move) {
            case UP:    ny--; break;
            case DOWN:  ny++; break;
            case LEFT:  nx--; break;
            case RIGHT: nx++; break;
            case STAND: break;
        }
        w.moves++;
        if (!maze.wall(nx, ny)) { w.x = nx; w.y = ny; }
        if (path) path->emplace_back(w.x, w.y);
    }
    if (w.x == gx && w.y == gy) w.reached = true;
    return w;
}

int fitnessOf(const Walk& w, const Maze& maze, int genomeLength) {
    return maze.dist[w.y][w.x] * (genomeLength + 1) + w.moves;
}

int evaluateFitness(const Genome& genome, const Maze& maze) {
    return fitnessOf(walk(genome, maze), maze, static_cast<int>(genome.size()));
}

int genomeLengthFor(const Maze& maze) {
    return std::max(100, (maze.shortestPath() * 3 + 1) / 2);
}

void evaluateSerial(const std::vector<Genome>& population, const Maze& maze, std::vector<int>& fitness) {
    for (size_t i = 0; i < population.size(); ++i) fitness[i] = evaluateFitness(population[i], maze);
}

Population::Population(const Maze& maze, uint64_t seed, int size, Evaluator evaluator)
    : maze_(maze), rng_(seed), length_(genomeLengthFor(maze)), evaluator_(std::move(evaluator)),
      fitness_(size), order_(size) {
    for (int i = 0; i < size; ++i) genomes_.push_back(randomGenome());
    score();
}

Genome Population::randomGenome() {
    Genome g(length_);
    for (auto& d : g) d = static_cast<Direction>(rng_.below(5));
    return g;
}

Genome Population::crossover(const Genome& a, const Genome& b) {
    const int point = rng_.below(length_);
    Genome child = a;
    std::copy(b.begin() + point, b.end(), child.begin() + point);
    return child;
}

// One change per child: replace a move, or delete one (appending a random
// move) or insert one (dropping the last). The shifts matter with walls: a
// replaced move re-routes everything after it, while deleting a wasted move
// keeps the route and frees a move at the end.
void Population::mutate(Genome& g) {
    const int kind = rng_.below(3), point = rng_.below(length_);
    const Direction d = static_cast<Direction>(rng_.below(5));
    if (kind == 0) {
        g[point] = d;
    } else if (kind == 1) {
        g.erase(g.begin() + point);
        g.push_back(d);
    } else {
        g.insert(g.begin() + point, d);
        g.pop_back();
    }
}

void Population::score() {
    evaluator_(genomes_, maze_, fitness_);
    std::iota(order_.begin(), order_.end(), 0);
    std::stable_sort(order_.begin(), order_.end(), [this](int a, int b) { return fitness_[a] < fitness_[b]; });
}

void Population::step() {
    const int half = static_cast<int>(genomes_.size()) / 2;
    std::vector<Genome> next;
    next.reserve(genomes_.size());
    for (int i = 0; i < half; ++i) {
        const Genome& parent = genomes_[order_[i]];
        next.push_back(parent);
        Genome child = crossover(parent, genomes_[order_[half - i - 1]]);
        mutate(child);
        next.push_back(child);
    }
    while (next.size() < genomes_.size()) next.push_back(randomGenome());  // odd sizes
    genomes_ = std::move(next);
    score();
    generation_++;
}

bool Population::reachedGoal() const {
    return maze_.dist[0][0] >= 0 && bestFitness() <= length_;
}

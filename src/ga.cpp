#include "ga.h"

#include <algorithm>
#include <numeric>

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

// Each child receives a replacement, deletion, insertion, or BFS-guided segment
// mutation. Single-move edits can get trapped in a longer corridor; the guided
// segment grows after a plateau so crossover can discover a different route.
void Population::mutate(Genome& g) {
    const int kind = rng_.below(4), point = rng_.below(length_);
    const Direction d = static_cast<Direction>(rng_.below(5));
    if (kind == 0) {
        g[point] = d;
    } else if (kind == 1) {
        g.erase(g.begin() + point);
        g.push_back(d);
    } else if (kind == 2) {
        g.insert(g.begin() + point, d);
        g.pop_back();
    } else {
        // Reuse the BFS distance field that already scores fitness. Start with 2-8
        // legal downhill moves; expand up to a full genome after prolonged stagnation.
        // The seeded RNG chooses the prefix, segment length and equally good moves.
        const Walk prefix = walk(Genome(g.begin(), g.begin() + point), maze_);
        int x = prefix.x, y = prefix.y;
        const int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
        const int span = std::min(length_, 8 + stagnant_ / 2);
        const int end = std::min(length_, point + 2 + rng_.below(span - 1));
        for (int i = point; i < end; ++i) {
            int options[4], count = 0;
            for (int move = 0; move < 4; ++move) {
                const int nx = x + dx[move], ny = y + dy[move];
                if (!maze_.wall(nx, ny) && maze_.dist[ny][nx] < maze_.dist[y][x]) options[count++] = move;
            }
            if (!count) break;
            const int move = options[rng_.below(count)];
            g[i] = static_cast<Direction>(move);
            x += dx[move]; y += dy[move];
        }
    }
}

void Population::score() {
    evaluator_(genomes_, maze_, fitness_);
    std::iota(order_.begin(), order_.end(), 0);
    std::stable_sort(order_.begin(), order_.end(), [this](int a, int b) { return fitness_[a] < fitness_[b]; });
}

void Population::step() {
    const int previousBest = bestFitness();
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
    stagnant_ = bestFitness() < previousBest ? 0 : stagnant_ + 1;
    generation_++;
}

bool Population::reachedGoal() const {
    return maze_.dist[0][0] >= 0 && bestFitness() <= length_;
}

// C API for the WebAssembly build. Single-threaded: the browser build has no
// thread pool (WASM threads need SharedArrayBuffer and COOP/COEP headers).
// Pointers returned here are views into WASM memory, valid until the next
// mf_create or mf_step.
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ga.h"
#include "maze.h"

namespace {
std::unique_ptr<Population> pop;
std::string error;
std::vector<uint8_t> cells;
std::vector<int16_t> paths;     // population x (genome length + 1) x (x, y), fittest first
std::vector<int32_t> lengths;   // points in each path, same order

void refresh() {
    const Maze& m = pop->maze();
    const int n = static_cast<int>(pop->genomes().size()), stride = pop->genomeLength() + 1;
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    const std::vector<int>& f = pop->fitness();
    std::stable_sort(order.begin(), order.end(), [&f](int a, int b) { return f[a] < f[b]; });
    paths.assign(static_cast<size_t>(n) * stride * 2, 0);
    lengths.assign(n, 0);
    std::vector<std::pair<int, int>> path;
    for (int i = 0; i < n; ++i) {
        path.clear();
        walk(pop->genomes()[order[i]], m, &path);
        lengths[i] = static_cast<int32_t>(path.size());
        for (size_t k = 0; k < path.size(); ++k) {
            paths[(static_cast<size_t>(i) * stride + k) * 2] = static_cast<int16_t>(path[k].first);
            paths[(static_cast<size_t>(i) * stride + k) * 2 + 1] = static_cast<int16_t>(path[k].second);
        }
    }
}
}  // namespace

extern "C" {

int mf_create(const char* mazeText, uint32_t seed) {
    Maze m;
    if (!parseMaze(mazeText ? mazeText : "", m, error)) { pop.reset(); return 1; }
    error.clear();
    pop.reset(new Population(m, seed));
    cells.clear();
    for (const auto& row : m.cells)
        for (int c : row) cells.push_back(static_cast<uint8_t>(c));
    refresh();
    return 0;
}

const char* mf_error() { return error.c_str(); }

void mf_step(int generations) {
    if (!pop) return;
    for (int i = 0; i < generations; ++i) pop->step();
    refresh();
}

int mf_generation() { return pop ? pop->generation() : 0; }
int mf_best_fitness() { return pop ? pop->bestFitness() : 0; }
int mf_reached_goal() { return pop && pop->reachedGoal() ? 1 : 0; }
int mf_width() { return pop ? pop->maze().width : 0; }
int mf_height() { return pop ? pop->maze().height : 0; }
int mf_shortest_path() { return pop ? pop->maze().shortestPath() : 0; }
int mf_genome_length() { return pop ? pop->genomeLength() : 0; }
int mf_population_size() { return pop ? static_cast<int>(pop->genomes().size()) : 0; }
const uint8_t* mf_cells() { return cells.data(); }
const int16_t* mf_paths() { return paths.data(); }
const int32_t* mf_path_lengths() { return lengths.data(); }

}

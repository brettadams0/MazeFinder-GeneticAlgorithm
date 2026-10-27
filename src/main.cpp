// Native CLI: evolves a route through one maze, scoring fitness on a
// hand-rolled thread pool.
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "ga.h"
#include "maze.h"

struct Task {
    std::function<void()> func;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t threads);
    ~ThreadPool();
    template<class F>
    std::future<void> enqueue(F&& f);

private:
    std::vector<std::thread> workers;
    std::queue<Task> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back([this] {
            for (;;) {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task.func();
            }
        });
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) worker.join();
}

template<class F>
std::future<void> ThreadPool::enqueue(F&& f) {
    auto task = std::make_shared<std::packaged_task<void()>>(std::forward<F>(f));
    std::future<void> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(Task{[task]() { (*task)(); }});
    }
    condition.notify_one();
    return res;
}

static void printGenome(const Genome& genome, int moves) {
    static const char letters[] = "UDLRS";
    for (int i = 0; i < moves; ++i) std::cout << letters[genome[i]];
    std::cout << '\n';
}

static void usage() {
    std::cerr << "usage: mazefinder [--maze FILE] [--seed N] [--generations N] [--every N]\n"
                 "                  [--population N] [--threads N] [--verbose]\n";
    std::exit(2);
}

int main(int argc, char** argv) {
    std::string mazePath = "mazes/open.txt";
    unsigned long long seed = 1;
    int generations = 1000, every = 100, populationSize = 100, threads = 4;
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        auto value = [&]() -> const char* { if (i + 1 >= argc) usage(); return argv[++i]; };
        if (!std::strcmp(a, "--maze")) mazePath = value();
        else if (!std::strcmp(a, "--seed")) seed = std::strtoull(value(), nullptr, 10);
        else if (!std::strcmp(a, "--generations")) generations = std::atoi(value());
        else if (!std::strcmp(a, "--every")) every = std::atoi(value());
        else if (!std::strcmp(a, "--population")) populationSize = std::atoi(value());
        else if (!std::strcmp(a, "--threads")) threads = std::atoi(value());
        else if (!std::strcmp(a, "--verbose")) verbose = true;
        else usage();
    }
    if (generations < 0 || every < 1 || populationSize < 2 || threads < 1) usage();

    Maze maze;
    std::string error;
    if (!loadMaze(mazePath, maze, error)) { std::cerr << "mazefinder: " << error << '\n'; return 1; }

    ThreadPool pool(threads);
    Evaluator parallel = [&pool](const std::vector<Genome>& population, const Maze& m, std::vector<int>& fitness) {
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < population.size(); ++i)
            futures.push_back(pool.enqueue([&population, &m, &fitness, i] { fitness[i] = evaluateFitness(population[i], m); }));
        for (auto& f : futures) f.get();
    };

    Population pop(maze, seed, populationSize, parallel);
    std::cout << mazePath << ": " << maze.width << "x" << maze.height << ", shortest path " << maze.shortestPath()
              << ", genome " << pop.genomeLength() << " moves, seed " << seed << '\n';
    for (;;) {
        const int g = pop.generation();
        if (g % every == 0 || g == generations)
            std::cout << "generation " << g << " best " << pop.bestFitness() << (pop.reachedGoal() ? " (goal)" : "") << '\n';
        if (verbose) {
            std::cout << "fitness:";
            for (int f : pop.fitness()) std::cout << ' ' << f;
            std::cout << '\n';
        }
        if (g == generations) break;
        pop.step();
    }
    const Walk best = walk(pop.genomes()[pop.best()], maze);
    std::cout << "best fitness " << pop.bestFitness() << ", "
              << (best.reached ? "reached the goal in " + std::to_string(best.moves) + " moves"
                               : "ended " + std::to_string(maze.dist[best.y][best.x]) + " steps from the goal")
              << '\n';
    if (verbose) printGenome(pop.genomes()[pop.best()], best.moves);
    return 0;
}

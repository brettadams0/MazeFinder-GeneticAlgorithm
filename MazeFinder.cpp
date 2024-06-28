#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <algorithm>
#include <functional>
#include <queue>
#include <atomic>
#include <future>

const int MAZE_SIZE = 20;
const int POPULATION_SIZE = 100;
const int GENERATIONS = 1000;
const int THREAD_COUNT = 4;
const int GENOME_LENGTH = 100;

// Directions
enum Direction { UP, DOWN, LEFT, RIGHT, STAND };

// Genome definition
using Genome = std::vector<Direction>;

// Maze definition
using Maze = std::vector<std::vector<int>>;

struct Task {
    std::function<void()> func;
};

// Thread pool class
class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();
    template<class F>
    auto enqueue(F&& f) -> std::future<void>;

private:
    std::vector<std::thread> workers;
    std::queue<Task> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i)
        workers.emplace_back(
            [this] {
                for(;;) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task.func();
                }
            }
        );
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

template<class F>
auto ThreadPool::enqueue(F&& f) -> std::future<void> {
    auto task = std::make_shared<std::packaged_task<void()>>(std::forward<F>(f));
    std::future<void> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(Task{[task]() { (*task)(); }});
    }
    condition.notify_one();
    return res;
}

// Function to generate a random genome
Genome generateRandomGenome() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 4);
    Genome genome(GENOME_LENGTH);
    std::generate(genome.begin(), genome.end(), []() { return static_cast<Direction>(dis(gen)); });
    return genome;
}

// Function to evaluate the fitness of a genome
int evaluateFitness(const Genome& genome, const Maze& maze) {
    int x = 0, y = 0;
    for (auto move : genome) {
        switch (move) {
            case UP:    if (y > 0) y--; break;
            case DOWN:  if (y < MAZE_SIZE - 1) y++; break;
            case LEFT:  if (x > 0) x--; break;
            case RIGHT: if (x < MAZE_SIZE - 1) x++; break;
            case STAND: break;
        }
        if (maze[y][x] == 1) break;  // Hit a wall
    }
    return (MAZE_SIZE - y) + (MAZE_SIZE - x); // Distance to bottom-right corner
}

// Function to perform crossover between two genomes
Genome crossover(const Genome& parent1, const Genome& parent2) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, GENOME_LENGTH - 1);
    int crossover_point = dis(gen);
    Genome child = parent1;
    std::copy(parent2.begin() + crossover_point, parent2.end(), child.begin() + crossover_point);
    return child;
}

// Function to mutate a genome
void mutate(Genome& genome) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, GENOME_LENGTH - 1);
    static std::uniform_int_distribution<> disDir(0, 4);
    int mutation_point = dis(gen);
    genome[mutation_point] = static_cast<Direction>(disDir(gen));
}

// Function to print a genome
void printGenome(const Genome& genome) {
    for (auto direction : genome) {
        switch (direction) {
            case UP: std::cout << "U"; break;
            case DOWN: std::cout << "D"; break;
            case LEFT: std::cout << "L"; break;
            case RIGHT: std::cout << "R"; break;
            case STAND: std::cout << "S"; break;
        }
    }
    std::cout << std::endl;
}

int main() {
    Maze maze(MAZE_SIZE, std::vector<int>(MAZE_SIZE, 0));
    std::vector<Genome> population(POPULATION_SIZE);
    std::vector<int> fitness(POPULATION_SIZE);

    ThreadPool pool(THREAD_COUNT);

    // Initial population
    std::cout << "Generating initial population..." << std::endl;
    for (auto& genome : population) {
        genome = generateRandomGenome();
        printGenome(genome);
    }

    for (int generation = 0; generation < GENERATIONS; ++generation) {
        std::cout << "Generation " << generation << std::endl;
        std::vector<std::future<void>> futures;

        // Evaluate fitness
        std::cout << "Evaluating fitness..." << std::endl;
        for (int i = 0; i < POPULATION_SIZE; ++i) {
            futures.push_back(pool.enqueue([&population, &fitness, &maze, i] {
                fitness[i] = evaluateFitness(population[i], maze);
            }));
        }
        for (auto &future : futures) {
            future.get();
        }

        // Print fitness
        std::cout << "Fitness values: ";
        for (auto fit : fitness) {
            std::cout << fit << " ";
        }
        std::cout << std::endl;

        // Selection
        std::cout << "Selecting top genomes..." << std::endl;
        std::vector<Genome> new_population;
        std::partial_sort(population.begin(), population.begin() + POPULATION_SIZE / 2, population.end(),
                          [&fitness, &population](const Genome& a, const Genome& b) {
                              return fitness[&a - population.data()] < fitness[&b - population.data()];
                          });

        // Print selected genomes
        std::cout << "Selected genomes: " << std::endl;
        for (int i = 0; i < POPULATION_SIZE / 2; ++i) {
            printGenome(population[i]);
        }

        // Crossover and mutation
        std::cout << "Performing crossover and mutation..." << std::endl;
        for (int i = 0; i < POPULATION_SIZE / 2; ++i) {
            new_population.push_back(population[i]);
            Genome child = crossover(population[i], population[POPULATION_SIZE / 2 - i - 1]);
            mutate(child);
            new_population.push_back(child);
        }

        population = new_population;

        // Print new population
        std::cout << "New population: " << std::endl;
        for (const auto& genome : population) {
            printGenome(genome);
        }

        // Evaluate new population fitness
        std::cout << "Re-evaluating fitness for new population..." << std::endl;
        futures.clear();
        for (int i = 0; i < POPULATION_SIZE; ++i) {
            futures.push_back(pool.enqueue([&population, &fitness, &maze, i] {
                fitness[i] = evaluateFitness(population[i], maze);
            }));
        }
        for (auto &future : futures) {
            future.get();
        }

        // Best fitness in this generation
        int best_fitness = *std::min_element(fitness.begin(), fitness.end());
        std::cout << "Best Fitness: " << best_fitness << std::endl;
    }

    return 0;
}

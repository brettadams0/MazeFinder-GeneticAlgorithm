// Plain-assert tests for the core. Build and run with `make test`.
#undef NDEBUG
#include <cassert>
#include <iostream>
#include <string>

#include "../src/ga.h"
#include "../src/maze.h"

static Maze parse(const std::string& text) {
    Maze m;
    std::string error;
    assert(parseMaze(text, m, error));
    return m;
}

int main() {
    std::string error;
    Maze m;

    // The loader rejects mazes with no path, a walled start or goal, and bad input.
    assert(!parseMaze(".#\n#.\n", m, error) && error == "no path from start to goal");
    assert(!parseMaze("#.\n..\n", m, error) && error == "start or goal is a wall");
    assert(!parseMaze("..\n...\n", m, error) && error == "rows differ in length");
    assert(!parseMaze("..\n.x\n", m, error));
    assert(!parseMaze("", m, error));

    // BFS distances from the goal on a hand-made maze.
    m = parse("...\n.#.\n...\n");
    const Grid want = {{4, 3, 2}, {3, -1, 1}, {2, 1, 0}};
    assert(m.dist == want);
    assert(m.shortestPath() == 4);

    // A move into a wall is blocked and the walk goes on.
    Genome g = {RIGHT, DOWN, RIGHT, DOWN, STAND};
    std::vector<std::pair<int, int>> path;
    Walk w = walk(g, m, &path);
    assert(w.x == 2 && w.y == 1 && !w.reached && w.moves == 5);
    assert(path.size() == 6 && path[2] == std::make_pair(1, 0));  // DOWN into (1,1) stayed put
    assert(evaluateFitness(g, m) == 1 * 6 + 5);

    // The walk stops at the goal; fewer moves score better.
    g = {RIGHT, RIGHT, DOWN, DOWN, LEFT};
    w = walk(g, m);
    assert(w.reached && w.moves == 4 && w.x == 2 && w.y == 2);
    assert(evaluateFitness(g, m) == 4);

    // Off the edge is blocked too.
    g = {UP, LEFT, STAND};
    w = walk(g, m);
    assert(w.x == 0 && w.y == 0 && w.moves == 3);

    // Genome length: about 1.5x the shortest path, never under 100.
    assert(genomeLengthFor(m) == 100);

    // The same seed gives the same run; another seed gives another.
    Maze maze;
    assert(loadMaze("mazes/maze.txt", maze, error));
    Population a(maze, 7), b(maze, 7), c(maze, 8);
    for (int i = 0; i < 50; ++i) { a.step(); b.step(); c.step(); }
    assert(a.bestFitness() == b.bestFitness() && a.genomes() == b.genomes());
    assert(a.genomes() != c.genomes());

    // Elitism: the best fitness never gets worse.
    Population p(maze, 3);
    int last = p.bestFitness();
    for (int i = 0; i < 200; ++i) { p.step(); assert(p.bestFitness() <= last); last = p.bestFitness(); }

    // Every shipped maze is solvable and fits its genome comfortably.
    for (const char* name : {"open", "obstacles", "maze", "loops"}) {
        Maze s;
        assert(loadMaze(std::string("mazes/") + name + ".txt", s, error));
        assert(genomeLengthFor(s) * 2 >= s.shortestPath() * 3);
    }

    std::cout << "all tests passed\n";
    return 0;
}

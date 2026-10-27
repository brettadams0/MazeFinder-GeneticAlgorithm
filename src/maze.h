#ifndef MAZEFINDER_MAZE_H
#define MAZEFINDER_MAZE_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// A maze is a grid of cells: 1 is a wall, 0 is open. Start is the top-left
// corner, goal is the bottom-right one.
using Grid = std::vector<std::vector<int>>;

struct Maze {
    int width = 0;
    int height = 0;
    Grid cells;
    // BFS distance from the goal to every cell; -1 for walls and cells that
    // cannot reach the goal.
    Grid dist;

    bool wall(int x, int y) const {
        return x < 0 || y < 0 || x >= width || y >= height || cells[y][x] == 1;
    }
    int shortestPath() const { return dist[0][0]; }
};

// Parses rows of '#' (wall) and '.' (open). Returns false and sets `error`
// when the text is not a rectangle of those characters, the start or goal is
// a wall, or no path joins them.
bool parseMaze(const std::string& text, Maze& out, std::string& error);
bool loadMaze(const std::string& path, Maze& out, std::string& error);

// Distance from (gx, gy) to every open cell, by breadth-first search.
Grid bfsDistances(const Grid& cells, int gx, int gy);

// An n x n maze (5 <= n <= 100) from a seeded recursive backtracker over the
// cells with even coordinates. For even n, the last row and column stay walls
// except a two-cell tail to the goal. With `loops`, 16% of the remaining walls
// between two carved cells are knocked out, so several routes exist. Returns
// the maze as text ('#' wall, '.' open), the format parseMaze reads.
std::string generateMaze(int n, uint64_t seed, bool loops);

// One shortest route from start to goal, start and goal included, following
// the BFS distances (ties broken up, down, left, right).
std::vector<std::pair<int, int>> optimalPath(const Maze& maze);

#endif

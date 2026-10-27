#ifndef MAZEFINDER_MAZE_H
#define MAZEFINDER_MAZE_H

#include <string>
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

#endif

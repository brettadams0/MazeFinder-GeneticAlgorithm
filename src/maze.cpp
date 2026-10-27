#include "maze.h"

#include <deque>
#include <fstream>
#include <sstream>

Grid bfsDistances(const Grid& cells, int gx, int gy) {
    const int h = static_cast<int>(cells.size());
    const int w = h ? static_cast<int>(cells[0].size()) : 0;
    Grid dist(h, std::vector<int>(w, -1));
    if (gx < 0 || gy < 0 || gx >= w || gy >= h || cells[gy][gx] == 1) return dist;
    std::deque<std::pair<int, int>> queue;
    dist[gy][gx] = 0;
    queue.emplace_back(gx, gy);
    const int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
    while (!queue.empty()) {
        const int x = queue.front().first, y = queue.front().second;
        queue.pop_front();
        for (int d = 0; d < 4; ++d) {
            const int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
            if (cells[ny][nx] == 1 || dist[ny][nx] != -1) continue;
            dist[ny][nx] = dist[y][x] + 1;
            queue.emplace_back(nx, ny);
        }
    }
    return dist;
}

bool parseMaze(const std::string& text, Maze& out, std::string& error) {
    Grid cells;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::vector<int> row;
        for (char c : line) {
            if (c == '#') row.push_back(1);
            else if (c == '.') row.push_back(0);
            else { error = std::string("unexpected character '") + c + "'"; return false; }
        }
        if (!cells.empty() && row.size() != cells[0].size()) { error = "rows differ in length"; return false; }
        cells.push_back(row);
    }
    if (cells.empty()) { error = "empty maze"; return false; }
    const int h = static_cast<int>(cells.size()), w = static_cast<int>(cells[0].size());
    if (cells[0][0] == 1 || cells[h - 1][w - 1] == 1) { error = "start or goal is a wall"; return false; }
    Grid dist = bfsDistances(cells, w - 1, h - 1);
    if (dist[0][0] < 0) { error = "no path from start to goal"; return false; }
    out.width = w;
    out.height = h;
    out.cells = cells;
    out.dist = dist;
    return true;
}

bool loadMaze(const std::string& path, Maze& out, std::string& error) {
    std::ifstream file(path);
    if (!file) { error = "cannot open " + path; return false; }
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (!parseMaze(buffer.str(), out, error)) { error = path + ": " + error; return false; }
    return true;
}

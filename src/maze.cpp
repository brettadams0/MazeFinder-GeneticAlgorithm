#include "maze.h"

#include "rng.h"

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

std::string generateMaze(int n, uint64_t seed, bool loops) {
    if (n < 5) n = 5;
    if (n > 100) n = 100;
    Rng rng(seed);
    Grid g(n, std::vector<int>(n, 1));
    const int top = (n % 2) ? n - 1 : n - 2;  // largest even coordinate
    std::vector<std::pair<int, int>> stack{{0, 0}};
    g[0][0] = 0;
    const int dx[] = {2, -2, 0, 0}, dy[] = {0, 0, 2, -2};
    while (!stack.empty()) {
        const int x = stack.back().first, y = stack.back().second;
        int options[4], count = 0;
        for (int d = 0; d < 4; ++d) {
            const int nx = x + dx[d], ny = y + dy[d];
            if (nx >= 0 && ny >= 0 && nx <= top && ny <= top && g[ny][nx] == 1) options[count++] = d;
        }
        if (!count) { stack.pop_back(); continue; }
        const int d = options[rng.below(count)];
        g[y + dy[d] / 2][x + dx[d] / 2] = 0;
        g[y + dy[d]][x + dx[d]] = 0;
        stack.emplace_back(x + dx[d], y + dy[d]);
    }
    if (n % 2 == 0) g[n - 1][n - 2] = g[n - 1][n - 1] = 0;
    if (loops) {
        std::vector<std::pair<int, int>> walls;  // walls between two carved cells
        for (int y = 0; y <= top; ++y)
            for (int x = 0; x <= top; ++x)
                if ((x + y) % 2 == 1 && g[y][x] == 1) walls.emplace_back(x, y);
        const int knock = static_cast<int>(walls.size() * 16 / 100);
        for (int i = 0; i < knock; ++i) {  // partial Fisher-Yates
            const int j = i + rng.below(static_cast<int>(walls.size()) - i);
            std::swap(walls[i], walls[j]);
            g[walls[i].second][walls[i].first] = 0;
        }
    }
    std::string text;
    for (const auto& row : g) {
        for (int c : row) text += c ? '#' : '.';
        text += '\n';
    }
    return text;
}

std::vector<std::pair<int, int>> optimalPath(const Maze& maze) {
    std::vector<std::pair<int, int>> path;
    if (maze.dist.empty() || maze.dist[0][0] < 0) return path;
    int x = 0, y = 0;
    path.emplace_back(x, y);
    const int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
    while (maze.dist[y][x] > 0) {
        for (int d = 0; d < 4; ++d) {
            const int nx = x + dx[d], ny = y + dy[d];
            if (!maze.wall(nx, ny) && maze.dist[ny][nx] == maze.dist[y][x] - 1) { x = nx; y = ny; break; }
        }
        path.emplace_back(x, y);
    }
    return path;
}

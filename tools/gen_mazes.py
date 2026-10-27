#!/usr/bin/env python3
"""Writes the mazes in mazes/. Run once; the text files are what gets committed.

Proper mazes come from a seeded recursive backtracker over the even cells of
the 20x20 grid (0, 2, ..., 18). Row and column 19 stay walls except for a
two-cell tail from (18, 18) to the goal at (19, 19).
"""
import random
from pathlib import Path

N = 20


def backtracker(seed):
    rng = random.Random(seed)
    g = [['#'] * N for _ in range(N)]
    stack = [(0, 0)]
    g[0][0] = '.'
    while stack:
        x, y = stack[-1]
        nxt = [(x + dx, y + dy, dx, dy) for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2))
               if 0 <= x + dx <= 18 and 0 <= y + dy <= 18 and g[y + dy][x + dx] == '#']
        if not nxt:
            stack.pop()
            continue
        nx, ny, dx, dy = rng.choice(nxt)
        g[y + dy // 2][x + dx // 2] = '.'
        g[ny][nx] = '.'
        stack.append((nx, ny))
    g[19][18] = g[19][19] = '.'
    return g, rng


def loops(seed, count):
    """A backtracker maze with `count` interior walls knocked out, so several routes exist."""
    g, rng = backtracker(seed)
    walls = [(x, y) for y in range(19) for x in range(19)
             if g[y][x] == '#' and (x + y) % 2 == 1]  # walls between two even cells
    for x, y in rng.sample(walls, count):
        g[y][x] = '.'
    return g


def obstacles():
    g = [['.'] * N for _ in range(N)]
    bars = [(3, 2, 3, 12), (7, 6, 7, 19), (11, 0, 11, 13), (15, 5, 15, 17), (0, 16, 4, 16), (12, 9, 18, 9)]
    for x0, y0, x1, y1 in bars:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                g[y][x] = '#'
    return g


def write(name, g):
    Path('mazes', name).write_text(''.join(''.join(r) + '\n' for r in g))


write('open.txt', [['.'] * N for _ in range(N)])
write('obstacles.txt', obstacles())
write('maze.txt', backtracker(1)[0])
write('loops.txt', loops(2, 15))

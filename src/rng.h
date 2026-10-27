#ifndef MAZEFINDER_RNG_H
#define MAZEFINDER_RNG_H

#include <cstdint>

// splitmix64, with ranges mapped by hand: std::uniform_int_distribution
// returns different numbers under libstdc++ and libc++, and the browser build
// must replay the native one exactly.
class Rng {
public:
    explicit Rng(uint64_t seed) : state(seed) {}
    uint64_t next() {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    int below(int n) { return static_cast<int>(next() % static_cast<uint64_t>(n)); }
private:
    uint64_t state;
};

#endif

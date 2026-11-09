#pragma once
#include <random>
#include <cstdint>

namespace ironcast {

    // 64-bit random numbers from a randomly seeded mt19937_64; behind Ironcast::random(). Not thread-safe.
    class RandomGenerator {
        std::mt19937_64 engine_;
    public:
        RandomGenerator() {
            std::random_device rd;
            engine_.seed(rd());
        }

        uint64_t random() {
            return engine_();
        }
    };

}

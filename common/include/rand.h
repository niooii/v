//
// Created by niooi on 9/27/2025.
//

#pragma once

#include <defs.h>
#include <cstdint>
#include <iterator>

/// The RNG namespace
namespace v::rand {
    /// Initializes the random subsystem, and does other necessary setup.
    void init();

    /// Explicitly seed the engine RNG.
    /// Also seeds the C RNG (std::rand) with the low 32 bits for legacy code.
    /// Logs the seed at debug level for reproducibility.
    /// @param seed 64-bit seed value
    void seed(u64 seed);

    /// Get the last seed used to initialize the RNG.
    /// Returns 0 if not yet seeded.
    u64 last_seed();

    /// Get a uniformly distributed 64-bit unsigned integer.
    u64 next_u64();

    /// Get a uniformly distributed 32-bit unsigned integer.
    u32 next_u32();

    /// Uniform real in [0, 1).
    f64 uniform();

    /// Uniform real in [min, max).
    /// @note If min > max, the values are swapped.
    f64 frange(f64 min, f64 max);

    /// Uniform integer in [min, max] (inclusive).
    /// @note If min > max, the values are swapped.
    i64 irange(i64 min, i64 max);

    /// Uniform unsigned integer in [min, max] (inclusive).
    /// @note If min > max, the values are swapped.
    u64 urange(u64 min, u64 max);

    /// Returns true with probability p (0.0 <= p <= 1.0).
    bool chance(f64 p);

    /// Pick a random iterator in [first, last).
    /// Returns last if the range is empty.
    template <typename It>
    It pick(It first, It last)
    {
        auto n = std::distance(first, last);
        if (n <= 0)
            return last;
        auto idx = static_cast<decltype(n)>(irange(0, static_cast<i64>(n - 1)));
        std::advance(first, idx);
        return first;
    }
} // namespace v::rand


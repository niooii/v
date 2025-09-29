//
// Created by niooi on 9/27/2025.
//

#include <absl/synchronization/mutex.h>
#include <cstdlib>
#include <random>

#include <defs.h>
#include <rand.h>
#include <time/time.h>

namespace v::rand {
    static absl::Mutex                         g_mutex;
    static std::mt19937_64                     g_rng;
    static std::uniform_real_distribution<f64> g_unit01(0.0, 1.0);
    static u64                                 g_last_seed = 0;

    static u64 mix_seed(u64 a, u64 b)
    {
        // simple 64-bit mix, inspired by splitmix64
        u64 x = a + 0x9E3779B97F4A7C15ULL + (b << 1);
        x ^= x >> 30;
        x *= 0xBF58476D1CE4E5B9ULL;
        x ^= x >> 27;
        x *= 0x94D049BB133111EBULL;
        x ^= x >> 31;
        return x;
    }

    void seed(u64 seed)
    {
        absl::WriterMutexLock lock(&g_mutex);
        g_rng.seed(seed);
        g_last_seed = seed;
        // seed legacy C RNG for any existing code using std::rand()
        std::srand(static_cast<unsigned>(seed & 0xFFFF'FFFFu));
        LOG_DEBUG("Seeded RNG with {}", seed);
    }

    void init()
    {
        // high entropy seed from time and random_device
        std::random_device rd;
        u64 rdseed = (static_cast<u64>(rd()) << 32) ^ static_cast<u64>(rd());
        u64 tseed  = time::epoch_ns();
        u64 seedv  = mix_seed(rdseed, tseed);
        seed(seedv);
    }

    u64 last_seed()
    {
        absl::ReaderMutexLock lock(&g_mutex);
        return g_last_seed;
    }

    u64 next_u64()
    {
        absl::WriterMutexLock lock(&g_mutex);
        return g_rng();
    }

    u32 next_u32() { return static_cast<u32>(next_u64() & 0xFFFF'FFFFu); }

    f64 uniform()
    {
        absl::WriterMutexLock lock(&g_mutex);
        return g_unit01(g_rng);
    }

    f64 frange(f64 min, f64 max)
    {
        if (min > max)
            std::swap(min, max);
        absl::WriterMutexLock               lock(&g_mutex);
        std::uniform_real_distribution<f64> dist(min, max);
        return dist(g_rng);
    }

    i64 irange(i64 min, i64 max)
    {
        if (min > max)
            std::swap(min, max);
        absl::WriterMutexLock              lock(&g_mutex);
        std::uniform_int_distribution<i64> dist(min, max);
        return dist(g_rng);
    }

    u64 urange(u64 min, u64 max)
    {
        if (min > max)
            std::swap(min, max);
        absl::WriterMutexLock              lock(&g_mutex);
        std::uniform_int_distribution<u64> dist(min, max);
        return dist(g_rng);
    }

    bool chance(f64 p)
    {
        if (p <= 0.0)
            return false;
        if (p >= 1.0)
            return true;
        return uniform() < p;
    }
} // namespace v::rand

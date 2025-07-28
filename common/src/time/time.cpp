//
// Created by niooi on 7/28/2025.
//

#include <chrono>
#include <thread>
#include <time/time.h>

namespace v::time {
    static std::chrono::high_resolution_clock::time_point start_time;

    void init() { start_time = std::chrono::high_resolution_clock::now(); }

    f64 secs()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now - start_time);
        return static_cast<f64>(duration.count()) / 1e9;
    }

    u64 ns()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now - start_time);
        return static_cast<u64>(duration.count());
    }

    u64 ms()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - start_time);
        return static_cast<u64>(duration.count());
    }

    u64 epoch_ms()
    {
        auto now      = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                duration);
        return static_cast<u64>(millis.count());
    }

    u64 epoch_ns()
    {
        auto now      = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto nanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        return static_cast<u64>(nanos.count());
    }

    void sleep_ms(u32 ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void sleep_ns(u64 ns)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }
} // namespace v::time

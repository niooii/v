//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <containers/ud_set.h>
#include <coroutine>
#include <defs.h>
#include <queue>
#include <vector>

namespace v {
    /// Scheduler for coroutines running on the main thread.
    /// Manages sleeping coroutines via min-heap for efficient wake-up.
    /// Owns all coroutine handles and destroys them when completed.
    class CoroutineScheduler {
    public:
        CoroutineScheduler() = default;

        /// Register a coroutine handle for lifetime management
        void register_handle(std::coroutine_handle<> handle);

        /// Schedule a coroutine to sleep until the given wake time (nanoseconds)
        void schedule_sleep(std::coroutine_handle<> handle, u64 wake_time_ns);

        /// Update scheduler and resume ready coroutines.
        /// Should be called once per frame from main thread.
        void tick(u64 current_time_ns);

    private:
        struct SleepEntry {
            u64                     wake_time_ns;
            std::coroutine_handle<> handle;

            bool operator>(const SleepEntry& other) const
            {
                return wake_time_ns > other.wake_time_ns;
            }
        };

        // Min-heap of sleeping coroutines, sorted by wake_time_ns
        std::priority_queue<SleepEntry, std::vector<SleepEntry>, std::greater<>>
            sleeping_;

        // All active coroutine handles owned by scheduler
        v::ud_set<std::coroutine_handle<>> active_handles_;
    };
} // namespace v

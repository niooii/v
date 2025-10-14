//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <any>
#include <containers/ud_map.h>
#include <containers/ud_set.h>
#include <coroutine>
#include <defs.h>
#include <memory>
#include <queue>
#include <vector>


namespace v {

    template <typename T>
    struct CoroutineState;
    /// Scheduler for coroutines running on the main thread.
    /// Manages sleeping coroutines via min-heap for efficient wake-up.
    /// Owns all coroutine handles and destroys them when completed.
    class CoroutineScheduler {
        friend struct FinalAwaitable;

    public:
        CoroutineScheduler() = default;

        /// Register a coroutine handle for lifetime management
        void register_handle(std::coroutine_handle<> handle);

        /// Store heap-allocated coroutine state (including lambda and interface) to keep
        /// it alive for the duration of the coroutine
        template <typename T>
        void store_heap_state(std::coroutine_handle<> handle, std::shared_ptr<T> state)
        {
            heap_state_storage_[handle] = std::static_pointer_cast<void>(state);
        }

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

        /// Called when a coroutine exits via final_suspend (exception or co_return).
        void schedule_finish(std::coroutine_handle<> handle);

        // Min-heap of sleeping coroutines, sorted by wake_time_ns
        std::priority_queue<SleepEntry, std::vector<SleepEntry>, std::greater<>>
            sleeping_;

        ud_set<std::coroutine_handle<>> to_kill_{};

        // All active coroutine handles owned by scheduler
        v::ud_set<std::coroutine_handle<>> active_handles_;

        // We need to heap allocate stuff like lambdas and other temporary objects
        v::ud_map<std::coroutine_handle<>, std::shared_ptr<void>> heap_state_storage_;
    };
} // namespace v

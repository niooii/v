//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <coroutine>
#include <defs.h>
#include <time/time.h>

namespace v {
    class Engine;
    class CoroutineScheduler;

    /// Awaitable for sleeping a coroutine
    struct SleepAwaitable {
        u64                 duration_ms;
        CoroutineScheduler* scheduler;

        bool await_ready() const noexcept { return duration_ms == 0; }

        void await_suspend(std::coroutine_handle<> handle);

        void await_resume() const noexcept {}
    };

    /// Interface passed to coroutines for scheduler interaction
    class CoroutineInterface {
    public:
        explicit CoroutineInterface(CoroutineScheduler& scheduler, Engine& engine) :
            scheduler_(scheduler), engine_(engine)
        {}

        /// Suspend coroutine for the given duration in milliseconds.
        /// Returns true to allow use in while loops: while (ci.sleep(100)) { ... }
        SleepAwaitable sleep(u64 duration_ms)
        {
            return SleepAwaitable{ duration_ms, &scheduler_ };
        }

        Engine& engine() { return engine_; }

        CoroutineScheduler& scheduler() { return scheduler_; }

    private:
        CoroutineScheduler& scheduler_;
        Engine&             engine_;
    };
} // namespace v

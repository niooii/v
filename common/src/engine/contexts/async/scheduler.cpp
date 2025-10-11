//
// Created by niooi on 10/10/2025.
//

#include <engine/contexts/async/scheduler.h>

namespace v {
    void CoroutineScheduler::register_handle(std::coroutine_handle<> handle)
    {
        active_handles_.insert(handle);
    }

    void
    CoroutineScheduler::schedule_sleep(std::coroutine_handle<> handle, u64 wake_time_ns)
    {
        sleeping_.push(SleepEntry{ wake_time_ns, handle });
    }

    void CoroutineScheduler::tick(u64 current_time_ns)
    {
        // Resume sleeping coroutines that are ready
        while (!sleeping_.empty() && sleeping_.top().wake_time_ns <= current_time_ns)
        {
            auto entry = sleeping_.top();
            sleeping_.pop();
            entry.handle.resume();
        }

        // Clean up completed coroutines
        for (auto it = active_handles_.begin(); it != active_handles_.end();)
        {
            if (it->done())
            {
                it->destroy();
                it = active_handles_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
} // namespace v

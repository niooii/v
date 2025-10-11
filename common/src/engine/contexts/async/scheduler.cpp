//
// Created by niooi on 10/10/2025.
//

#include <engine/contexts/async/scheduler.h>

namespace v {
    void CoroutineScheduler::register_handle(std::coroutine_handle<> handle)
    {
        active_handles_.insert(handle);
    }

    void CoroutineScheduler::store_lambda(std::coroutine_handle<> handle, std::shared_ptr<void> lambda)
    {
        lambda_storage_[handle] = std::move(lambda);
    }

    void
    CoroutineScheduler::schedule_sleep(std::coroutine_handle<> handle, u64 wake_time_ns)
    {
        LOG_DEBUG("scheduled sleep");
        sleeping_.push(SleepEntry{ wake_time_ns, handle });
    }

    void CoroutineScheduler::schedule_finish(std::coroutine_handle<> handle) {
        handle.destroy();
        // TODO! set some more state here?
        active_handles_.erase(handle);
        lambda_storage_.erase(handle);
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
    }
} // namespace v

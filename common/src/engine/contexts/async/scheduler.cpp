//
// Created by niooi on 10/10/2025.
//

#include <engine/contexts/async/scheduler.h>

namespace v {
    void CoroutineScheduler::register_handle(std::coroutine_handle<> handle)
    {
        active_handles_.insert(handle);
    }

    void CoroutineScheduler::store_heap_state(
        std::coroutine_handle<> handle, std::shared_ptr<CoroutineState> state)
    {
        heap_state_storage_[handle] = std::move(state);
    }

    void
    CoroutineScheduler::schedule_sleep(std::coroutine_handle<> handle, u64 wake_time_ns)
    {
        sleeping_.push(SleepEntry{ wake_time_ns, handle });
    }

    void CoroutineScheduler::schedule_finish(std::coroutine_handle<> handle)
    {
        handle.destroy();
        // TODO! set some more state here?
        active_handles_.erase(handle);
        heap_state_storage_.erase(handle);
    }

    void CoroutineScheduler::tick(u64 current_time_ns)
    {
        // Resume sleeping coroutines that are ready
        while (!sleeping_.empty() && sleeping_.top().wake_time_ns <= current_time_ns)
        {
            auto entry = sleeping_.top();
            sleeping_.pop();
            entry.handle.resume();
            // TODO! make it so that the same coroutine can't be woken twice
            // in a single tick (key by tuple<wake_time, tick_slept> but 
            // still sort by wake_time, just check if tick_slept == tick(), if not dont
            // resume that task and keep going up SO MAYBE need diff data structure?)
        }
    }
} // namespace v

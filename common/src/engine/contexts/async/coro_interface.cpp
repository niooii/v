//
// Created by niooi on 10/10/2025.
//

#include <engine/contexts/async/coro_interface.h>
#include <engine/contexts/async/scheduler.h>

namespace v {
    void SleepAwaitable::await_suspend(std::coroutine_handle<> handle)
    {
        u64 wake_time_ns = time::ns() + (duration_ms * 1'000'000);
        scheduler->schedule_sleep(handle, wake_time_ns);
    }
} // namespace v

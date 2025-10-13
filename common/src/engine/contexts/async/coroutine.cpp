//
// Created by niooi on 10/11/2025.
//

#include <engine/contexts/async/coroutine.h>
#include <engine/contexts/async/scheduler.h>

namespace v {
    void FinalAwaitable::await_suspend(std::coroutine_handle<> handle) noexcept
    {
        scheduler.schedule_finish(handle);
    }
} // namespace v

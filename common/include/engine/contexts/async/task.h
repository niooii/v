//
// Created by niooi on 9/28/2025.
//

#pragma once

#include <engine/contexts/async/future.h>
#include <engine/contexts/async/task_state.h>
#include <future>

namespace v {
    template <typename T>
    class Task : public FutureBase<Task<T>, T> {
        friend class AsyncContext;

    public:
        Task(Engine& engine) : state_(std::make_shared<TaskState<T>>(engine)) {}

        void wait() const { future_.wait(); }

        template <typename Rep, typename Period>
        void wait_for(const std::chrono::duration<Rep, Period>& rel) const
        {
            future_.wait_for(rel);
        }

        // CRTP interface implementations
        template <typename Callback>
        Task& then_impl(Callback&& callback)
        {
            if constexpr (std::is_void_v<T>)
            {
                static_assert(
                    std::is_invocable_v<Callback>,
                    "Callback for Task<void>.then() must be callable with no arguments: "
                    "[](){ ... }");
            }
            else
            {
                static_assert(
                    std::is_invocable_v<Callback, T>,
                    "Callback for Task<T>.then() must be callable with T as argument: "
                    "[](T value){ ... }");
            }

            auto guard = state_->lock.write();
            if (state_->is_completed && !state_->stored_exception)
            {
                // immediately queue
                if constexpr (std::is_void_v<T>)
                {
                    state_->engine.post_tick(callback);
                }
                else
                {
                    state_->engine.post_tick(std::bind(callback, future_.get()));
                }
            }
            else
            {
                state_->callback = callback;
            }
            return *this;
        }

        template <typename Callback>
        Task& or_else_impl(Callback&& callback)
        {
            static_assert(
                std::is_invocable_v<Callback, std::exception_ptr>,
                "Callback for .or_else() must be callable with std::exception_ptr: "
                "[](std::exception_ptr e){ ... }");

            auto guard = state_->lock.write();
            if (state_->is_completed && state_->stored_exception)
            {
                // immediately queue
                state_->engine.post_tick(std::bind(callback, state_->stored_exception));
            }
            else
            {
                state_->error_callback = callback;
            }
            return *this;
        }

        T get_impl() { return future_.get(); }

        void set_future(std::future<T>&& future) { future_ = std::move(future); }

    private:
        std::future<T>                future_;
        std::shared_ptr<TaskState<T>> state_;
    };
} // namespace v

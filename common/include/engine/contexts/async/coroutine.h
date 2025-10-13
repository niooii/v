//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <any>
#include <coroutine>
#include <defs.h>
#include <engine/contexts/async/coro_interface.h>
#include <engine/contexts/async/future.h>
#include <engine/contexts/async/scheduler.h>
#include <engine/engine.h>

namespace v {

    // Base for promise return handling - CRTP
    template <typename T, typename Derived>
    struct PromiseReturnBase {
        void return_value(T value)
        {
            auto* derived                 = static_cast<Derived*>(this);
            auto  guard                   = derived->state_->lock.write();
            derived->state_->is_completed = true;
            if (derived->state_->callback)
            {
                derived->state_->engine.post_tick(
                    std::bind(derived->state_->callback, value));
            }
        }
    };

    template <typename Derived>
    struct PromiseReturnBase<void, Derived> {
        void return_void()
        {
            auto* derived                 = static_cast<Derived*>(this);
            auto  guard                   = derived->state_->lock.write();
            derived->state_->is_completed = true;
            if (derived->state_->callback)
            {
                derived->state_->engine.post_tick(derived->state_->callback);
            }
        }
    };

    /// The final returned awaitable for the scheduler to destroy resources and stuff
    struct FinalAwaitable {
        CoroutineScheduler& scheduler;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> handle) noexcept;

        void await_resume() const noexcept {}
    };

    template <typename T>
    class Coroutine : public Future<T> {
    public:
        using value_type = T;

        struct promise_type : PromiseReturnBase<T, promise_type> {
            promise_type(CoroutineInterface& ci) :
                engine_(ci.engine()), scheduler_(ci.scheduler())
            {}

            Coroutine<T> get_return_object()
            {
                auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
                return Coroutine<T>(handle, engine_, scheduler_);
            }

            Engine&             engine_;
            CoroutineScheduler& scheduler_;

            std::suspend_never initial_suspend() noexcept { return {}; }

            FinalAwaitable final_suspend() noexcept
            {
                return { .scheduler = scheduler_ };
            }

            void unhandled_exception()
            {
                auto guard               = state_->lock.write();
                state_->is_completed     = true;
                state_->stored_exception = std::current_exception();
                if (state_->error_callback)
                {
                    state_->engine.post_tick(
                        std::bind(state_->error_callback, state_->stored_exception));
                }
            }

            std::shared_ptr<FutureState<T>> state_;
        };

        using handle_type = std::coroutine_handle<promise_type>;

        explicit Coroutine(handle_type h, Engine& engine, CoroutineScheduler& scheduler) :
            Future<T>(engine), handle_(h)
        {
            h.promise().state_ = this->state_;
            scheduler.register_handle(h);
        }

        ~Coroutine() = default;

        bool done() const { return this->state_->is_completed; }

        // Get the underlying coroutine handle
        std::coroutine_handle<> handle() const { return handle_; }

        // Delete copy, default move
        Coroutine(const Coroutine&)            = delete;
        Coroutine& operator=(const Coroutine&) = delete;
        Coroutine(Coroutine&&)                 = default;
        Coroutine& operator=(Coroutine&&)      = default;

    private:
        handle_type handle_;
    };
} // namespace v

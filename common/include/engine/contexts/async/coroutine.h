//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <any>
#include <coroutine>
#include <defs.h>
#include <engine/contexts/async/coro_interface.h>
#include <engine/contexts/async/coroutine_state.h>
#include <engine/contexts/async/future.h>
#include <engine/contexts/async/scheduler.h>
#include <engine/engine.h>

namespace v {

    // TODO! why not just merge these lol
    template <typename Ret, typename Derived>
    struct PromiseReturnBase {
        void return_value(Ret value)
        {
            auto* derived                      = static_cast<Derived*>(this);
            derived->coro_state_->is_completed = true;
            derived->coro_state_->value        = value;
            if (derived->coro_state_->callback)
            {
                derived->coro_state_->callback(value);
            }
        }
    };

    template <typename Derived>
    struct PromiseReturnBase<void, Derived> {
        void return_void()
        {
            auto* derived                      = static_cast<Derived*>(this);
            derived->coro_state_->is_completed = true;
            if (derived->coro_state_->callback)
                derived->coro_state_->callback();
        }
    };

    /// The final returned awaitable for the scheduler to destroy resources and stuff
    struct FinalAwaitable {
        CoroutineScheduler& scheduler;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> handle) noexcept;

        void await_resume() const noexcept {}
    };

    template <typename Ret>
    class Coroutine : public FutureBase<Coroutine<Ret>, Ret> {
    public:
        using value_type = Ret;

        struct promise_type : PromiseReturnBase<Ret, promise_type> {
            promise_type(CoroutineInterface& ci) :
                engine_(ci.engine()), scheduler_(ci.scheduler())
            {}

            Coroutine<Ret> get_return_object()
            {
                auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
                return Coroutine<Ret>(handle, engine_, scheduler_);
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
                coro_state_->is_completed     = true;
                coro_state_->stored_exception = std::current_exception();
                if (coro_state_->error_callback)
                    coro_state_->error_callback(coro_state_->stored_exception);
            }

            std::shared_ptr<CoroutineState<Ret>> coro_state_;
        };

        using handle_type = std::coroutine_handle<promise_type>;

        explicit Coroutine(handle_type h, Engine& engine, CoroutineScheduler& scheduler) :
            FutureBase<Coroutine<Ret>, Ret>(),
            coro_state_(std::make_shared<CoroutineState<Ret>>(engine)), handle_(h),
            scheduler_(scheduler)
        {
            h.promise().coro_state_ = coro_state_;
            scheduler.register_handle(h);
        }

        /// TODO! actually should kill this on destroy, force raii
        ~Coroutine()
        {
            // stop();
        }

        /// If the coroutine has finished executing normally
        /// via a thrown exception or return
        FORCEINLINE bool done() const { return coro_state_->is_completed; }

        /// If the coroutine is still alive and running.
        /// This is different from done(), as done() will return false for
        /// a manually killed coroutine.
        FORCEINLINE bool alive() const { TODO() }

        // TODO! pause, resume, stop

        /// Kills the coroutine since if this is called it is guarenteed to be yielded.
        /// (OR NOT?? is this correct usage??) use await_transform probably
        FORCEINLINE void stop(){ TODO() }

        FORCEINLINE void pause(){ TODO() }

        FORCEINLINE void resume(){ TODO() }

        // Get the underlying coroutine handle
        std::coroutine_handle<> handle() const
        {
            return handle_;
        }

        // Delete copy, default move
        Coroutine(const Coroutine&)            = delete;
        Coroutine& operator=(const Coroutine&) = delete;
        Coroutine(Coroutine&&)                 = default;
        Coroutine& operator=(Coroutine&&)      = default;

        // CRTP interface implementations
        template <typename Callback>
        Coroutine& then_impl(Callback&& callback)
        {
            if constexpr (std::is_void_v<Ret>)
            {
                static_assert(
                    std::is_invocable_v<Callback>,
                    "Callback for Coroutine<void>.then() must be callable with no "
                    "arguments: [](){ ... }");
            }
            else
            {
                static_assert(
                    std::is_invocable_v<Callback, Ret>,
                    "Callback for Coroutine<T>.then() must be callable with T as "
                    "argument: [](T value){ ... }");
            }

            if (coro_state_->is_completed && !coro_state_->stored_exception)
            {
                if constexpr (std::is_void_v<Ret>)
                    callback();
                else
                    callback(*coro_state_->value);
            }
            else
            {
                coro_state_->callback = callback;
            }
            return *this;
        }

        template <typename Callback>
        Coroutine& or_else_impl(Callback&& callback)
        {
            static_assert(
                std::is_invocable_v<Callback, std::exception_ptr>,
                "Callback for .or_else() must be callable with std::exception_ptr: "
                "[](std::exception_ptr e){ ... }");

            if (coro_state_->is_completed && coro_state_->stored_exception)
                // immediately execute error callback since this all happens on the main
                // thread
                callback(coro_state_->stored_exception);
            else
                coro_state_->error_callback = callback;
            return *this;
        }

        Ret get_impl()
        {
            if (coro_state_->stored_exception)
                std::rethrow_exception(coro_state_->stored_exception);
            if constexpr (!std::is_void_v<Ret>)
                return *coro_state_->value;
        }

    private:
        std::shared_ptr<CoroutineState<Ret>> coro_state_;
        handle_type                          handle_;
        CoroutineScheduler&                  scheduler_;
    };
} // namespace v

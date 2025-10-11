//
// Created by niooi on 9/28/2025.
//

#pragma once

#include <defs.h>
#include <engine/engine.h>
#include <future>
#include <memory>
#include "engine/sync.h"

/// Future mirrors the std::future API for the most part

namespace v {
    template <typename T>
    class Coroutine;

    template <typename T>
    struct FutureState {
        Engine&                                 engine;
        std::atomic_bool                        is_completed{ false };
        std::function<void(T)>                  callback{};
        std::function<void(std::exception_ptr)> error_callback{};
        std::exception_ptr                      stored_exception{};
        RwLock<int>                             lock{};

        explicit FutureState(Engine& eng) : engine(eng) {}
    };

    template <>
    struct FutureState<void> {
        Engine&                                 engine;
        std::atomic_bool                        is_completed{ false };
        std::function<void()>                   callback{};
        std::function<void(std::exception_ptr)> error_callback{};
        std::exception_ptr                      stored_exception{};
        RwLock<int>                             lock{};

        explicit FutureState(Engine& eng) : engine(eng) {}
    };

    template <typename T>
    class Future {
        friend class AsyncContext;
        friend class Coroutine<T>;

    public:
        Future(Engine& engine) : state_(std::make_shared<FutureState<T>>(engine)) {}

        ~Future() = default;

        // Move constructor and assignment
        Future(Future&&)            = default;
        Future& operator=(Future&&) = default;

        // Delete copy operations
        Future(const Future&)            = delete;
        Future& operator=(const Future&) = delete;

        /// Register a callback to run on the engine's main thread after the successful
        /// completion of the Future. May only be called once, calling more than once has
        /// undefined behavior.
        template <typename Callback>
        Future& then(Callback&& callback)
        {
            if constexpr (std::is_void_v<T>)
            {
                static_assert(
                    std::is_invocable_v<Callback>,
                    "Callback for Future<void>.then() must be callable with no "
                    "arguments: [](){ ... }");
            }
            else
            {
                static_assert(
                    std::is_invocable_v<Callback, T>,
                    "Callback for Future<T>.then() must be callable with T as argument: "
                    "[](T value){ ... }");
            }

            // TODO! might as well wrap FutureState in a RwLock<>?? what am i doing
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

        /// Register a callback to run on the engine's main thread after an exception has
        /// been thrown from the Future. May only be called once, calling more than once
        /// has undefined behavior.
        template <typename Callback>
        Future& or_else(Callback&& callback)
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

        void wait() const { future_.wait(); }

        template <typename Rep, typename Period>
        void wait_for(const std::chrono::duration<Rep, Period>& rel) const
        {
            future_.wait_for(rel);
        }

        T get() { return future_.get(); }

    private:
        std::future<T>                  future_;
        std::shared_ptr<FutureState<T>> state_;
    };
} // namespace v

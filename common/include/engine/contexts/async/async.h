//
// Created by niooi on 9/28/2025.
//

#pragma once

#include <defs.h>
#include <engine/context.h>
#include <engine/contexts/async/coro_interface.h>
#include <engine/contexts/async/coroutine.h>
#include <engine/contexts/async/future.h>
#include <engine/contexts/async/scheduler.h>
#include <engine/contexts/async/task.h>
#include <optional>
#include "taskflow/taskflow.hpp"

namespace v {
    // Extended CoroutineState that includes the lambda and interface storage
    template <typename T>
    struct ExtendedCoroutineState : public CoroutineState<T> {
        std::shared_ptr<void> lambda; // The coroutine lambda
        std::shared_ptr<CoroutineInterface>
            iface; // The interface (scheduler/engine access)

        template <typename F>
        ExtendedCoroutineState(F&& func, CoroutineScheduler& scheduler, Engine& engine) :
            CoroutineState<T>(engine),
            lambda(std::make_shared<std::decay_t<F>>(std::forward<F>(func))),
            iface(std::make_shared<CoroutineInterface>(scheduler, engine))
        {}
    };

    template <>
    struct ExtendedCoroutineState<void> : public CoroutineState<void> {
        std::shared_ptr<void> lambda; // The coroutine lambda
        std::shared_ptr<CoroutineInterface>
            iface; // The interface (scheduler/engine access)

        template <typename F>
        ExtendedCoroutineState(F&& func, CoroutineScheduler& scheduler, Engine& engine) :
            CoroutineState<void>(engine),
            lambda(std::make_shared<std::decay_t<F>>(std::forward<F>(func))),
            iface(std::make_shared<CoroutineInterface>(scheduler, engine))
        {}
    };

    class AsyncContext : public Context {
    public:
        AsyncContext(Engine& engine, u16 num_threads) :
            Context(engine), executor_(num_threads)
        {
            // makes sure this runs first before any domains are destroyed, since it will
            // probably be running things for domains
            engine.on_destroy.connect(
                {}, {}, "async_finish",
                [&]
                {
                    LOG_TRACE("Waiting for executor tasks to finish...");
                    this->executor_.wait_for_all();
                    LOG_TRACE("Executor tasks finish");
                });

            // TODO! don't do this but this actually segfaults for some reason..
            // migiht be an issue?
            // wait nope its a async coro tick issue
            // whatever it runs on update() anyways
            // engine.on_tick.connect(
            //{}, {}, "async_coro_tick", [this] { scheduler_.tick(v::time::ns()); });
        }

        ~AsyncContext() { executor_.wait_for_all(); }

        // Ticks the coroutine scheduler
        void update() { scheduler_.tick(v::time::ns()); }

        template <typename Ret>
        Task<Ret> task(std::function<Ret(void)> func)
        {
            Task<Ret> ret{ engine_ };
            auto      state = ret.state_;

            auto f = executor_.async(
                [func, state]()
                {
                    try
                    {
                        // if its void then dont store the ret value
                        if constexpr (std::is_void_v<Ret>)
                        {
                            func();

                            auto guard          = state->lock.write();
                            state->is_completed = true;
                            if (state->callback)
                                state->engine.post_tick(state->callback);
                        }
                        else
                        {
                            Ret result = func();

                            auto guard          = state->lock.write();
                            state->is_completed = true;
                            if (state->callback)
                                state->engine.post_tick(
                                    std::bind(state->callback, result));

                            return result;
                        }
                    }
                    catch (...)
                    {
                        {
                            auto guard              = state->lock.write();
                            state->is_completed     = true;
                            state->stored_exception = std::current_exception();
                            if (state->error_callback)
                            {
                                state->engine.post_tick(
                                    std::bind(
                                        state->error_callback, state->stored_exception));
                            }
                        }
                        throw; // throw again to keep std::future exception semantics
                    }
                });
            ret.set_future(std::move(f));

            return std::move(ret);
        }

        // Template argument deduction helper
        template <typename F>
        auto task(F&& func) -> Task<std::invoke_result_t<F>>
        {
            using Ret = std::invoke_result_t<F>;
            return task<Ret>(std::function<Ret(void)>(std::forward<F>(func)));
        }

        /// Spawn a coroutine that runs on the main thread.
        template <typename CoroRet, typename F>
        CoroRet spawn(F&& coro_fn)
        {
            // Extract the inner type from CoroRet (e.g., void from Coroutine<void>)
            using InnerRet = typename CoroRet::value_type;

            // Create centralized state (heap-allocates lambda and interface)
            auto state = std::make_shared<ExtendedCoroutineState<InnerRet>>(
                std::forward<F>(coro_fn), scheduler_, engine_);

            // Call from the stable heap locations
            auto coro = (*std::static_pointer_cast<std::decay_t<F>>(state->lambda))(
                *state->iface);

            // Store the state to keep everything alive
            scheduler_.store_heap_state(coro.handle(), state);

            return coro;
        }

        /// Spawn a coroutine that runs on the main thread.
        template <typename F>
        auto spawn(F&& coro_fn)
        {
            using CoroRet = std::invoke_result_t<F, CoroutineInterface&>;
            return spawn<CoroRet>(std::forward<F>(coro_fn));
        }


        /// Get the coroutine scheduler
        CoroutineScheduler& scheduler() { return scheduler_; }

    private:
        tf::Executor       executor_;
        CoroutineScheduler scheduler_;
    };
} // namespace v

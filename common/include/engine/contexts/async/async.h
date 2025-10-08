//
// Created by niooi on 9/28/2025.
//

#pragma once

#include <defs.h>
#include <engine/context.h>
#include <engine/contexts/async/future.h>
#include "taskflow/core/executor.hpp"

namespace v {
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
        }

        ~AsyncContext() { executor_.wait_for_all(); }

        template <typename Ret>
        Future<Ret> task(std::function<Ret(void)> func)
        {
            Future<Ret> ret{ engine_ };
            auto        state = ret.state_;

            std::future<Ret> f = executor_.async(
                [func, state]() -> Ret
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
            ret.future_ = std::move(f);

            return std::move(ret);
        }

        // Template argument deduction helper
        template <typename F>
        auto task(F&& func) -> Future<std::invoke_result_t<F>>
        {
            using Ret = std::invoke_result_t<F>;
            return task<Ret>(std::function<Ret(void)>(std::forward<F>(func)));
        }

    private:
        tf::Executor executor_;
    };
} // namespace v

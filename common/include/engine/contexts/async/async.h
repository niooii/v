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
        AsyncContext(Engine& engine, u16 num_threads) : Context(engine), executor_(num_threads) {
            
        }

        template <typename Ret>
        Future<Ret> task(std::function<Ret(void)> func) {
            engine_.post_tick([] {

                              });
        }

    private:
        tf::Executor executor_;
    };
}

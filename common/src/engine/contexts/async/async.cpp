//
// Created by niooi on 9/28/2025.
//

#include <async/async.h>
#include <prelude.h>
#include "taskflow/core/executor.hpp"
#include "taskflow/core/taskflow.hpp"

static tf::Executor executor;

namespace v::async {
    static std::optional<tf::Executor> executor;

    void init(u16 num_threads) { executor.emplace(num_threads); }

    void join_all() { executor->wait_for_all(); }
} // namespace v::async

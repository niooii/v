//
// Created by niooi on 9/28/2025.
//

#pragma once

#include <atomic>
#include <engine/engine.h>
#include <engine/sync.h>
#include <exception>
#include <functional>

namespace v {
    template <typename T>
    struct TaskState {
        Engine&                                 engine;
        std::atomic_bool                        is_completed{ false };
        std::function<void(T)>                  callback{};
        std::function<void(std::exception_ptr)> error_callback{};
        std::exception_ptr                      stored_exception{};
        RwLock<int>                             lock{};

        explicit TaskState(Engine& eng) : engine(eng) {}
    };

    template <>
    struct TaskState<void> {
        Engine&                                 engine;
        std::atomic_bool                        is_completed{ false };
        std::function<void()>                   callback{};
        std::function<void(std::exception_ptr)> error_callback{};
        std::exception_ptr                      stored_exception{};
        RwLock<int>                             lock{};

        explicit TaskState(Engine& eng) : engine(eng) {}
    };
} // namespace v

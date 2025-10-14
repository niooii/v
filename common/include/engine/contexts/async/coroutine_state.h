//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <engine/engine.h>
#include <exception>
#include <functional>
#include <optional>

namespace v {
    // CoroutineState for single-threaded coroutines (no synchronization needed)
    template <typename T>
    struct CoroutineState {
        Engine&                                 engine;
        bool                                    is_completed{ false };
        std::optional<T>                        value{};
        std::function<void(T)>                  callback{};
        std::function<void(std::exception_ptr)> error_callback{};
        std::exception_ptr                      stored_exception{};

        explicit CoroutineState(Engine& eng) : engine(eng) {}
    };

    template <>
    struct CoroutineState<void> {
        Engine&                                 engine;
        bool                                    is_completed{ false };
        std::function<void()>                   callback{};
        std::function<void(std::exception_ptr)> error_callback{};
        std::exception_ptr                      stored_exception{};

        explicit CoroutineState(Engine& eng) : engine(eng) {}
    };
} // namespace v

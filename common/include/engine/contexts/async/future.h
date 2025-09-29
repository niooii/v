//
// Created by niooi on 9/28/2025.
//

#pragma once

#include <defs.h>
#include <future>
#include <engine/engine.h>
#include "engine/contexts/async/async.h"

/// Future mirrors the std::future API for the most part

namespace v {
    template <typename T>
    class Future {
    friend class AsyncContext;

    public:
        Future(std::future<T>&& future, Engine& engine) : future_(std::move(future)) { TODO(); }
        ~Future() {
            // TODO! remove some internal state tracking or something
        }

        /// Register a callback to run on the engine's main thread after the successful
        /// completion of the Future
        Future& then(std::function<void(T)> callback) { TODO(); }

        /// Register a callback to run on the engine's main thread after an exception has been
        /// thrown from the Future
        Future& or_else(std::function<void(std::exception_ptr)> callback) { TODO(); }

        void wait() const { future_.wait(); }

        template <typename Rep, typename Period>
        void wait_for(const std::chrono::duration<Rep, Period>& rel) const
        {
            future_.wait_for(rel);
        }

        T get() { return future_.get(); }

    private:
        std::future<T> future_;
    };
}

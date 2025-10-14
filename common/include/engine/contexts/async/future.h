//
// Created by niooi on 9/28/2025.
//

#pragma once

#include <defs.h>
#include <engine/engine.h>
#include <memory>

/// CRTP interface for async operations (Task and Coroutine)

namespace v {
    template <typename T>
    class Coroutine;

    template <typename Derived, typename T>
    class FutureBase {
    protected:
        Derived&       derived() { return static_cast<Derived&>(*this); }
        const Derived& derived() const { return static_cast<const Derived&>(*this); }

    public:
        FutureBase() = default;

        // Delete copy operations
        FutureBase(const FutureBase&)            = delete;
        FutureBase& operator=(const FutureBase&) = delete;

        // Move constructor and assignment
        FutureBase(FutureBase&&)            = default;
        FutureBase& operator=(FutureBase&&) = default;

        ~FutureBase() = default;

        /// Register a callback to run after successful completion.
        /// May only be called once, calling more than once has undefined behavior.
        template <typename Callback>
        Derived& then(Callback&& callback)
        {
            return derived().then_impl(std::forward<Callback>(callback));
        }

        /// Register a callback to run after an exception has been thrown.
        /// May only be called once, calling more than once has undefined behavior.
        template <typename Callback>
        Derived& or_else(Callback&& callback)
        {
            return derived().or_else_impl(std::forward<Callback>(callback));
        }

        T get() { return derived().get_impl(); }
    };
} // namespace v

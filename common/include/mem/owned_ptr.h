//
// Created by niooi on 10/17/2025.
//

#pragma once

#include <defs.h>
#include <memory>

/// A unique ptr, except it has a constructor that forwards the arguments to the type
namespace v::mem {
    template <typename T>
    struct owned_ptr {
        std::unique_ptr<T> ptr;

        // Default constructor - creates null pointer
        constexpr owned_ptr() noexcept : ptr(nullptr) {}

        // Forwarding constructor - construct T in-place with args
        // Only enabled when args are provided
        template <typename... Args>
            requires(sizeof...(Args) > 0)
        explicit owned_ptr(Args&&... args) :
            ptr(std::make_unique<T>(std::forward<Args>(args)...))
        {}

        owned_ptr(owned_ptr&&) noexcept            = default;
        owned_ptr& operator=(owned_ptr&&) noexcept = default;

        owned_ptr(const owned_ptr&)            = delete;
        owned_ptr& operator=(const owned_ptr&) = delete;

        FORCEINLINE T*       operator->() noexcept { return ptr.get(); }
        FORCEINLINE const T* operator->() const noexcept { return ptr.get(); }

        FORCEINLINE T&       operator*() noexcept { return *ptr; }
        FORCEINLINE const T& operator*() const noexcept { return *ptr; }

        FORCEINLINE T*       get() noexcept { return ptr.get(); }
        FORCEINLINE const T* get() const noexcept { return ptr.get(); }

        FORCEINLINE explicit operator bool() const noexcept
        {
            return static_cast<bool>(ptr);
        }
    };
} // namespace v::mem

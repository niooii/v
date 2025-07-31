//
// Created by niooi on 7/30/2025.
// Example usages can be found in the engine.h header, but essentially,
// for these classes to function as proper RW guards, they must both use the
// same shared_mutex.
//

#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>

/// A read guard for accessing non-thread-safe resources
template<typename T>
class ReadProtectedResourceGuard {
public:
    ReadProtectedResourceGuard(const T& resource, std::shared_mutex& mtx)
        : resource_(resource), lock_(mtx) {}

    const T* operator->() const { return &resource_; }
    const T& operator*() const { return resource_; }

    /// Manually releases the lock before the guard goes out of scope.
    /// Use with caution: you almost never want to call this
    void release() {
        if (lock_.owns_lock())
            lock_.unlock();
    }

private:
    const T& resource_;
    std::shared_lock<std::shared_mutex> lock_;
};

/// A write guard for accessing non-thread-safe resources
template<typename T>
class WriteProtectedResourceGuard {
public:
    WriteProtectedResourceGuard(T& resource, std::shared_mutex& mtx)
        : resource_(resource), lock_(mtx) {}

    T* operator->() { return &resource_; }
    T& operator*() { return resource_; }

    /// Manually releases the lock before the guard goes out of scope.
    /// Use with caution: you almost never want to call this
    void release() {
        if (lock_.owns_lock())
            lock_.unlock();
    }

private:
    T& resource_;
    std::unique_lock<std::shared_mutex> lock_;
};

/// A wrapper class for a resource that should be protected by a RW-lock at all times.
/// Inspired by rust's RwLock<T> pattern.
template<typename T>
class RWProtectedResource {
public:
    template<typename... Args>
    explicit RWProtectedResource(Args&&... args)
        : obj_(std::forward<Args>(args)...) {}

    /// Gets a read guard for shared, const access to the resource
    ReadProtectedResourceGuard<T> read() const {
        return ReadProtectedResourceGuard<T>(obj_, shared_mutex_);
    }

    /// Gets a write guard for exclusive, non-const access to the resource
    WriteProtectedResourceGuard<T> write() {
        return WriteProtectedResourceGuard<T>(obj_, shared_mutex_);
    }

private:
    T obj_;
    /// Mutex is mutable so const read() method can get a lock
    mutable std::shared_mutex shared_mutex_;
};

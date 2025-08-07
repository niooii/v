//
// Created by niooi on 7/30/2025.
// Example usages can be found in the engine.h header, but essentially,
// for these classes to function as proper RW guards, they must both use the
// same shared_mutex.
//

#pragma once

#include <memory>
#include <absl/synchronization/mutex.h>

/// A read guard for accessing non-thread-safe resources
template<typename T>
class ReadProtectedResourceGuard {
public:
    ReadProtectedResourceGuard(const T& resource, absl::Mutex* mtx)
        : resource_(resource), lock_(std::make_unique<absl::ReaderMutexLock>(mtx)) {}

    const T* operator->() const { return &resource_; }
    const T& operator*() const { return resource_; }

private:
    const T& resource_;
    // unique ptr so the lock can be moved
    std::unique_ptr<absl::ReaderMutexLock> lock_;
};

/// A wrute guard for accessing non-thread-safe resources
template<typename T>
class WriteProtectedResourceGuard {
public:
    WriteProtectedResourceGuard(T& resource, absl::Mutex* mtx)
        : resource_(resource), lock_(std::make_unique<absl::WriterMutexLock>(mtx)) {}

    T* operator->() { return &resource_; }
    T& operator*() { return resource_; }

private:
    T& resource_;
    std::unique_ptr<absl::WriterMutexLock> lock_;
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
        return ReadProtectedResourceGuard<T>(obj_, &mutex_);
    }

    /// Gets a write guard for exclusive, non-const access to the resource
    WriteProtectedResourceGuard<T> write() {
        return WriteProtectedResourceGuard<T>(obj_, &mutex_);
    }

private:
    T obj_;
    /// Mutex is mutable so const read() method can get a lock
    mutable absl::Mutex mutex_;
};

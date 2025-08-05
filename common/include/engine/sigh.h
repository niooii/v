//
// Created by niooi on 8/3/25.
//

#pragma once

#include <absl/container/btree_set.h>

#include <defs.h>

template <typename T>
struct PriorityCallback {
    i64 priority;
    std::function<T> func;

    #ifndef V_RELEASE
    std::string name;
    #endif

    std::strong_ordering operator<=>(const PriorityCallback& other) const
    {
        return other.priority <=> priority;
    }
};

/// Combines the Sink and Sigh for now - not ideal
template <typename T>
class PrioritySink {
    public:
    FORCEINLINE void connect(i64 priority, const std::string& name, std::function<T> func)
    {
        PriorityCallback<T> callback;
        callback.priority = priority;
        callback.func = func;
        #ifndef V_RELEASE
        callback.name = std::move(name);
        #endif

        callbacks_.insert(std::move(callback));
    }

    /// TODO! bind this to the arguments of the std::function only? or no?
    template <typename... Args>
    FORCEINLINE void publish(Args&&... args)
    {
        for (auto& callback : callbacks_)
        {
            LOG_DEBUG("running callback {} with prio {}", callback.name, callback.priority);
            // std::cout << callback.name << " " << callback.priority << std::endl;
            callback.func(std::forward<Args>(args)...);
        }
    }


private:
    absl::btree_multiset<PriorityCallback<T>> callbacks_;
};
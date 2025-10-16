//
// Template implementations for DomainBase
// included at the end of engine.h to avoid circular include
//

#pragma once

namespace v {
    template <typename T>
    bool DomainBase::has() const
    {
        return engine_.has_component<T>(entity_);
    }

    template <typename T>
    T* DomainBase::try_get()
    {
        return engine_.try_get_component<T>(entity_);
    }

    template <typename T>
    const T* DomainBase::try_get() const
    {
        return engine_.try_get_component<T>(entity_);
    }

    template <typename T>
    T& DomainBase::get()
    {
        return engine_.get_component<T>(entity_);
    }

    template <typename T>
    const T& DomainBase::get() const
    {
        return engine_.get_component<T>(entity_);
    }

    template <typename T, typename... Args>
    T& DomainBase::attach(Args&&... args)
    {
        return engine_.add_component<T>(entity_, std::forward<Args>(args)...);
    }

    template <typename T>
    usize DomainBase::remove()
    {
        return engine_.remove_component<T>(entity_);
    }
} // namespace v

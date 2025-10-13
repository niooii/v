//
// Created by niooi on 7/28/2025.
//

#include <engine/domain.h>
#include <engine/engine.h>

namespace v {
    DomainBase::DomainBase(Engine& engine, std::string name) :
        engine_(engine), name_(std::move(name))
    {
        entity_ = engine_.registry().create();
    }

    DomainBase::~DomainBase()
    {
        // Entity lifetime is managed externally; do not access other domains/contexts
        // here.
    }

    // Template implementations for domain registry shorthands

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

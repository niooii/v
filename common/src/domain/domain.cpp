//
// Created by niooi on 7/28/2025.
//

#include <domain/domain.h>
#include <engine/engine.h>

#include <utility>

namespace v {
    Domain::Domain(Engine& engine, std::string  name)
        : engine_(engine), name_(std::move(name))
    {
        auto& reg = engine_.domain_registry();
        entity_ = reg.create();

        reg.emplace<Domain*>(entity_, this);
    }

    Domain::~Domain()
    {
        auto& reg = engine_.domain_registry();

        reg.destroy(entity_);
    }

    template <typename T, typename... Args>
    T& Domain::attach_component(Args&&... args)
    {
        auto& reg = engine_.domain_registry();

        auto& component = reg.emplace_or_replace<T>(entity_, std::forward<Args>(args)...);
        return component;
    }

    template <typename T>
    T* Domain::get_component()
    {
        const auto& reg = engine_.domain_registry();

        auto component = reg.try_get<T>(entity_);

        return component;
    }
}
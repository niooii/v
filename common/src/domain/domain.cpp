//
// Created by niooi on 7/28/2025.
//

#include <domain/domain.h>
#include <engine/engine.h>

#include <utility>

namespace v {
    Domain::Domain(Engine& engine, std::string name) :
        engine_(engine), name_(std::move(name))
    {
        auto& reg = engine_.domain_registry();
        entity_   = reg.create();

        reg.emplace<Domain*>(entity_, this);
    }
} // namespace v

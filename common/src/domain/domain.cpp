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
        entity_ = engine_.registry().create();
    }

    Domain::~Domain()
    {
        // Entity destruction is handled by the engine's domain destruction queue
    }
} // namespace v

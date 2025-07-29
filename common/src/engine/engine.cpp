//
// Created by niooi on 7/28/2025.
//

#include <engine/engine.h>
#include <prelude.h>

namespace v {
    Engine::Engine() : registry_{}, engine_entity_{ registry_.create() }
    {
        LOG_INFO("Initialized the engine.");
    }
} // namespace v

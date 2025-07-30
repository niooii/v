//
// Created by niooi on 7/28/2025.
//

#include <engine/engine.h>
#include <prelude.h>

namespace v {
    Engine::Engine() : engine_entity_{ registry_.create() }
    {
        LOG_INFO("Initialized the engine.");
    }

    void Engine::tick()
    {
        prev_tick_span_ = tick_time_stopwatch_.reset();

        // if this was the first frame, the deltatime value would probably be kind of
        // huge and not useful, so set dt to 0.
        if (UNLIKELY(current_tick_ == 0))
            prev_tick_span_ = 0;

        current_tick_++;
    }

} // namespace v

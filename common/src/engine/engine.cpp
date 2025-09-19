//
// Created by niooi on 7/28/2025.
//

#include <engine/engine.h>
#include <engine/sink.h>
#include <prelude.h>

namespace v {
    Engine::Engine() :
        ctx_entity_{ ctx_registry_.create() }, engine_entity_{ registry_.create() }
    {
        LOG_INFO("Initialized the engine.");
    }

    Engine::~Engine()
    {
        LOG_INFO("Engine shutting down..");

        on_destroy.execute();
    }

    void Engine::tick()
    {
        prev_tick_span_ = tick_time_stopwatch_.reset();

        // if this was the first frame, the deltatime value would probably be kind of
        // huge and not useful, so set dt to 0.
        if (UNLIKELY(current_tick_ == 0))
            prev_tick_span_ = 0;

        current_tick_++;

        // process domain destruction queue
        {
            entt::entity id;
            while (domain_destroy_queue_.try_dequeue(id))
            {
                if (registry_.valid(id))
                {
                    registry_.destroy(id);
                }
            }
        }

        // run tick callbacks with dependency management
        on_tick.execute();

        // LOG_TRACE("Finished tick {} ", current_tick_);

        // flush default logger
        spd::default_logger()->flush();
    }

} // namespace v

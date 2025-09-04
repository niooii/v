//
// Created by niooi on 7/28/2025.
//

#include <engine/engine.h>
#include <engine/sink.h>
#include <prelude.h>

namespace v {
    Engine::Engine() :
        ctx_entity_{ ctx_registry_.create() },
        engine_entity_{ registry_.write()->create() }
    {
        LOG_INFO("Initialized the engine.");
    }

    Engine::~Engine()
    {
        LOG_INFO("Engine shutting down..");

        on_destroy.write()->execute();
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
            auto reg = registry_write();

            entt::entity id;
            while (domain_destroy_queue_.try_dequeue(id))
            {
                if (reg->valid(id))
                {
                    auto& owner_component = reg->get<std::unique_ptr<Domain>>(id);
                    owner_component.reset();
                    reg->destroy(id);
                }
            }
        }

        {
            // run tick callbacks with dependency management
            auto t_callbacks = on_tick.write();
            t_callbacks->execute();
        }

        LOG_TRACE("Finished tick {} ", current_tick_);

        // flush default logger
        spd::default_logger()->flush();
    }

    ReadGuard<entt::registry> Engine::registry_read() const
    {
        return registry_.read();
    }

    WriteGuard<entt::registry> Engine::registry_write()
    {
        return registry_.write();
    }
} // namespace v

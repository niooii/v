//
// Created by niooi on 7/28/2025.
//

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <entt/entt.hpp>
#include <time/stopwatch.h>

namespace v {
    class Engine {
    public:
        // Engine is non-copyable
        Engine(const Engine&)            = delete;
        Engine& operator=(const Engine&) = delete;

        Engine();
        ~Engine() = default;

        /// Updates the deltatime value. Should be called first in a main loop
        void tick();

        /// Returns the deltaTime (time between previous tick start and current tick
        /// start) in seconds. This will return 0 on the first frame
        FORCEINLINE f64 delta_time() const { return prev_tick_span_; };

        /// Adds a context to the engine, retrievable by type
        template <std::derived_from<Context> T, typename... Args>
        T& add_context(Args&&... args)
        {
            // If the component already exists, remove it
            if (registry_.all_of<T>(engine_entity_))
            {
                LOG_WARN("Adding duplicate context, removing old instance..");
                registry_.remove<T>(engine_entity_);
            }

            return registry_.emplace<T>(engine_entity_, std::forward<Args>(args)...);
        }

        /// Retrieve a context from the engine by type
        template <std::derived_from<Context> T>
        FORCEINLINE T* get_context()
        {
            return registry_.try_get<T>(engine_entity_);
        }

        /// Fetch the domain registry (ecs entity registry)
        FORCEINLINE entt::registry& domain_registry() { return registry_; }

    private:
        entt::registry registry_{};
        /// The engine itself is an entity. This allows us to have
        /// 'contexts' (essentially singleton components) that we can fetch
        /// by type
        entt::entity engine_entity_;

        Stopwatch tick_time_stopwatch_{};
        /// How long it took between the previous tick's start and the current tick's
        /// start. In other words, the 'deltaTime' variable
        f64 prev_tick_span_{ 0 };

        u64 current_tick_{ 0 };
    };
} // namespace v

//
// Created by niooi on 8/5/2025.
//

#pragma once

#include <domain/context.h>
#include <functional>

#include "entt/entt.hpp"

namespace v {
    /// A component of callbacks for global events
    struct SDLComponent {
        SDLComponent() = default;
        /// Called when SDL_Quit has been fired (e.g. sigkill)
        std::function<void()> on_quit;
    };

    /// Context for managing SDL3 Events subsystem and global event handling
    class SDLContext : public Context {
    public:
        explicit SDLContext(Engine& engine);
        ~SDLContext();

        /// Updates SDL events, pumps events, and routes window events to WindowContext
        /// Should be called before WindowContext::update() in application loop
        void update() const;

        /// Create an SDLComponent that will be attached to the given entity.
        /// Must not have any locks held to the registry, as this acquires a write lock.
        SDLComponent& create_component(entt::entity id) const;
    };
} // namespace v

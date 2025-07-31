//
// Created by niooi on 7/30/2025.
//

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <domain/domain.h>
#include <contexts/window.h>

namespace v {
    typedef void(RenderComponentFnRender)(
        Engine*, class RenderContext*, Window*);

    typedef void(RenderComponentFnResize)(
        Engine*, RenderContext*, Window*);

    struct RenderComponent {
        std::function<RenderComponentFnRender> pre_render{};
        std::function<RenderComponentFnRender> render{};
        std::function<RenderComponentFnRender> display{};
        std::function<RenderComponentFnRender> resize{};
    };

    class RenderContext : public Context {
    public:
        explicit RenderContext(Engine& engine);

        /// Creates and attaches a RenderComponent to a given entity, typically
        /// used within a Domain
        /// @return A reference to the newly created component for modification.
        /// The reference must not be stored, and is safe to use as long as only
        /// one thread is creating a RenderComponent at a time.
        RenderComponent& create_component(entt::entity entity_id);
    };
} // namespace v

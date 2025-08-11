//
// Created by niooi on 7/30/2025.
//

#include <contexts/render.h>
#include "init_vk.h"
#include <stdexcept>
#include "engine/engine.h"

namespace v {
    RenderContext::RenderContext(Engine& engine) : Context(engine) {
        // Create Daxa resources 
        daxa_resources_ = std::make_unique<DaxaResources>(engine_);

        // Init rendering stuff for the window
        auto window_ctx = engine_.write_context<WindowContext>();
        if (!window_ctx) {
            throw std::runtime_error("Create WindowContext before creating RenderContext");
        }

        Window* win = (*window_ctx)->get_window();
        window_resources_ = std::make_unique<WindowRenderResources>(win, daxa_resources_.get());
    }

    RenderContext::~RenderContext() = default;

    RenderComponent& RenderContext::create_component(entt::entity id) {
        // TODO: Implement component creation
        throw std::runtime_error("create_component not yet implemented");
    }

    void RenderContext::update() {
        // TODO! record command buffer
        pre_render.execute();
        LOG_TRACE("rendering");
    }
} // namespace v

//
// Created by niooi on 7/30/2025.
//

#include <render/ctx.h>
#include <stdexcept>
#include "engine/engine.h"
#include "init_vk.h"

namespace v {
    RenderContext::RenderContext(Engine& engine) : Context(engine)
    {
        // Create Daxa resources
        daxa_resources_ = std::make_unique<DaxaResources>(engine_);

        // Init rendering stuff for the window
        auto window_ctx = engine_.get_ctx<WindowContext>();
        if (!window_ctx)
        {
            throw std::runtime_error(
                "Create WindowContext before creating RenderContext");
        }

        Window* win = window_ctx->get_window();
        window_resources_ =
            std::make_unique<WindowRenderResources>(win, daxa_resources_.get());
    }

    RenderContext::~RenderContext() = default;


    RenderComponent& RenderContext::create_component(entt::entity id)
    {
        return engine_.add_component<RenderComponent>(id);
    }

    void RenderContext::update()
    {
        // TODO! record command buffer
        pre_render.execute();
        window_resources_->render();
        LOG_TRACE("rendered");
        daxa_resources_->device.collect_garbage();
    }
} // namespace v

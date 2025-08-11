//
// Created by niooi on 7/30/2025.
//

#pragma once

#include <contexts/window.h>
#include <defs.h>
#include <domain/context.h>
#include <domain/domain.h>
#include <vector>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_graph.hpp>
#include "engine/sink.h"

namespace v {
    typedef void(RenderComponentFnRender)(Engine*, class RenderContext*, Window*);

    typedef void(RenderComponentFnResize)(Engine*, RenderContext*, Window*);

    struct RenderComponent {
        std::function<RenderComponentFnRender> pre_render{};
        std::function<RenderComponentFnRender> render{};
        std::function<RenderComponentFnRender> post_render{};
        std::function<RenderComponentFnRender> resize{};
    };

    struct DaxaResources;

    struct WindowRenderResources {
        WindowRenderResources(const WindowRenderResources& other) = delete;

        WindowRenderResources& operator=(const WindowRenderResources& other) = delete;

        WindowRenderResources(Window* window, DaxaResources* daxa_resources);
        ~WindowRenderResources();

        daxa::Swapchain swapchain;
        daxa::TaskGraph render_graph;
        daxa::TaskImage task_swapchain_image;
        static constexpr u32 FRAMES_IN_FLIGHT = 2;

        DaxaResources* daxa_resources_;
    };

    class RenderContext : public Context {
        friend struct WindowRenderResources;

    public:
        explicit RenderContext(Engine& engine);
        ~RenderContext();

        /// Tasks that should run before the rendering of a frame
        DependentSink pre_render;

        /// Creates and attaches a RenderComponent to an entity, usually a Domain.
        /// @return A reference to the newly created component for modification.
        /// The reference must not be stored, and is safe to use as long as only
        /// one thread is creating a RenderComponent at a time.
        RenderComponent& create_component(entt::entity id);

        void update();

    private:
        std::unique_ptr<DaxaResources> daxa_resources_;

        /// Daxa resources for the for now singleton window
        std::unique_ptr<WindowRenderResources> window_resources_;
    };
} // namespace v

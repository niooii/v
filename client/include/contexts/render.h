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

        void render();
        // called to resize the swapchain
        void resize();

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

        /// Creates an empty RenderComponent
        FORCEINLINE RenderComponent create_component() { return {}; };
        
        /// Attaches a RenderComponent to an entity, usually a Domain
        void attach_component(const RenderComponent& component, entt::entity id);

        void update();

    private:
        std::unique_ptr<DaxaResources> daxa_resources_;

        /// Daxa resources for the for now singleton window
        std::unique_ptr<WindowRenderResources> window_resources_;
    };
} // namespace v

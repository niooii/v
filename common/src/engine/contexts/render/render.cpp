//
// Created by niooi on 7/30/2025.
//

#include <algorithm>
#include <engine/contexts/render/ctx.h>
#include <engine/contexts/render/render_domain.h>
#include <stdexcept>
#include "engine/contexts/render/init_vk.h"
#include "engine/engine.h"

namespace v {
    RenderContext::RenderContext(Engine& engine, const std::string& shader_root_path) :
        Context(engine), shader_root_path_(shader_root_path)
    {
        // Create Daxa resources
        daxa_resources_ = std::make_unique<DaxaResources>(engine_, shader_root_path_);

        // Init rendering stuff for the window
        auto window_ctx = engine_.get_ctx<WindowContext>();
        if (!window_ctx)
        {
            throw std::runtime_error(
                "Create WindowContext before creating RenderContext");
        }

        Window* win = window_ctx->get_window();
        window_resources_ =
            std::make_unique<WindowRenderResources>(win, daxa_resources_.get(), this);
    }

    RenderContext::~RenderContext() = default;


    RenderComponent& RenderContext::create_component(entt::entity id)
    {
        return engine_.add_component<RenderComponent>(id);
    }

    void RenderContext::register_render_domain(RenderDomainBase* domain)
    {
        render_domains_.push_back(domain);
        domain_version_++;
        graph_dirty_ = true;
    }

    void RenderContext::unregister_render_domain(RenderDomainBase* domain)
    {
        std::erase(render_domains_, domain);
        domain_version_++;
        graph_dirty_ = true;
    }

    void RenderContext::rebuild_graph()
    {
        LOG_TRACE("Rebuilding render graph (version {})", domain_version_);

        // Create new task graph
        window_resources_->render_graph = daxa::TaskGraph(
            {
                .device                   = daxa_resources_->device,
                .swapchain                = window_resources_->swapchain,
                .record_debug_information = true,
                .name                     = "main loop graph",
            });

        // Re-register persistent resources
        window_resources_->render_graph.use_persistent_image(
            window_resources_->task_swapchain_image);

        // Add tasks from all render domains
        for (auto* domain : render_domains_)
        {
            domain->add_render_tasks(window_resources_->render_graph);
        }

        // Finalize graph
        window_resources_->render_graph.submit({});
        window_resources_->render_graph.present({});
        window_resources_->render_graph.complete({});

        // Print debug info after graph rebuild
        auto dbg = window_resources_->render_graph.get_debug_string();
        LOG_INFO("TaskGraph debug (post-rebuild):\n{}", dbg);
    }

    void RenderContext::update()
    {
        // Rebuild graph if domains changed or manually marked dirty
        if (graph_dirty_ || domain_version_ != last_domain_version_)
        {
            rebuild_graph();
            graph_dirty_         = false;
            last_domain_version_ = domain_version_;
        }

        pre_render.execute();

        // execute the taskgraphs
        window_resources_->render();

        daxa_resources_->device.collect_garbage();
    }
} // namespace v

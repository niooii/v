//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <daxa/utils/task_graph.hpp>
#include <defs.h>
#include <engine/domain.h>

namespace v {
    class RenderContext;

    /// Base class for domains that integrate with the Daxa task graph.
    ///
    /// RenderDomains are singleton domains that can add GPU tasks to the render graph.
    /// They own GPU resources (buffers, images) and define rendering operations.
    ///
    /// IMPORTANT - Task Graph Rebuild Behavior:
    /// The task graph is ONLY rebuilt when render domains are added or removed.
    /// For conditional rendering logic (e.g. "only render if feature X exists"),
    /// use runtime checks WITHIN the .task lambda itself - do NOT conditionally
    /// call add_render_tasks(). This allows dynamic behavior without triggering
    /// expensive graph rebuilds.
    ///
    /// Example:
    ///   class ParticleRenderDomain : public RenderDomain<ParticleRenderDomain> {
    ///   public:
    ///       ParticleRenderDomain(Engine& engine) : RenderDomain(engine) {
    ///           particle_buffer_ = create_buffer(...);
    ///       }
    ///
    ///       void add_render_tasks(daxa::TaskGraph& graph) override {
    ///           // Query other domains at GRAPH CONSTRUCTION time for dependencies
    ///           auto* terrain = engine_.try_get_domain<TerrainRenderDomain>();
    ///
    ///           graph.add_task({
    ///               .attachments = {
    ///                   terrain ? terrain->depth_image_ : fallback_depth_,
    ///                   particle_buffer_,
    ///               },
    ///               .task = [](daxa::TaskInterface ti) {
    ///                   // Runtime conditionals are fine here for CPU logic
    ///                   // but resource dependencies must be declared above
    ///                   ti.recorder.dispatch(...);
    ///               }
    ///           });
    ///       }
    ///
    ///   private:
    ///       daxa::TaskBuffer particle_buffer_;
    ///   };
    ///
    /// Use RenderComponent for CPU-side pre/post-render hooks:
    ///   auto& rc = render_ctx->create_component(entity());
    ///   rc.pre_render = [this](...) { /* upload uniforms */ };
    ///   rc.post_render = [this](...) { /* readback data */ };
    class RenderDomainBase {
        friend class RenderContext;

    public:
        RenderDomainBase(Engine& engine);
        virtual ~RenderDomainBase();

        /// Add this domain's render tasks to the global task graph.
        /// Called during graph rebuild (on domain add/remove, or manual
        /// mark_graph_dirty).
        /// @param graph The global render task graph to add tasks to
        virtual void add_render_tasks(daxa::TaskGraph& graph) = 0;

        /// Mark the task graph as dirty, forcing a rebuild on the next frame.
        /// Call this when resources are resized, reallocated, or dependencies change
        /// in ways that affect the graph structure (not just data in buffers).
        void mark_graph_dirty();

    protected:
        RenderContext* render_ctx_ = nullptr;
    };

    template <typename Derived>
    class RenderDomain : public SDomain<Derived>, public RenderDomainBase {
    public:
        RenderDomain(
            Engine&            engine,
            const std::string& name = std::string{ type_name<Derived>() }) :
            SDomain<Derived>(engine, name), RenderDomainBase(engine)
        {}

        virtual ~RenderDomain() = default;
    };
} // namespace v

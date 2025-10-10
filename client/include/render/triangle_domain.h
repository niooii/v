//
// Created by niooi on 10/10/2025.
//

#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_graph.hpp>
#include <engine/contexts/render/render_domain.h>

namespace v {
    /// Simple rainbow triangle render domain for testing the RenderDomain API
    class TriangleRenderDomain : public RenderDomain<TriangleRenderDomain> {
    public:
        explicit TriangleRenderDomain(Engine& engine);
        ~TriangleRenderDomain() override = default;

        void add_render_tasks(daxa::TaskGraph& graph) override;

    private:
        std::shared_ptr<daxa::RasterPipeline> pipeline_;
    };
} // namespace v

//
// Created by niooi on 10/15/2025.
//

#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_graph.hpp>
#include <engine/contexts/render/render_domain.h>
#include <prelude.h>
#include "time/stopwatch.h"

namespace v {
    class Camera;
    class Window;

    class WorldAxesRenderer : public RenderDomain<WorldAxesRenderer> {
    public:
        explicit WorldAxesRenderer(Engine& engine);
        ~WorldAxesRenderer() override;

        void add_render_tasks(daxa::TaskGraph& graph) override;

    private:
        std::shared_ptr<daxa::RasterPipeline> raster_pipe_;

        Stopwatch sw_{};
    };
} // namespace v

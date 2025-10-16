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

    class MandelbulbRenderer : public RenderDomain<MandelbulbRenderer> {
    public:
        explicit MandelbulbRenderer(Engine& engine);
        ~MandelbulbRenderer() override;

        void add_render_tasks(daxa::TaskGraph& graph) override;

    private:
        std::shared_ptr<daxa::ComputePipeline> compute_pipeline_;

        daxa::ImageId   render_image_{};
        daxa::TaskImage task_render_image_{};

        daxa::Extent2D last_extent_{};

        Camera*   camera_ = nullptr;
        Window*   window_ = nullptr;
        Stopwatch sw_{};
    };
} // namespace v

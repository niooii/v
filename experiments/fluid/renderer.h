//
// Created by niooi on 11/7/2025.
//

#pragma once

#include <prelude.h>

namespace v {

    /// Renders a box around simulation bounds, and the particles as spheres (for now)
    /// Should be created after the simulation domain is created
    class SimulationRenderer : RenderDomain<SimulationRenderer> {
        void add_render_tasks(daxa::TaskGraph& graph) override {}
    };
} // namespace v

//
// Created by niooi on 11/7/2025.
//

#pragma once

#include "particles.h"
#include <prelude.h>

namespace v {
    struct GridCell {
        glm::vec2 vel;      // velocity
        f32       mass;     // mass
    };

    struct SimParams {
        f32 dt           = 1e-4f;        // timestep
        f32 gravity      = -9.8f;        // gravity in y direction
        u32 grid_res     = 64;           // grid resolution (square grid)
        f32 grid_spacing = 1.0f;         // spacing between grid cells
        f32 E            = 1e3f;         // Young's modulus
        f32 nu           = 0.2f;         // Poisson's ratio
        f32 bounds       = 1.0f;         // simulation bounds [0, bounds]
    };

    /// MLS-MPM simulation domain that operates on ParticleDomain data
    class FluidSimulation : Domain<FluidSimulation> {
    public:
        FluidSimulation() = default;
        ~FluidSimulation() override = default;

        void init() override;
        void step();

    private:
        void reset_grid();
        void p2g();  // particle to grid
        void update_grid();
        void g2p();  // grid to particle

        ParticleDomain*       particle_domain_ = nullptr;
        std::vector<GridCell> grid_;
        SimParams             params_;
    };
} // namespace v

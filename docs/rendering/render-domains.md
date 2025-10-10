# Render Domains

Render Domains provide a clean, modular API for integrating Daxa GPU tasks with the engine's ECS architecture. Each RenderDomain is a singleton that owns GPU resources and defines rendering operations that automatically integrate with the global task graph.

## Overview

The RenderDomain system is designed to solve the challenge of organizing GPU rendering work in a modular, extensible way. Instead of having a monolithic renderer, each rendering concern (terrain, particles, UI, etc.) gets its own domain that:

- Owns its GPU resources (pipelines, buffers, images)
- Defines rendering tasks that integrate with the global task graph
- Can query other domains for cross-domain dependencies
- Automatically handles resource synchronization and barriers

## Core Concepts

### 1. RenderDomain Base Class

RenderDomain extends `SDomain<Derived>` (singleton pattern) and automatically registers with the RenderContext.

```cpp
class MyRenderDomain : public v::RenderDomain<MyRenderDomain> {
public:
    MyRenderDomain(v::Engine& engine);
    ~MyRenderDomain() override = default;

    void add_render_tasks(daxa::TaskGraph& graph) override;

    // GPU resources owned by this domain
    daxa::TaskBuffer vertex_buffer_;
    daxa::TaskImage  output_image_;
};
```

**Key Characteristics**:
- **Singleton**: Only one instance per domain type
- **Auto-Registration**: Registers with RenderContext on construction
- **RAII Lifecycle**: Resources cleaned up automatically on destruction
- **Task Graph Integration**: Automatically contributes to global rendering pipeline

### 2. Task Graph Rebuild Strategy

The task graph is **only rebuilt** when necessary, which is important for performance:

- **Automatic Triggers**: Domain add/remove, window resize
- **Manual Trigger**: `render_ctx_->mark_graph_dirty()`
- **Runtime Logic**: Use runtime checks in task lambdas for conditional rendering

```cpp
// GOOD: Runtime conditional logic in task lambda
graph.add_task(daxa::Task::Raster("my_render")
    .color_attachment.writes(swapchain_image)
    .executes([this](daxa::TaskInterface ti) {
        if (debug_mode_enabled_) {  // Runtime check - OK
            // Do debug rendering
        }
    })
);

// BAD: Conditional task graph construction
void add_render_tasks(daxa::TaskGraph& graph) override {
    if (debug_mode_enabled_) {  // This breaks the graph!
        graph.add_task(...);  // Don't do this
    }
}
```

### 3. Resource Ownership

Resources are stored directly as domain members - no nested structs needed:

```cpp
class TerrainRenderDomain : public v::RenderDomain<TerrainRenderDomain> {
public:
    TerrainRenderDomain(v::Engine& engine) : RenderDomain(engine, "Terrain") {
        auto& daxa = render_ctx_->daxa_resources();

        // Create resources directly as members
        heightmap_ = daxa::TaskImage({
            .initial_images = {
                daxa.device.create_image({
                    .format = daxa::Format::R32_SFLOAT,
                    .size = {1024, 1024, 1},
                    .usage = daxa::ImageUsageFlagBits::SHADER_SAMPLED |
                             daxa::ImageUsageFlagBits::TRANSFER_DST,
                    .name = "terrain_heightmap",
                })
            },
            .name = "heightmap_task",
        });

        vertex_buffer_ = daxa::TaskBuffer({
            .initial_buffers = {
                daxa.device.create_buffer({
                    .size = sizeof(TerrainVertex) * max_vertices_,
                    .name = "terrain_vertices",
                })
            },
            .name = "terrain_vertices_task",
        });
    }

    // Tasks can access these resources directly
    void add_render_tasks(daxa::TaskGraph& graph) override {
        graph.add_task(daxa::Task::Raster("terrain")
            .uses(vertex_buffer_, daxa::TaskBufferAccess::VERTEX_SHADER_READ)
            .uses(heightmap_, daxa::TaskImageAccess::FRAGMENT_SHADER_SAMPLED)
            .color_attachment.writes(swapchain_image)
            .executes([this](daxa::TaskInterface ti) {
                // Use vertex_buffer_ and heightmap_ directly
            })
        );
    }

private:
    daxa::TaskImage heightmap_;
    daxa::TaskBuffer vertex_buffer_;
    size_t max_vertices_ = 1000000;
};
```

## Basic Usage Example

Here's a complete example based on the TriangleRenderDomain in the codebase:

### Header File
```cpp
// triangle_render_domain.h
#pragma once

#include <engine/contexts/render/render_domain.h>
#include <daxa/daxa.hpp>

namespace game {
    class TriangleRenderDomain : public v::RenderDomain<TriangleRenderDomain> {
    public:
        TriangleRenderDomain(v::Engine& engine);
        ~TriangleRenderDomain() override = default;

        void add_render_tasks(daxa::TaskGraph& graph) override;

    private:
        std::shared_ptr<daxa::RasterPipeline> pipeline_;
    };
}
```

### Implementation
```cpp
// triangle_render_domain.cpp
#include "triangle_render_domain.h"
#include <engine/contexts/render/ctx.h>

namespace game {
    TriangleRenderDomain::TriangleRenderDomain(v::Engine& engine)
        : RenderDomain(engine, "Triangle")
    {
        // Get Daxa resources from RenderContext
        auto& daxa = render_ctx_->daxa_resources();

        // Create the pipeline
        pipeline_ = daxa.pipeline_manager.add_raster_pipeline2({
            .vertex_shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"triangle.glsl"},
                .defines = {{"VERTEX_SHADER", "1"}},
            },
            .fragment_shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"triangle.glsl"},
                .defines = {{"FRAGMENT_SHADER", "1"}},
            },
            .color_attachments = {{
                .format = daxa::Format::R8G8B8A8_UNORM,
                .blend = {
                    .enable = true,
                    .src_factor = daxa::BlendFactor::SRC_ALPHA,
                    .dst_factor = daxa::BlendFactor::ONE_MINUS_SRC_ALPHA,
                },
            }},
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE,
                .front_face = daxa::FrontFace::COUNTER_CLOCKWISE,
            },
            .name = "triangle_pipeline",
        }).value();
    }

    void TriangleRenderDomain::add_render_tasks(daxa::TaskGraph& graph) {
        // Get swapchain image from RenderContext
        auto& swapchain_image = render_ctx_->swapchain_image();

        graph.add_task(
            daxa::Task::Raster("triangle_render")
                // IMPORTANT: Specify view type for swapchain images
                .color_attachment.writes(daxa::ImageViewType::REGULAR_2D, swapchain_image)
                .executes([this](daxa::TaskInterface ti) {
                    // Get runtime swapchain image info
                    auto& swapchain_image = render_ctx_->swapchain_image();
                    auto image_id = ti.get(swapchain_image).ids[0];
                    auto image_view = ti.get(swapchain_image).view_ids[0];
                    auto image_info = ti.device.image_info(image_id).value();

                    // Begin render pass
                    auto render_recorder = std::move(ti.recorder).begin_renderpass({
                        .color_attachments = {{
                            .image_view = image_view,
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = std::array<f32, 4>{0.1f, 0.1f, 0.1f, 1.0f},
                        }},
                        .render_area = {
                            .x = 0, .y = 0,
                            .width = image_info.size.x,
                            .height = image_info.size.y
                        },
                    });

                    // Set pipeline and draw
                    render_recorder.set_pipeline(*pipeline_);
                    render_recorder.draw({.vertex_count = 3});

                    // End render pass
                    ti.recorder = std::move(render_recorder).end_renderpass();
                })
        );
    }
}
```

### Registration
```cpp
// In client initialization
auto* render_ctx = engine.add_ctx<v::RenderContext>("./resources/shaders");

// Add render domain - automatically registers with RenderContext
engine.add_domain<game::TriangleRenderDomain>();

// Graph will be built automatically on first frame
```

## Cross-Domain Dependencies

RenderDomains can query other domains to create complex rendering pipelines:

### Querying Other Domains

```cpp
void CompositeRenderDomain::add_render_tasks(daxa::TaskGraph& graph) {
    // Query domains at GRAPH CONSTRUCTION time
    auto* terrain = engine_.try_get_domain<TerrainRenderDomain>();
    auto* particles = engine_.try_get_domain<ParticleRenderDomain>();
    auto* ui = engine_.try_get_domain<UIRenderDomain>();

    // Build task with conditional dependencies
    auto composite_task = daxa::Task::Raster("final_composite")
        .color_attachment.writes(daxa::ImageViewType::REGULAR_2D,
                                 render_ctx_->swapchain_image());

    // Add resource dependencies for domains that exist
    if (terrain) {
        composite_task.uses(terrain->output_image_,
                           daxa::TaskImageAccess::FRAGMENT_SHADER_SAMPLED);
    }
    if (particles) {
        composite_task.uses(particles->output_image_,
                           daxa::TaskImageAccess::FRAGMENT_SHADER_SAMPLED);
    }
    if (ui) {
        composite_task.uses(ui->output_image_,
                           daxa::TaskImageAccess::FRAGMENT_SHADER_SAMPLED);
    }

    graph.add_task(composite_task.executes([this, terrain, particles, ui]
                                          (daxa::TaskInterface ti) {
        // Runtime logic - can check domains again if needed
        bool show_terrain = terrain != nullptr;
        bool show_particles = particles != nullptr;
        bool show_ui = ui != nullptr;

        // Composite available layers
        render_composite(ti, show_terrain, show_particles, show_ui);
    }));
}
```

### Resource Sharing Between Domains

```cpp
class WaterRenderDomain : public v::RenderDomain<WaterRenderDomain> {
public:
    void add_render_tasks(daxa::TaskGraph& graph) override {
        // Query terrain domain for height data
        auto* terrain = engine_.try_get_domain<TerrainRenderDomain>();

        if (terrain) {
            graph.add_task(daxa::Task::Compute("water_simulation")
                .reads(terrain->heightmap_, daxa::TaskImageAccess::SHADER_SAMPLED)
                .writes(water_height_buffer_, daxa::TaskBufferAccess::SHADER_WRITE)
                .executes([this](daxa::TaskInterface ti) {
                    // Water simulation using terrain height data
                })
            );

            graph.add_task(daxa::Task::Raster("water_render")
                .reads(water_height_buffer_, daxa::TaskBufferAccess::VERTEX_SHADER_READ)
                .reads(terrain->heightmap_, daxa::TaskImageAccess::FRAGMENT_SHADER_SAMPLED)
                .color_attachment.writes(render_ctx_->swapchain_image())
                .executes([this](daxa::TaskInterface ti) {
                    // Render water with terrain interaction
                })
            );
        }
    }

private:
    daxa::TaskBuffer water_height_buffer_;
};
```

## Pipeline Creation

### Raster Pipelines

```cpp
class MyRenderDomain : public v::RenderDomain<MyRenderDomain> {
public:
    MyRenderDomain(v::Engine& engine) : RenderDomain(engine, "MyRender") {
        auto& daxa = render_ctx_->daxa_resources();

        pipeline_ = daxa.pipeline_manager.add_raster_pipeline2({
            .vertex_shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"my_shader.glsl"},
                .defines = {{"VERTEX_SHADER", "1"}},
            },
            .fragment_shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"my_shader.glsl"},
                .defines = {{"FRAGMENT_SHADER", "1"}},
            },
            .color_attachments = {
                {.format = daxa::Format::R8G8B8A8_UNORM},
                {.format = daxa::Format::R32_SFLOAT},  // G-buffer
            },
            .depth_attachment = {.format = daxa::Format::D32_SFLOAT},
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::BACK,
                .front_face = daxa::FrontFace::COUNTER_CLOCKWISE,
            },
            .depth_test = {
                .depth_test_enable = true,
                .depth_write_enable = true,
                .depth_compare_op = daxa::CompareOp::LESS_OR_EQUAL,
            },
            .name = "my_pipeline",
        }).value();
    }

private:
    std::shared_ptr<daxa::RasterPipeline> pipeline_;
};
```

### Compute Pipelines

```cpp
class ComputeDomain : public v::RenderDomain<ComputeDomain> {
public:
    ComputeDomain(v::Engine& engine) : RenderDomain(engine, "Compute") {
        auto& daxa = render_ctx_->daxa_resources();

        compute_pipeline_ = daxa.pipeline_manager.add_compute_pipeline2({
            .shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"compute.glsl"},
                .defines = {{"COMPUTE_SHADER", "1"}},
            },
            .name = "compute_pipeline",
        }).value();
    }

    void add_render_tasks(daxa::TaskGraph& graph) override {
        graph.add_task(daxa::Task::Compute("compute_pass")
            .reads(input_buffer_, daxa::TaskBufferAccess::SHADER_READ)
            .writes(output_buffer_, daxa::TaskBufferAccess::SHADER_WRITE)
            .executes([this](daxa::TaskInterface ti) {
                auto pipeline = ti.get_pipeline(compute_pipeline_);
                ti.recorder.bind_pipeline(pipeline);

                // Dispatch compute work
                ti.recorder.dispatch({
                    .x = (work_size_ + 63) / 64,  // Workgroup size is 64
                    .y = 1,
                    .z = 1,
                });
            })
        );
    }

private:
    std::shared_ptr<daxa::ComputePipeline> compute_pipeline_;
    size_t work_size_ = 1024;
};
```

## CPU-Side Integration

RenderDomains can also use CPU-side hooks for data uploads and readbacks:

```cpp
class MyRenderDomain : public v::RenderDomain<MyRenderDomain> {
public:
    MyRenderDomain(v::Engine& engine) : RenderDomain(engine, "MyRender") {
        // Setup CPU-side hooks for this domain's entity
        auto& render_component = render_ctx_->create_component(entity());

        // Pre-render hook: Upload data to GPU
        render_component.pre_render = [this](auto...) {
            upload_vertex_data();
            update_uniforms();
        };

        // Post-render hook: Readback data if needed
        render_component.post_render = [this](auto...) {
            if (needs_readback_) {
                read_gpu_data();
                needs_readback_ = false;
            }
        };
    }

private:
    void upload_vertex_data() {
        // Upload CPU data to GPU buffers
        auto& daxa = render_ctx_->daxa_resources();

        // Map buffer and upload data
        auto* buffer_ptr = daxa.device.map_buffer_as<Vertex>(vertex_buffer_.get_state().buffers[0]);
        std::memcpy(buffer_ptr, cpu_vertices_.data(), cpu_vertices_.size() * sizeof(Vertex));
        daxa.device.unmap_buffer(vertex_buffer_.get_state().buffers[0]);
    }

    void update_uniforms() {
        // Update uniform buffers
        UniformData uniforms{
            .mvp = view_projection_matrix_,
            .time = current_time_,
            .resolution = window_resolution_,
        };

        auto& daxa = render_ctx_->daxa_resources();
        auto* uniform_ptr = daxa.device.map_buffer_as<UniformData>(uniform_buffer_.get_state().buffers[0]);
        *uniform_ptr = uniforms;
        daxa.device.unmap_buffer(uniform_buffer_.get_state().buffers[0]);
    }

    std::vector<Vertex> cpu_vertices_;
    bool needs_readback_ = false;
    daxa::TaskBuffer vertex_buffer_;
    daxa::TaskBuffer uniform_buffer_;
};
```

## Engine Integration

### Adding RenderDomains

```cpp
void setup_rendering(v::Engine& engine) {
    // Create render context first
    auto* render_ctx = engine.add_ctx<v::RenderContext>("./resources/shaders");

    // Add render domains in desired order (affects dependency resolution)
    engine.add_domain<TerrainRenderDomain>();
    engine.add_domain<WaterRenderDomain>();
    engine.add_domain<ParticleRenderDomain>();
    engine.add_domain<UIRenderDomain>();

    // Setup task dependencies
    engine.on_tick.connect(
        {},                   // No dependencies before
        {"render"},           // Before render task
        "window_update",      // Task name
        [render_ctx]() {
            render_ctx->update();
        }
    );
}
```

### Destroying RenderDomains

```cpp
// Queue destruction (deferred to post-tick for thread safety)
engine.queue_destroy_domain(terrain_domain->entity());

// Task graph will automatically rebuild on next frame without this domain
```

## When to Mark Graph Dirty

Call `mark_graph_dirty()` when resource dependencies change:

```cpp
void MyRenderDomain::resize_output_image(u32vec2 new_size) {
    auto& daxa = render_ctx_->daxa_resources();

    // Destroy old image
    daxa.device.destroy_image(output_image_.get_state().images[0]);

    // Create new image with different size
    output_image_.set_images({
        daxa.device.create_image({
            .format = daxa::Format::R8G8B8A8_UNORM,
            .size = {new_size.x, new_size.y, 1},
            .usage = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT |
                     daxa::ImageUsageFlagBits::SHADER_SAMPLED,
            .name = "resized_output",
        })
    });

    // Force graph rebuild since resource dependencies changed
    mark_graph_dirty();
}
```

**DO NOT** call for:
- Updating buffer data (uniforms, vertex data)
- Camera movement or parameter changes
- Per-frame animations
- Runtime conditional logic

## Best Practices

1. **Always Add Tasks**: Don't conditionally call `add_render_tasks()` - always add tasks and use runtime logic for conditional execution

2. **Declare Resource Dependencies**: All resources must be declared in `.uses()`, `.reads()`, or `.writes()` even if used conditionally

3. **Query at Construction Time**: Query other domains when building the task graph, not in task lambdas (except for runtime logic)

4. **Direct Resource Storage**: Store resources directly as domain members, no nested structs needed

5. **Trust the Rebuild System**: The dirty tracking system is efficient - don't try to optimize it prematurely

6. **Use Proper View Types**: Always specify `daxa::ImageViewType::REGULAR_2D` for swapchain images

## Mod Support

External mods can create their own RenderDomains using the same API:

```cpp
// In mod.dll
class ModParticleSystem : public v::RenderDomain<ModParticleSystem> {
public:
    ModParticleSystem(v::Engine& engine) : RenderDomain(engine, "ModParticles") {
        // Create mod-specific GPU resources
        auto& daxa = render_ctx_->daxa_resources();

        particle_buffer_ = daxa::TaskBuffer({
            .initial_buffers = {
                daxa.device.create_buffer({
                    .size = sizeof(Particle) * max_particles,
                    .name = "mod_particles",
                })
            },
            .name = "mod_particles_task",
        });

        update_pipeline_ = daxa.pipeline_manager.add_compute_pipeline2({
            .shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"mod_particle_update.glsl"},
            },
            .name = "mod_particle_update",
        }).value();
    }

    void add_render_tasks(daxa::TaskGraph& graph) override {
        // Update particle simulation
        graph.add_task(daxa::Task::Compute("mod_particle_update")
            .reads(particle_buffer_, daxa::TaskBufferAccess::SHADER_READ_WRITE)
            .executes([this](daxa::TaskInterface ti) {
                auto pipeline = ti.get_pipeline(update_pipeline_);
                ti.recorder.bind_pipeline(pipeline);
                ti.recorder.dispatch({
                    .x = (particle_count_ + 255) / 256,  // Workgroup size 256
                    .y = 1,
                    .z = 1,
                });
            })
        );

        // Render particles
        graph.add_task(daxa::Task::Raster("mod_particle_render")
            .uses(particle_buffer_, daxa::TaskBufferAccess::VERTEX_SHADER_READ)
            .color_attachment.writes(render_ctx_->swapchain_image())
            .executes([this](daxa::TaskInterface ti) {
                // Render particles
            })
        );
    }

private:
    daxa::TaskBuffer particle_buffer_;
    std::shared_ptr<daxa::ComputePipeline> update_pipeline_;
    size_t max_particles_ = 10000;
    size_t particle_count_ = 0;
};

// Mod initialization
extern "C" void mod_init(v::Engine& engine) {
    engine.add_domain<ModParticleSystem>();
}
```

Core game code can optionally integrate with mod domains:

```cpp
void GameRenderDomain::add_render_tasks(daxa::TaskGraph& graph) {
    // Check if particle mod is loaded
    auto* mod_particles = engine_.try_get_domain<ModParticleSystem>();

    // Create composite task
    auto composite_task = daxa::Task::Raster("game_composite")
        .color_attachment.writes(render_ctx_->swapchain_image())
        .uses(game_output_, daxa::TaskImageAccess::FRAGMENT_SHADER_SAMPLED);

    if (mod_particles) {
        // Include mod particles in composite
        composite_task.uses(mod_particles->particle_output_,
                           daxa::TaskImageAccess::FRAGMENT_SHADER_SAMPLED);
    }

    graph.add_task(composite_task.executes([this, mod_particles]
                                          (daxa::TaskInterface ti) {
        render_game_composite(ti, mod_particles != nullptr);
    }));
}
```
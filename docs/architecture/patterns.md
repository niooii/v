# Architectural Patterns

This document describes the core architectural patterns used in V Engine. These patterns were designed to solve the challenges of creating a modular, extensible voxel engine that supports seamless C++/C# interop while maintaining high performance.

## Design Philosophy

V Engine follows a **hybrid ECS-OOP approach** that breaks "pure ECS rules" for practical usability. This design is inspired by the principles described in [Object-Oriented Entity-Component-System Design](https://johnlin.se/blog/2021/07/27/object-oriented-entity-component-system-design/), where systems can act as entities, components, and systems simultaneously.

The core philosophy is:
- **Practical over Pure**: Use patterns that work in practice, not just in theory
- **Recursive Architecture**: Components can contain systems, systems can act as components
- **LEGO-like Composition**: Systems should combine intuitively without hardcoded dependencies
- **Performance with Usability**: C++ for performance-critical code, easy extension for game logic

## Core Architecture

### ECS Foundation

The engine uses EnTT as its ECS backbone with two separate registries:

```cpp
// In Engine class
entt::registry registry_;        // Main ECS for domains/components
entt::registry ctx_registry_;    // Separate registry for singleton contexts
```

**QueryBy Trait System**: Types specify their storage representation via `QueryBy<T>`:

```cpp
// Domains store as unique_ptr
template<typename Derived>
using DomainStorage = QueryBy<std::unique_ptr<Derived>>;

// Users query by raw type, engine handles storage transformation
auto* domain = engine.get_domain<MyDomain>();  // Automatically finds unique_ptr storage
```

### Domain Pattern

**Core Concept**: Domains are modular systems that can act as entities, components, and systems.

```cpp
class MyDomain : public v::Domain<MyDomain> {
public:
    MyDomain(v::Engine& engine) : v::Domain(engine, "MyDomain") {
        // Initialize domain resources and systems
    }

    void init_standard_components() override {
        // Add components and setup systems
    }
};
```

**Key Characteristics**:
- **CRTP Base**: `Domain<Derived>` provides static polymorphism
- **Singleton Support**: `SDomain<Derived>` for single-instance domains
- **RAII Lifecycle**: Automatic registration/destruction with engine
- **Queryable**: Access via `engine.get_domain<T>()` or `engine.try_get_domain<T>()`

**Usage Example**:
```cpp
// Create and register domain
auto* my_domain = engine.add_domain<MyDomain>(config);

// Query domain from anywhere
auto* domain = engine.try_get_domain<MyDomain>();
if (domain) {
    domain->do_something();
}

// Deferred destruction
engine.queue_destroy_domain(my_domain->entity());
```

### Context Pattern

**Purpose**: Singleton-like services for core engine functionality (rendering, networking, windowing, etc.).

```cpp
class MyContext : public v::Context {
public:
    MyContext(v::Engine& engine) : v::Context(engine) {
        // Initialize context resources
    }

    void update() {
        // Update context each frame
    }
};
```

**Key Characteristics**:
- **Service Locator**: Singleton services accessible via `engine.get_ctx<T>()`
- **Automatic Replacement**: Adding duplicate context replaces existing one
- **Engine Integration**: Contexts automatically receive engine reference
- **Shared Services**: Used by multiple domains throughout the engine

**Usage Examples from the Codebase**:
```cpp
// Client setup - common pattern
auto sdl_ctx = engine.add_ctx<v::SDLContext>();
auto window_ctx = engine.add_ctx<v::WindowContext>();
auto render_ctx = engine.add_ctx<v::RenderContext>("./resources/shaders");
auto net_ctx = engine.add_ctx<v::NetworkContext>(1.0 / 500);
```

## Rendering Architecture

### Resource Hierarchy

The rendering system uses a layered resource hierarchy:

```
DaxaResources (global, application lifetime)
├── daxa::Instance
├── daxa::Device
└── daxa::PipelineManager (shader hot-reload support)

WindowRenderResources (per-window, window lifetime)
├── daxa::Swapchain (FRAMES_IN_FLIGHT = 2)
├── daxa::TaskGraph (render graph with auto-sync)
└── daxa::TaskImage (swapchain wrapper)
```

### RenderDomain Pattern

**Purpose**: Modular GPU rendering system where each domain manages its own GPU resources and tasks.

```cpp
class TriangleRenderer : public v::RenderDomain<TriangleRenderer> {
public:
    TriangleRenderer(v::Engine& engine)
        : v::RenderDomain(engine, "Triangle") {

        // Access Daxa resources from RenderContext
        auto& daxa = render_ctx_->daxa_resources();

        // Create pipeline
        pipeline_ = daxa.pipeline_manager.add_raster_pipeline2({
            .vertex_shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"triangle.glsl"},
                .defines = {{"VERTEX_SHADER", "1"}},
            },
            .fragment_shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"triangle.glsl"},
                .defines = {{"FRAGMENT_SHADER", "1"}},
            },
            .color_attachments = {{.format = daxa::Format::R8G8B8A8_UNORM}},
            .name = "triangle_pipeline",
        }).value();
    }

    void add_render_tasks(daxa::TaskGraph& graph) override {
        auto& swapchain_image = render_ctx_->swapchain_image();

        graph.add_task(
            daxa::Task::Raster("triangle_render")
                .color_attachment.writes(daxa::ImageViewType::REGULAR_2D, swapchain_image)
                .executes([this](daxa::TaskInterface ti) {
                    auto& swapchain_image = render_ctx_->swapchain_image();
                    auto image_id = ti.get(swapchain_image).ids[0];
                    auto image_view = ti.get(swapchain_image).view_ids[0];

                    auto render_recorder = std::move(ti.recorder).begin_renderpass({
                        .color_attachments = {{
                            .image_view = image_view,
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = std::array<f32, 4>{0.1f, 0.1f, 0.1f, 1.0f},
                        }},
                        .render_area = {.x = 0, .y = 0, .width = 1920, .height = 1080},
                    });

                    render_recorder.set_pipeline(*pipeline_);
                    render_recorder.draw({.vertex_count = 3});
                    ti.recorder = std::move(render_recorder).end_renderpass();
                })
        );
    }

private:
    std::shared_ptr<daxa::RasterPipeline> pipeline_;
};
```

**Key Characteristics**:
- **Automatic Registration**: Self-registers on construction, unregisters on destruction
- **Dirty Tracking**: Task graph only rebuilds when domains change
- **Resource Ownership**: Each domain owns its GPU resources as members
- **Cross-Domain Dependencies**: Can query other domains at graph construction time

**Graph Rebuild Triggers**:
1. Domain registration/unregistration (automatic)
2. Window resize (automatic)
3. Manual `render_ctx_->mark_graph_dirty()` call

## Resource Lifetime Patterns

### 1. Application Lifetime
```cpp
// DaxaResources - owned by RenderContext, cleaned up automatically
class RenderContext {
    std::unique_ptr<DaxaResources> daxa_resources_;
};
```

### 2. Window Lifetime
```cpp
// WindowRenderResources - per-window resources
class RenderContext {
    std::unordered_map<Window*, std::unique_ptr<WindowRenderResources>> window_resources_;
};
```

### 3. Domain/Entity Lifetime
```cpp
// Domains stored as unique_ptr on entities
engine.add_domain<MyDomain>();  // Creates entity + stores unique_ptr
engine.queue_destroy_domain(entity);  // Deferred cleanup
```

### 4. Per-Frame Resources
```cpp
// Double buffering with automatic garbage collection
static constexpr size_t FRAMES_IN_FLIGHT = 2;
device.collect_garbage();  // Called each frame
```

## ECS Query Patterns

### Standard Domain Queries
```cpp
// Get singleton domain
auto* server = engine.get_domain<ServerDomain>();

// Optional domain query
auto* terrain = engine.try_get_domain<TerrainRenderDomain>();
if (terrain) {
    terrain->update_terrain();
}

// View multiple domain types
auto view = engine.view<TerrainDomain, PhysicsDomain>();
for (auto [entity, terrain, physics] : view.each()) {
    // Process interactions
}
```

### Component Queries
```cpp
// Query components within a domain
auto view = engine.view<PositionComponent, VelocityComponent>();
for (auto [entity, pos, vel] : view.each()) {
    pos.position += vel.velocity * delta_time;
}
```

## Threading & Synchronization

### RwLock Pattern (Rust-inspired)
```cpp
v::RwLock<std::vector<PlayerData>> players;

// Read access (multiple readers allowed)
{
    auto read_guard = players.read();
    for (const auto& player : *read_guard) {
        // Read player data
    }
} // Guard released

// Write access (exclusive access)
{
    auto write_guard = players.write();
    write_guard->push_back(new_player);
} // Guard released
```

### Concurrent Queues
```cpp
// Lock-free cross-thread communication
moodycamel::ConcurrentQueue<NetworkEvent> net_events_;

// Producer thread
net_events_.enqueue(event);

// Consumer thread
NetworkEvent event;
while (net_events_.try_dequeue(event)) {
    // Process event
}
```

### IO Thread Pattern
```cpp
// Network operations on separate thread
class NetworkContext {
    std::thread io_thread_;
    moodycamel::ConcurrentQueue<NetworkEvent> events_;

    void io_thread_loop() {
        while (running_) {
            // Process network IO
            // Queue events for main thread
        }
    }
};
```

## Task Dependency Management

### CPU-side Dependencies
```cpp
// Task ordering with dependencies
engine.on_tick.connect(
    {"input"},           // After input task
    {"physics"},         // Before physics task
    "player_movement",   // Task name
    []() { /* update players */ }
);
```

### GPU-side Dependencies
```cpp
// Automatic barrier insertion via Daxa task graph
graph.add_task(daxa::Task::Compute("physics")
    .reads(vertex_buffer_)    // Auto-sync before reading
    .writes(physics_buffer_)  // Auto-sync after writing
    .executes([](daxa::TaskInterface ti) {
        // GPU work here
    })
);
```

## Serialization Pattern

### CBOR-based with reflect-cpp
```cpp
struct PlayerData {
    std::string name;
    glm::vec3 position;
    std::vector<Item> inventory;

    SERDE_SKIP(std::mutex lock);  // Skip non-serializable fields
    SERDE_IMPL(PlayerData);       // Auto-generate serialize/parse
};

// Usage
PlayerData player;
auto serialized = player.serialize();  // CBOR format
PlayerData loaded = PlayerData::parse(serialized);
```

## Extension Points (For Mods)

### Custom Domains
```cpp
class ModDomain : public v::Domain<ModDomain> {
public:
    ModDomain(v::Engine& engine) : v::Domain(engine, "MyMod") {
        // Mod initialization
    }
};

// Mod registration
extern "C" void mod_init(v::Engine& engine) {
    engine.add_domain<ModDomain>();
}
```

### Custom RenderDomains
```cpp
class ModRenderDomain : public v::RenderDomain<ModRenderDomain> {
public:
    ModRenderDomain(v::Engine& engine) : v::RenderDomain(engine, "ModRender") {
        // Create GPU resources
        auto& daxa = render_ctx_->daxa_resources();
        pipeline_ = daxa.pipeline_manager.add_compute_pipeline2({...}).value();
    }

    void add_render_tasks(daxa::TaskGraph& graph) override {
        // Add mod's rendering tasks
        graph.add_task(daxa::Task::Compute("mod_effect")
            .writes(mod_buffer_)
            .executes([this](daxa::TaskInterface ti) {
                // GPU work
            })
        );
    }
};
```

### Custom Contexts
```cpp
class ModContext : public v::Context {
public:
    ModContext(v::Engine& engine) : v::Context(engine) {}
    void update() override { /* Mod-specific updates */ }
};

// Usage
auto* mod_ctx = engine.add_ctx<ModContext>();
```

## Key Files Reference

### Core Engine
- `common/include/engine/engine.h` - Engine class, ECS APIs, domain/context management
- `common/include/engine/domain.h` - Domain base classes, CRTP pattern
- `common/include/engine/context.h` - Context base class
- `common/include/engine/traits.h` - QueryBy trait system

### Rendering
- `common/include/engine/contexts/render/ctx.h` - RenderContext and resource management
- `common/include/engine/contexts/render/render_domain.h` - RenderDomain base classes
- `common/src/engine/contexts/render/per_window_init.cpp` - Swapchain and task graph setup

### Threading
- `common/include/engine/sync.h` - RwLock pattern implementation
- `common/include/engine/sink.h` - Task dependency management

### Examples
- `client/src/render/triangle_domain.cpp` - Complete TriangleRenderer example
- `experiments/voxel-rendering/main.cpp` - Minimal application setup
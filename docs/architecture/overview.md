# Architecture Overview

V Engine is built with a modular, domain-centric architecture that combines the best of Entity-Component-System design with object-oriented patterns. This document provides a high-level overview of the engine's architecture and design philosophy.

## Design Philosophy

The architecture was created to solve three core challenges:

1. **System Modularity**: Components should combine like LEGO blocks without hardcoded dependencies
2. **C++/C# Interop**: Seamless integration between native and managed code
3. **Effortless Extensibility**: Easy to add new systems and support modding

The solution is a **hybrid ECS-OOP approach** that breaks "pure ECS rules" for practical usability, inspired by the principles described in [Object-Oriented Entity-Component-System Design](https://johnlin.se/blog/2021/07/27/object-oriented-entity-component-system-design/).

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Application Layer                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │    Client App   │  │   Server App    │  │   Experiments│ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────┐
│                         Engine Core                         │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                   Engine (ECS)                          │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │ │
│  │  │   Domain    │  │   Context   │  │    Component    │  │ │
│  │  │  Registry   │  │  Registry   │  │     Storage     │  │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────┐
│                      Foundation Layer                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │
│  │    SDL3     │  │    Daxa     │  │        EnTT             │ │
│  │ (Windowing) │  │ (Vulkan)    │  │       (ECS)             │ │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### Engine

The **Engine** is the central orchestrator that manages:
- ECS registries for domains and contexts
- Task scheduling and dependency management
- Resource lifecycle and cleanup
- Main game loop and timing

```cpp
class Engine {
public:
    // Domain management
    template<typename T, typename... Args>
    T* add_domain(Args&&... args);

    template<typename T>
    T* get_domain();

    template<typename T>
    T* try_get_domain();

    // Context management (singleton services)
    template<typename T, typename... Args>
    T* add_ctx(Args&&... args);

    template<typename T>
    T* get_ctx();

    // Task system
    void on_tick.connect(DependentDeps after, DependentDeps before,
                        std::string name, std::function<void()> task);

    // Main loop
    void tick();
};
```

### Domains

**Domains** are modular systems that can act as entities, components, and systems simultaneously. This recursive nature is what gives the architecture its flexibility.

```cpp
class MyGameSystem : public v::Domain<MyGameSystem> {
public:
    MyGameSystem(v::Engine& engine) : v::Domain(engine, "MyGame") {
        // Initialize system resources
    }

    void init_standard_components() override {
        // Setup components and behavior
    }

    // System can be queried like a singleton
    void do_game_logic() {
        // Game-specific logic here
    }
};
```

**Key Domain Types**:
- **Standard Domains**: Game logic, physics, AI, etc.
- **Render Domains**: GPU rendering tasks and resources
- **Network Domains**: Networking and multiplayer functionality
- **Singleton Domains**: Systems that should only have one instance

### Contexts

**Contexts** are singleton services that provide core engine functionality:

```cpp
// Core contexts that most applications will use:
auto* sdl_ctx = engine.add_ctx<v::SDLContext>();           // Platform abstraction
auto* window_ctx = engine.add_ctx<v::WindowContext>();     // Window management
auto* render_ctx = engine.add_ctx<v::RenderContext>();     // GPU rendering
auto* net_ctx = engine.add_ctx<v::NetworkContext>();       // Networking
auto* async_ctx = engine.add_ctx<v::AsyncContext>();       // Threading
```

Contexts are:
- **Singleton**: Only one instance per type
- **Service-like**: Provide functionality to other systems
- **Auto-replaced**: Adding a duplicate replaces the existing one

### Components

**Components** hold data and can be attached to entities (domains in this architecture):

```cpp
// Transform component example
struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

// Usage in a domain
class PlayerDomain : public v::Domain<PlayerDomain> {
public:
    void init_standard_components() override {
        // Add component to this domain's entity
        engine().emplace<TransformComponent>(entity(),
            glm::vec3{0.0f, 0.0f, 0.0f},  // position
            glm::vec3{0.0f, 0.0f, 0.0f},  // rotation
            glm::vec3{1.0f, 1.0f, 1.0f}   // scale
        );
    }
};
```

## Architectural Patterns

### 1. Recursive Domain Pattern

Domains can act as entities, components, and systems:

```cpp
// Domain as entity (stored in ECS registry)
engine.add_domain<PlayerDomain>();

// Domain as component (attached to another entity)
player_entity.add_component<InventoryDomain>();

// Domain as system (processes other entities)
terrain_domain.process_visible_chunks(player_entities);
```

### 2. Service Locator Pattern

Contexts provide global services while maintaining testability:

```cpp
class MySystem {
public:
    void update(v::Engine& engine) {
        // Access services through engine
        auto* render_ctx = engine.get_ctx<v::RenderContext>();
        auto* window_ctx = engine.get_ctx<v::WindowContext>();

        // Use services
        auto window_size = window_ctx->get_size();
        render_ctx->set_render_target(window_size);
    }
};
```

### 3. Task Dependency Pattern

Declarative task ordering ensures proper execution sequence:

```cpp
// Define task dependencies
engine.on_tick.connect({}, {}, "input", []() { /* Process input */ });
engine.on_tick.connect({"input"}, {}, "physics", []() { /* Update physics */ });
engine.on_tick.connect({"physics"}, {}, "ai", []() { /* Update AI */ });
engine.on_tick.connect({"ai"}, {}, "render", []() { /* Render frame */ });
```

### 4. RAII Resource Management

All resources use RAII for automatic cleanup:

```cpp
class MyDomain : public v::Domain<MyDomain> {
public:
    MyDomain(v::Engine& engine) : v::Domain(engine, "MyDomain") {
        // Resources automatically cleaned up
        buffer_ = create_gpu_buffer();
        texture_ = create_gpu_texture();
        audio_source_ = create_audio_source();
    }

    // No explicit cleanup needed - destructor handles it
    ~MyDomain() = default;

private:
    GPUBuffer buffer_;
    GPUTexture texture_;
    AudioSource audio_source_;
};
```

## Data Flow

### Typical Frame Execution

```
1. Input Processing
   └── SDLContext processes window and input events

2. Game Logic Updates
   ├── PlayerDomain processes player input
   ├── AIDomain updates AI behaviors
   ├── PhysicsDomain simulates physics
   └── NetworkDomain handles network messages

3. Resource Preparation
   ├── Domains upload data to GPU
   ├── RenderDomains prepare GPU resources
   └── Uniform buffers updated

4. GPU Rendering
   ├── RenderDomains submit tasks to task graph
   ├── Daxa handles GPU synchronization
   └── Final image presented to swapchain

5. Cleanup
   ├── Deferred destruction processed
   ├── Garbage collection for GPU resources
   └── Frame-end tasks executed
```

### Cross-Domain Communication

```cpp
// Query pattern for loose coupling
class RenderDomain : public v::RenderDomain<RenderDomain> {
public:
    void add_render_tasks(daxa::TaskGraph& graph) override {
        // Query other domains at graph construction time
        auto* terrain = engine_.try_get_domain<TerrainDomain>();
        auto* lighting = engine_.try_get_domain<LightingDomain>();

        // Build tasks with conditional dependencies
        if (terrain) {
            graph.add_task(...).reads(terrain->heightmap());
        }
        if (lighting) {
            graph.add_task(...).reads(lighting->light_buffers());
        }
    }
};
```

## Memory Management

### Stack Allocation
- Most objects use stack allocation
- RAII ensures automatic cleanup
- Exception safety guaranteed

### Heap Allocation
- Used for large resources and polymorphic objects
- Managed through smart pointers (unique_ptr, shared_ptr)
- Automatic cleanup through RAII

### GPU Resources
- Managed by Daxa (Vulkan abstraction)
- Automatic garbage collection each frame
- Explicit destruction for large resource changes

```cpp
// Example of automatic GPU resource management
class RenderDomain : public v::RenderDomain<RenderDomain> {
public:
    RenderDomain(v::Engine& engine) : v::RenderDomain(engine, "Render") {
        // GPU resources automatically tracked and cleaned up
        pipeline_ = daxa.pipeline_manager.add_raster_pipeline2({...});
        buffer_ = daxa.device.create_buffer({...});
        image_ = daxa.device.create_image({...});
    }

    ~RenderDomain() {
        // GPU resources automatically destroyed
        // Daxa handles deferred destruction if GPU is still using them
    }
};
```

## Extensibility and Modding

The architecture is designed from the ground up for extensibility:

### Plugin System (Planned)
```cpp
// Mod interface
class IMod {
public:
    virtual void init(v::Engine& engine) = 0;
    virtual void shutdown() = 0;
};

// Mod implementation
class MyMod : public IMod {
public:
    void init(v::Engine& engine) override {
        // Add mod-specific domains
        engine.add_domain<MyModDomain>();

        // Add mod-specific contexts
        engine.add_ctx<MyModContext>();

        // Register mod systems
        register_mod_systems(engine);
    }
};
```

### Script Integration (Planned)
- Lua scripting support for game logic
- C# integration for high-level game code
- JavaScript support for UI and tools

## Performance Considerations

### Cache-Friendly Design
- Component data stored contiguously
- Systems operate on data in cache-friendly order
- Minimal pointer chasing in hot paths

### GPU Optimization
- Task graph system minimizes GPU stalls
- Automatic barrier insertion and synchronization
- Efficient resource reuse and pooling

### Memory Efficiency
- Sparse ECS storage for optional components
- Object pooling for frequently allocated objects
- Lazy loading and streaming for large assets

## Comparison with Traditional Architectures

### Monolithic Game Engine
```
Traditional: Game Engine -> Fixed Systems -> Game Code
V Engine:    Engine -> Modular Domains -> Game Code + Mods
```

### Pure ECS
```
Pure ECS:   Entities -> Components -> Systems (no data in systems)
V Engine:   Domains (Entity + Component + System) -> Flexible Composition
```

### Component-Based Architecture
```
Traditional: Components communicate via messages
V Engine:    Direct queries with loose coupling + optional messaging
```

This architecture provides the performance benefits of data-oriented design while maintaining the flexibility and usability of object-oriented programming.

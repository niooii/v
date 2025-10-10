# Getting Started

This guide will help you get V Engine up and running, understand the project structure, and create your first application.

## Prerequisites

### System Requirements
- **Operating System**: Linux or Windows
- **Compiler**: Clang (recommended) or GCC with C++23 support
- **Build Tools**: CMake 3.20+, Python 3.8+
- **GPU**: Vulkan-compatible graphics card with up-to-date drivers

### Dependencies
The engine uses git submodules for dependency management. All required libraries are included in the `extern/` directory:

- **Daxa**: Modern Vulkan wrapper and task graph system
- **EnTT**: Entity Component System framework
- **SDL3**: Windowing, input, and platform abstraction
- **GLM**: Mathematics library for 3D graphics
- **Tracy**: Performance profiling and debugging
- **reflect-cpp**: Serialization library (CBOR format)

## Building the Project

### 1. Clone the Repository
```bash
git clone --recurse-submodules https://github.com/your-repo/v.git
cd v

# If you forgot to clone with submodules:
git submodule update --init --recursive
```

### 2. Build Configuration
The project uses a Python-based build system (`v.py`) that configures CMake and manages targets:

```bash
# Show available targets and options
./v.py --help

# Configure and build all targets (Debug configuration)
./v.py build

# Build specific configuration
./v.py build --config release

# Build specific target
./v.py build client
./v.py build experiments/voxel-rendering
```

### 3. Run Applications
```bash
# Run the main client
./v.py run client

# Run experiments and demos
./v.py run experiments/voxel-rendering

# Run with specific configuration
./v.py run client --config release
```

### 4. Testing
```bash
# Run all tests
./v.py test

# Run specific test
./v.py test some_test_target
```

## Project Structure

Understanding the project layout is key to working effectively with V Engine:

```
v/
├── client/                     # Client application
│   ├── include/               # Client headers
│   ├── src/                   # Client source files
│   ├── resources/             # Client resources (shaders, assets)
│   └── CMakeLists.txt         # Client build configuration
├── common/                    # Shared engine library (vlib)
│   ├── include/               # Engine headers
│   │   ├── engine/           # Core engine APIs
│   │   ├── vox/              # Voxel-specific systems
│   │   └── ...               # Other engine modules
│   └── src/                   # Engine implementation
├── server/                    # Server application
├── experiments/               # R&D and testing projects
├── extern/                    # Third-party dependencies
├── tests/                     # Unit tests
├── docs/                      # Documentation (you're here!)
├── v.py                       # Build system and project management
└── CMakeLists.txt            # Root build configuration
```

## Creating Your First Application

Let's create a minimal V Engine application to understand the basic structure:

### 1. Create the Application Directory
```bash
mkdir my_app
cd my_app
```

### 2. Create the Main Source File
```cpp
// main.cpp
#include <engine/engine.h>
#include <engine/contexts/window/sdl.h>
#include <engine/contexts/render/ctx.h>
#include <engine/contexts/render/render_domain.h>

class MyApp {
public:
    MyApp(v::Engine& engine) : engine_(engine) {
        setup_contexts();
        setup_rendering();
        setup_tasks();
    }

    void run() {
        // Main game loop would go here
        // For now, just run a few frames to test
        for (int i = 0; i < 100; ++i) {
            engine_.tick();
        }
    }

private:
    v::Engine& engine_;

    void setup_contexts() {
        // Initialize core systems
        sdl_ctx_ = engine_.add_ctx<v::SDLContext>();
        window_ctx_ = engine_.add_ctx<v::WindowContext>();
        render_ctx_ = engine_.add_ctx<v::RenderContext>("../common/resources/shaders");
    }

    void setup_rendering() {
        // Add basic rendering domains
        engine_.add_domain<v::DefaultRenderDomain>();
        // Add your custom render domains here
    }

    void setup_tasks() {
        // Setup task dependencies
        engine_.on_tick.connect({}, {}, "window_update", [this]() {
            window_ctx_->update();
        });

        engine_.on_tick.connect({"window_update"}, {}, "render", [this]() {
            render_ctx_->update();
        });
    }

    v::SDLContext* sdl_ctx_;
    v::WindowContext* window_ctx_;
    v::RenderContext* render_ctx_;
};

int main() {
    v::Engine engine;

    try {
        MyApp app(engine);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

### 3. Create CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app)

# Find the V Engine library
find_package(vlib REQUIRED)

# Create executable
add_executable(my_app main.cpp)

# Link with V Engine
target_link_libraries(my_app PRIVATE vlib)

# Set C++ standard
target_compile_features(my_app PRIVATE cxx_std_23)
```

### 4. Build and Run
```bash
# From the V Engine root directory
./v.py build my_app
./v.py run my_app
```

## Understanding the Core Concepts

### Engine and ECS Architecture
V Engine uses an Entity-Component-System (ECS) architecture at its core:

```cpp
v::Engine engine;  // Main engine orchestrator

// Add singleton contexts (services)
auto* render_ctx = engine.add_ctx<v::RenderContext>("./shaders");
auto* window_ctx = engine.add_ctx<v::WindowContext>();

// Add domains (modular systems)
engine.add_domain<MyGameDomain>();

// Main loop
while (running) {
    engine.tick();  // Processes all systems and tasks
}
```

### Contexts vs Domains
- **Contexts**: Singleton services (rendering, networking, windowing)
- **Domains**: Modular systems that can be added/removed dynamically

```cpp
// Contexts - singleton services
auto* render_ctx = engine.get_ctx<v::RenderContext>();
auto* net_ctx = engine.get_ctx<v::NetworkContext>();

// Domains - modular systems
auto* game_domain = engine.get_domain<MyGameDomain>();
auto* terrain_domain = engine.try_get_domain<TerrainDomain>();  // Optional
```

### Task System
The engine uses a task dependency system for ordering operations:

```cpp
// Define task dependencies
engine.on_tick.connect(
    {"input"},           // After input task
    {"physics"},         // Before physics task
    "player_movement",   // Task name
    []() { /* Update players */ }
);
```

## Common Patterns

### Resource Management
V Engine uses RAII extensively - resources are automatically cleaned up:

```cpp
class MyDomain : public v::Domain<MyDomain> {
public:
    MyDomain(v::Engine& engine) : v::Domain(engine, "MyDomain") {
        // Resources are cleaned up automatically
        buffer_ = create_buffer();
        texture_ = create_texture();
    }

    // No need for explicit cleanup - destructor handles it
    ~MyDomain() = default;

private:
    Buffer buffer_;
    Texture texture_;
};
```

### Error Handling
Use exceptions for error handling:

```cpp
try {
    v::Engine engine;
    MyApp app(engine);
    app.run();
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
```

### Cross-Platform Considerations
The engine handles platform differences automatically:

```cpp
// This works on Windows and Linux
auto* window_ctx = engine.add_ctx<v::WindowContext>();
window_ctx->create_window({1920, 1080, "My App"});
```

## Next Steps

Now that you have a basic understanding of V Engine, explore these topics:

1. **[Creating Domains](creating-domains.md)** - Learn to build custom game systems
2. **[Render Domains](../rendering/render-domains.md)** - Understand GPU rendering with RenderDomains
3. **[Architecture Patterns](../architecture/patterns.md)** - Deep dive into engine architecture
4. **[Voxel Storage](voxel-storage.md)** - Work with voxel data structures

## Troubleshooting

### Common Build Issues

**CMake Configuration Errors**:
```bash
# Clean build directory
./v.py clean
./v.py build
```

**Missing Submodules**:
```bash
git submodule update --init --recursive
```

**Compiler Issues**:
Ensure you have a C++23 compatible compiler:
```bash
clang++ --version  # Should be version 14+
g++ --version      # Should be version 11+
```

### Runtime Issues

**Vulkan Initialization Errors**:
- Update your graphics drivers
- Ensure Vulkan SDK is installed
- Check that your GPU supports Vulkan

**Window Creation Errors**:
- Make sure you're running on a supported platform (Linux/Windows)
- Check that SDL3 can access the display system

### Getting Help

- Check the [documentation](../README.md) for detailed information
- Look at the [experiments](../../experiments/) for working examples
- Examine the [client application](../../client/src/) for a complete implementation

## Profiling

V Engine includes Tracy profiler integration for performance analysis:

```bash
# Build with profiling enabled
./v.py build --config profile

# Run and connect Tracy profiler
./v.py run client --config profile
# Then connect Tracy GUI to the running application
```

This will give you detailed insights into:
- Frame timing and bottlenecks
- CPU and GPU usage
- Memory allocations
- Task execution times
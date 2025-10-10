# Getting Started

## Prerequisites

- Linux or Windows
- Clang with C++23 support
- CMake 3.20+, Python 3.8+
- Vulkan-compatible GPU with updated drivers
- [Vulkan SDK](https://vulkan.lunarg.com/)
- GLM (via system package manager or build from source)

### Submodules
All other dependencies are in `extern/` via git submodules (Daxa, EnTT, SDL3, Tracy, reflect-cpp).

## Build

```bash
git clone --recurse-submodules <repo-url>
cd v
git submodule update --init --recursive  # if forgot --recurse-submodules

# Build
./v.py build                # Debug
./v.py build --release      # Release
./v.py build vclient        # Specific target

# Run
./v.py run vclient
./v.py run vclient --release

# Test
./v.py test
```

Aliases: `./v` (*nix), `v.bat` (Windows).

## Structure

```
v/
├── client/         # Client app
├── common/         # Engine library (vlib)
│   ├── include/engine/  # Core APIs
│   └── include/vox/     # Voxel systems
├── server/         # Server app
├── experiments/    # Demos
├── extern/         # Dependencies (submodules)
├── tests/          # Unit tests
└── v.py            # Build script
```

## First Application

### main.cpp
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

## Core Concepts

- **Engine**: ECS orchestrator, owns registries and tick loop
- **Context**: Singleton service (e.g., RenderContext, WindowContext)
- **Domain**: Modular system owning components and logic
- **Task dependencies**: Order operations via `engine.on_tick.connect()`

## Next Steps

- [Creating Domains](creating-domains.md)
- [Render Domains](../rendering/render-domains.md)
- [Architecture Patterns](../architecture/patterns.md)
- [Voxel Storage](voxel-storage.md)

## Troubleshooting

**Build fails**: `./v.py clean && ./v.py build`
**Missing submodules**: `git submodule update --init --recursive`
**Vulkan errors**: Update GPU drivers, install [Vulkan SDK](https://vulkan.lunarg.com/)

## Profiling

```bash
./v.py profile vclient  # Tracy-enabled build
# Connect Tracy GUI to capture session
```
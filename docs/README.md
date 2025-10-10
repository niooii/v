# V Engine Documentation

Welcome to the V Engine documentation! V is a modern C++23 voxel rendering engine built with a hybrid ECS-OOP architecture designed for modularity, extensibility, and seamless C++/C# interop.

## Quick Start

```bash
# Build the project
./v.py build

# Run a demo
./v.py run experiments/voxel-rendering

# Run tests
./v.py test
```

## Architecture Overview

V Engine uses a unique **Domain-Centric Architecture** that combines the best of Entity-Component-System design with object-oriented patterns:

- **Engine**: Core orchestrator using ECS architecture with EnTT
- **Contexts**: Singleton services (rendering, networking, windowing, etc.)
- **Domains**: Modular systems that own components and logic
- **RenderDomains**: Specialized domains for GPU rendering tasks

This design allows systems to be combined like LEGO blocks while maintaining high performance and clean separation of concerns.

## Documentation Structure

### [Getting Started](guides/getting-started.md)
- Build system setup and configuration
- Running your first application
- Project structure overview

### [Architecture](architecture/)
- [Overview](architecture/overview.md) - High-level system design
- [Patterns](architecture/patterns.md) - Core architectural patterns and design philosophy
- [ECS System](architecture/ecs-system.md) - Entity-Component-System implementation details

### [Rendering System](rendering/)
- [Overview](rendering/overview.md) - Rendering pipeline and architecture
- [Render Domains](rendering/render-domains.md) - Modular GPU rendering system
- [Daxa Integration](rendering/daxa-integration.md) - Vulkan abstraction layer
- [Shaders](rendering/shaders.md) - Shader system and pipeline creation

### [User Guides](guides/)
- [Creating Domains](guides/creating-domains.md) - Building custom game systems
- [Render Domain Tutorial](guides/render-domain-tutorial.md) - Step-by-step GPU rendering
- [Voxel Storage](guides/voxel-storage.md) - Voxel data structures and usage

### [Reference](reference/)
- [Build System](reference/build-system.md) - v.py build system and project management
- [Threading](reference/threading.md) - Threading patterns and synchronization
- [Networking](reference/networking.md) - Network system architecture
- [File Reference](reference/file-reference.md) - Key files and their purposes

## Key Features

- **Modern C++23**: Latest language features with Clang compiler
- **Vulkan Rendering**: High-performance GPU rendering via Daxa
- **Modular Architecture**: Domain-based system for easy extension
- **Cross-Platform**: Windows and Linux support
- **Voxel-Optimized**: Sparse64Tree octree for efficient voxel storage
- **Network Ready**: Built-in networking for multiplayer games
- **Performance Focused**: Tracy profiler integration and task-based architecture

## Design Philosophy

V Engine follows the principles outlined in [Object-Oriented Entity-Component-System Design](https://johnlin.se/blog/2021/07/27/object-oriented-entity-component-system-design/):

1. **Practical over Pure**: We break "pure ECS rules" when it makes the engine more usable
2. **Recursive Architecture**: Domains can act as entities, components, and systems
3. **LEGO-like Composition**: Systems should combine intuitively without hardcoded dependencies
4. **Performance and Usability**: C++ for performance-critical systems, easy extensibility for game logic

## Project Status

This is a **work-in-progress** engine rewrite. The core architecture is solid and the rendering system is functional, but many features are still being implemented.

Current capabilities:
- ✅ Core ECS architecture with domains and contexts
- ✅ Vulkan rendering with Daxa integration
- ✅ Modular RenderDomain system
- ✅ Window management and input handling
- ✅ Basic voxel storage with Sparse64Tree
- 🚧 Advanced voxel rendering (in development)
- 🚧 Complete networking system (planned)
- 🚧 C# interop layer (planned)

## Contributing

This documentation is part of the source code. If you find inconsistencies or have suggestions, please contribute by updating the relevant documentation files alongside the code they describe.

## License

[Add your license information here]
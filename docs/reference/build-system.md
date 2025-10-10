# Build System

V Engine uses a Python-based build system (`v.py`) that wraps CMake and provides convenient commands for building, running, and testing the project. This document explains how to use the build system effectively.

## Overview

The build system is designed to:
- Simplify common development tasks
- Provide cross-platform compatibility
- Manage multiple build configurations
- Handle dependency management
- Integrate with development tools

## Basic Usage

### Commands

```bash
# Show help and available commands
./v.py --help

# Build all targets (debug configuration by default)
./v.py build

# Build specific target
./v.py build client
./v.py build experiments/voxel-rendering

# Build with specific configuration
./v.py build --config debug
./v.py build --config release
./v.py build --config profile

# Run applications
./v.py run client
./v.py run experiments/voxel-rendering

# Run tests
./v.py test

# Clean build directories
./v.py clean

# Show available targets
./v.py list-targets
```

### Configuration Options

```bash
# Build configurations
./v.py build --config debug      # Debug build with full symbols
./v.py build --config release    # Optimized release build
./v.py build --config profile    # Release with profiling symbols

# Verbosity
./v.py build --verbose           # Show detailed build output
./v.py build --quiet             # Minimal output

# Parallel builds
./v.py build --jobs 8            # Use 8 parallel jobs
./v.py build --jobs $(nproc)     # Use all available CPU cores

# Target discovery
./v.py build "voxel*"            # Build targets matching pattern
./v.py build --fuzzy voxels      # Fuzzy search for targets
```

## Project Structure

The build system expects the following project structure:

```
v/
├── v.py                       # Main build script
├── CMakeLists.txt            # Root CMake configuration
├── cmake/                    # CMake modules and presets
│   ├── presets/              # Build presets
│   └── modules/              # Custom CMake modules
├── common/                   # Shared engine library
│   └── CMakeLists.txt        # Common library build config
├── client/                   # Client application
│   └── CMakeLists.txt        # Client build config
├── server/                   # Server application
│   └── CMakeLists.txt        # Server build config
├── experiments/              # Test and demo projects
│   ├── voxels/
│   │   └── CMakeLists.txt
│   └── ...
└── tests/                    # Unit tests
    └── CMakeLists.txt
```

## CMake Integration

### Presets Configuration

The build system uses CMake presets for consistent build configurations:

```json
// cmake/presets/debug.json
{
  "version": 6,
  "name": "debug",
  "displayName": "Debug",
  "generator": "Ninja",
  "toolset": {
    "value": "Clang",
    "strategy": "external"
  },
  "toolchainFile": "${sourceDir}/cmake/toolchain.cmake",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_CXX_STANDARD": "23",
    "ENABLE_PROFILING": "OFF"
  }
}
```

### Toolchain Configuration

```cmake
# cmake/toolchain.cmake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_PROFILE "-O3 -g -DNDEBUG")
endif()
```

## Advanced Usage

### Custom Build Commands

```bash
# Build with custom defines
./v.py build --define "ENABLE_DEBUG_LOGS=ON" --define "USE_VULKAN_VALIDATION=ON"

# Build specific subset
./v.py build --exclude tests          # Skip test targets
./v.py build --only client           # Only build client and dependencies

# Incremental builds
./v.py build --incremental           # Skip reconfiguration if possible
```

### Development Workflow

```bash
# Typical development cycle
./v.py build client                  # Build client
./v.py run client                    # Run application
# Make changes...
./v.py build client                  # Rebuild with changes

# Debug workflow
./v.py build --config debug          # Build with debug symbols
./v.py run client                    # Run debug build
# Attach debugger...

# Release workflow
./v.py build --config release        # Build optimized release
./v.py run client                    # Test release build
```

### Profiling Integration

```bash
# Build with profiling enabled
./v.py build --config profile

# Run with Tracy profiler
./v.py run client --config profile

# Tracy profiler will be available at localhost:8080
# Connect Tracy GUI to see profiling data
```

## Target Management

### Available Targets

```bash
# List all available targets
./v.py list-targets

# Example output:
# client
# server
# experiments/voxel-rendering
# experiments/triangle-demo
# tests/engine_tests
# tests/unit_tests
# vlib (engine library)
```

### Target Patterns

```bash
# Build all experiments
./v.py build "experiments/*"

# Build all test targets
./v.py build "tests/*"

# Build client and server
./v.py build "client,server"

# Fuzzy matching
./v.py build --fuzzy "vox"    # Matches experiments/voxel-rendering
```

## Dependency Management

### Git Submodules

Dependencies are managed through git submodules:

```bash
# Initialize all submodules
git submodule update --init --recursive

# Update submodules
git submodule update --remote

# Update specific submodule
git submodule update --remote extern/daxa
```

### Submodule Structure

```
extern/
├── daxa/                   # Vulkan abstraction library
├── entt/                   # Entity Component System
├── glm/                    # Mathematics library
├── sdl3/                   # Windowing and input
├── tracy/                  # Profiling
└── reflect-cpp/            # Serialization
```

### Custom Dependencies

Adding a new dependency:

1. Add as git submodule:
```bash
cd extern
git submodule add https://github.com/user/library.git
```

2. Update CMakeLists.txt:
```cmake
# In common/CMakeLists.txt
add_subdirectory(extern/library)

# Link to targets
target_link_libraries(vlib PRIVATE library_target)
```

3. Update v.py if needed for special handling.

## Cross-Platform Building

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install clang cmake ninja-build pkg-config
sudo apt install libvulkan-dev vulkan-tools glslangValidator

# Install dependencies (Arch)
sudo pacman -S clang cmake ninja pkg-config
sudo pacman -S vulkan-devel vulkan-tools glslang

# Build
./v.py build
```

### Windows

```bash
# Install Visual Studio with C++ and CMake
# Or install LLVM/Clang

# Install Vulkan SDK from LunarG

# Build using Visual Studio Developer Command Prompt
./v.py build

# Or using MinGW/MSYS2
./v.py build --generator "MinGW Makefiles"
```

## IDE Integration

### VS Code

The build system works well with VS Code's C/C++ and CMake Tools extensions:

```json
// .vscode/settings.json
{
  "cmake.configureOnOpen": true,
  "cmake.buildDirectory": "${workspaceFolder}/build/${buildType}",
  "cmake.generator": "Ninja",
  "cmake.configureArgs": [
    "-DCMAKE_BUILD_TYPE=${buildType}"
  ]
}
```

```json
// .vscode/tasks.json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build Debug",
      "type": "shell",
      "command": "./v.py",
      "args": ["build", "--config", "debug"],
      "group": "build"
    },
    {
      "label": "Run Client",
      "type": "shell",
      "command": "./v.py",
      "args": ["run", "client"],
      "group": "test"
    }
  ]
}
```

### CLion

CLion can use the CMake presets directly:

1. Open the project directory
2. CLion will detect CMakePresets.json
3. Select the desired preset (debug/release/profile)
4. Build and run as normal

### Vim/Neovim

```vim
" vimrc configuration for v.py
nnoremap <leader>b :w<CR>:!./v.py build<CR>
nnoremap <leader>r :!./v.py run client<CR>
nnoremap <leader>t :!./v.py test<CR>
```

## Performance Optimization

### Build Caching

```bash
# Use ccache for faster builds (Linux)
export CC="ccache clang"
export CXX="ccache clang++"

# Or configure in CMake
./v.py build --define "CMAKE_C_COMPILER_LAUNCHER=ccache"
./v.py build --define "CMAKE_CXX_COMPILER_LAUNCHER=ccache"
```

### Parallel Builds

```bash
# Use all available CPU cores
./v.py build --jobs $(nproc)

# Limit parallelism on low-memory systems
./v.py build --jobs 4
```

### Incremental Builds

The build system automatically performs incremental builds when possible:

```bash
# Only rebuild changed files
./v.py build

# Force complete rebuild
./v.py clean
./v.py build
```

## Troubleshooting

### Common Issues

**CMake Configuration Errors**:
```bash
# Clean and reconfigure
./v.py clean
./v.py build --verbose
```

**Missing Dependencies**:
```bash
# Check submodules
git submodule status

# Update submodules
git submodule update --init --recursive
```

**Compilation Errors**:
```bash
# Build with verbose output to see errors
./v.py build --verbose

# Check specific target
./v.py build client --verbose
```

**Linker Errors**:
```bash
# Clean and rebuild
./v.py clean
./v.py build

# Check library paths
./v.py build --define "CMAKE_VERBOSE_MAKEFILE=ON"
```

### Getting Help

```bash
# Show all options
./v.py --help

# Show CMake options
./v.py build --help-cmake

# Show target information
./v.py info target client
```

### Build System Internals

The build script (`v.py`) is a Python script that:

1. **Parses Arguments**: Handles command-line options
2. **Detects Environment**: Platform, compiler, available tools
3. **Configures CMake**: Sets up build directories and presets
4. **Manages Dependencies**: Ensures submodules are initialized
5. **Executes Commands**: Runs CMake, ninja, make, etc.
6. **Provides Convenience**: Wraps common development workflows

The script is designed to be easily modified for project-specific needs while maintaining a consistent interface.

## Customization

### Adding New Commands

You can extend the build system by modifying `v.py`:

```python
# Add custom command to v.py
def cmd_custom_build(args):
    """Custom build command"""
    if args.target:
        subprocess.run(["./v.py", "build", args.target])
    else:
        print("No target specified")

# Register the command
parser.add_argument("custom-build", help="Custom build command")
```

### Environment Variables

The build system respects these environment variables:

```bash
# Override default compiler
export CC=clang
export CXX=clang++

# Set build parallelism
export V_BUILD_JOBS=8

# Enable verbose output
export V_VERBOSE=1

# Custom build directory
export V_BUILD_DIR=custom_build
```

This build system provides a solid foundation for development while remaining flexible enough to accommodate project-specific needs and custom workflows.
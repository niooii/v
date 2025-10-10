# Build System

Python build script `v.py` wrapping CMake. Aliases: `./v` (*nix), `v.bat` (Windows).

## Commands

```bash
./v.py build              # Debug build (all targets)
./v.py build --release    # Release build
./v.py build vclient      # Specific target (fuzzy matching supported)
./v.py run vclient        # Build + run
./v.py test               # Build + run all tests
./v.py clean              # Clean build directory
./v.py targets            # List available targets
./v.py profile vclient    # Build with Tracy profiler
./v.py format             # Format code with clang-format
```

## Flags

```bash
--release               # Release build (default: debug)
--verbose               # Verbose logging (sets V_LOG_LEVEL=trace)
--full                  # Show full build output
```

## Logging

Runtime log level controlled via `V_LOG_LEVEL` environment variable:

```bash
V_LOG_LEVEL=trace ./v.py run vclient  # All logs
V_LOG_LEVEL=debug ./v.py run vclient  # Debug and above
V_LOG_LEVEL=info ./v.py run vclient   # Info and above (default for tests)
V_LOG_LEVEL=warn ./v.py run vclient   # Warnings and errors only
V_LOG_LEVEL=error ./v.py run vclient  # Errors only
```

Compile-time levels set in `CMakeLists.txt`:
- Debug: `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE`
- Release: `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO` + `SPDLOG_NO_SOURCE_LOC`

## CMake Presets

Uses `CMakePresets.json`. Presets:
- `debug` → `cmake-build-debug/`
- `release` → `cmake-build-release/`

Compiler flags set in `CMakeLists.txt`:
- Debug: `-g3 -Og`
- Release: `-Ofast -funroll-loops -fvectorize`

## Submodules

Dependencies in `extern/`:

```bash
git submodule update --init --recursive  # Initialize all
git submodule update --remote            # Update all
```

## System Dependencies

Install yourself:
- Clang (C++23 support)
- CMake 3.20+
- Ninja
- Python 3.8+
- uv
- [Vulkan SDK](https://vulkan.lunarg.com/)
- GLM (system package or build from source)

### Linux (Arch)
```bash
sudo pacman -S clang cmake ninja vulkan-devel vulkan-tools glslang glm
```

### Linux (Ubuntu/Debian)
```bash
sudo apt install clang cmake ninja-build libvulkan-dev vulkan-tools glslang-tools libglm-dev
```

## Fuzzy Target Matching

```bash
./v.py build domain      # Matches vtest-domain
./v.py run vexp          # Matches vexp-voxel-rendering
```

Uses rapidfuzz for matching.

## Tracy Profiling

```bash
./v.py profile vclient   # Builds with TRACY_ENABLE
# Launch Tracy GUI to capture
```

## Troubleshooting

**Build fails**: `./v.py clean && ./v.py build --full`
**Missing submodules**: `git submodule update --init --recursive`
**Clang not found**: Ensure clang++ in PATH

## Adding Targets

Add to appropriate `CMakeLists.txt` (client/, server/, tests/, experiments/). Targets prefixed with `v` are auto-discovered.

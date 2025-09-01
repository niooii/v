#!/bin/bash

# Usage: ./v.sh [build/run/clean/reload] [target] [--release]

set -e

# Default values
BUILD_TYPE="Debug"
BUILD_DIR="cmake-build-debug"
COMMAND=""
TARGET=""
CLEAN_ALL=false
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        build|run|clean|reload)
            if [[ -n "$COMMAND" ]]; then
                print_error "Multiple commands specified. Use only one of: build, run, clean, reload"
                exit 1
            fi
            COMMAND="$1"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            BUILD_DIR="cmake-build-release"
            shift
            ;;
        all)
            if [[ "$COMMAND" == "clean" ]]; then
                CLEAN_ALL=true
                shift
            else
                print_error "'all' flag is only valid with clean command"
                exit 1
            fi
            ;;
        vclient|vserver|vlib)
            if [[ -n "$TARGET" ]]; then
                print_error "Multiple targets specified. Use only one target."
                exit 1
            fi
            TARGET="$1"
            shift
            ;;
        *)
            print_error "Unknown argument: $1"
            echo "Usage: ./v.sh [build/run/clean/reload] [target/all] [--release]"
            echo "Commands: build, run, clean, reload"
            echo "Targets: vclient, vserver, vlib"
            echo "Clean options: all (includes external built libraries)"
            echo "Flags: --release (default is debug)"
            exit 1
            ;;
    esac
done

# Validate command
if [[ -z "$COMMAND" ]]; then
    print_error "No command specified"
    echo "Usage: ./v.sh [build/run/clean/reload] [target/all] [--release]"
    echo "Commands: build, run, clean, reload"
    echo "Targets: vclient, vserver, vlib"
    echo "Clean options: all (removes everything including extern/)"
    echo "Flags: --release (default is debug)"
    exit 1
fi

configure_cmake() {
    local preset_name=""
    if [[ "$BUILD_TYPE" == "Release" ]]; then
        preset_name="release"
    else
        preset_name="debug"
    fi
    
    print_status "Configuring CMake using preset: $preset_name"
    
    cmake --preset "$preset_name" -D CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
    
    print_success "CMake configured successfully using preset: $preset_name"
}

build_target() {
    local target="$1"
    local preset_name=""
    local build_preset=""
    
    if [[ "$BUILD_TYPE" == "Release" ]]; then
        preset_name="release"
    else
        preset_name="debug"
    fi
    
    # Check if we need to configure
    if [[ ! -d "$BUILD_DIR" || ! -f "$BUILD_DIR/build.ninja" ]]; then
        configure_cmake
    fi
    
    # Determine build preset based on target and build type
    if [[ -n "$target" ]]; then
        build_preset="${preset_name}-${target}"
        print_status "Building target: $target ($BUILD_TYPE)"
        cmake --build --preset "$build_preset"
        print_success "Target $target built successfully"
    else
        build_preset="${preset_name}-build"
        print_status "Building all targets ($BUILD_TYPE)"
        cmake --build --preset "$build_preset"
        print_success "All targets built successfully"
    fi
}

run_executable() {
    local target="$1"
    
    case "$target" in
        vclient|vserver)
            if [[ -f "$BUILD_DIR/$target" ]]; then
                print_status "Running $target..."
                cd "$BUILD_DIR"
                "./$target"
                cd "$PROJECT_ROOT"
            else
                print_error "Executable $target not found. Did you build it first?"
                exit 1
            fi
            ;;
        vlib)
            print_warning "vlib is a static library and cannot be executed"
            print_status "Build completed successfully - library ready for linking"
            ;;
        "")
            print_error "No target specified for run command"
            exit 1
            ;;
        *)
            print_error "Unknown target: $target"
            exit 1
            ;;
    esac
}

print_status "Using build directory: $BUILD_DIR"
print_status "Build type: $BUILD_TYPE"

case "$COMMAND" in
    clean)
        if [[ -d "$BUILD_DIR" ]]; then
            if [[ "$CLEAN_ALL" == true ]]; then
                print_status "Cleaning all built: $BUILD_DIR"
                rm -rf "$BUILD_DIR"
                print_success "Build directory nuked"
            else
                print_status "Cleaning project built: $BUILD_DIR"
                find "$BUILD_DIR" -mindepth 1 -maxdepth 1 ! -name 'extern' ! -name 'CMakeCache.txt' ! -name 'CMakeFiles' ! -name 'build.ninja' ! -name 'cmake_install.cmake' ! -name 'compile_commands.json' ! -name '.ninja_deps' ! -name '.ninja_log' -exec rm -rf {} +
                print_success "Build directory cleaned"
            fi
        else
            print_warning "Build directory $BUILD_DIR does not exist"
        fi
        ;;
        
    build)
        build_target "$TARGET"
        ;;
        
    run)
        # Build first, then run
        build_target "$TARGET"
        run_executable "$TARGET"
        ;;
        
    reload)
        print_status "Reloading CMake cache..."
        configure_cmake
        print_success "CMake cache reloaded successfully"
        ;;
        
    *)
        print_error "Unknown command: $COMMAND"
        exit 1
        ;;
esac

print_success "Operation completed successfully"

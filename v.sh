#!/bin/bash

# Cargo-like build script for CLion CMake integration
# Usage: ./v.sh [build/run/clean] [target] [--release]

set -e

# Default values
BUILD_TYPE="Debug"
BUILD_DIR="cmake-build-debug"
COMMAND=""
TARGET=""
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
        build|run|clean)
            if [[ -n "$COMMAND" ]]; then
                print_error "Multiple commands specified. Use only one of: build, run, clean"
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
        client|server|v_lib)
            if [[ -n "$TARGET" ]]; then
                print_error "Multiple targets specified. Use only one target."
                exit 1
            fi
            TARGET="$1"
            shift
            ;;
        *)
            print_error "Unknown argument: $1"
            echo "Usage: ./v.sh [build/run/clean] [target] [--release]"
            echo "Commands: build, run, clean"
            echo "Targets: client, server, v_lib"
            echo "Flags: --release (default is debug)"
            exit 1
            ;;
    esac
done

# Validate command
if [[ -z "$COMMAND" ]]; then
    print_error "No command specified"
    echo "Usage: ./v.sh [build/run/clean] [target] [--release]"
    echo "Commands: build, run, clean"
    echo "Targets: client, server, v_lib"
    echo "Flags: --release (default is debug)"
    exit 1
fi

configure_cmake() {
    print_status "Configuring CMake for $BUILD_TYPE build..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Mimic CLion's cmake configuration exactly
    cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DCMAKE_MAKE_PROGRAM=ninja \
          -G Ninja \
          "$PROJECT_ROOT"
    
    cd "$PROJECT_ROOT"
    print_success "CMake configured successfully"
}

build_target() {
    local target="$1"
    
    # Check if we need to configure
    if [[ ! -d "$BUILD_DIR" || ! -f "$BUILD_DIR/build.ninja" ]]; then
        configure_cmake
    fi
    
    cd "$BUILD_DIR"
    
    if [[ -n "$target" ]]; then
        print_status "Building target: $target ($BUILD_TYPE)"
        ninja "$target"
        print_success "Target $target built successfully"
    else
        print_status "Building all targets ($BUILD_TYPE)"
        ninja
        print_success "All targets built successfully"
    fi
    
    cd "$PROJECT_ROOT"
}

run_executable() {
    local target="$1"
    
    case "$target" in
        client|server)
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
        v_lib)
            print_warning "v_lib is a static library and cannot be executed"
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
            print_status "Cleaning build directory: $BUILD_DIR"
            rm -rf "$BUILD_DIR"
            print_success "Build directory cleaned"
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
        
    *)
        print_error "Unknown command: $COMMAND"
        exit 1
        ;;
esac

print_success "Operation completed successfully"
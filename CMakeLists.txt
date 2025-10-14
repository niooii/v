cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_COMPILER clang++)
project(gdf
        VERSION 1.0
        DESCRIPTION "Not really a game dev final..."
        LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# fix macro conflicts
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

find_program(GLSLC glslc REQUIRED)

# Common external dependencies used by both client and server
# Taskflow
set(TF_BUILD_EXAMPLES OFF)
set(TF_BUILD_TESTS OFF)
set(TF_BUILD_BENCHMARKS OFF)
add_subdirectory(extern/taskflow)

# jsoncons (header-only JSON/CBOR library for reflect-cpp)
add_library(jsoncons INTERFACE)
target_include_directories(jsoncons INTERFACE
    ${CMAKE_SOURCE_DIR}/extern/jsoncons/include)

# Tracy profiler
option(TRACY_ENABLE "Enable Tracy profiler" ON)
option(TRACY_ON_DEMAND "Enable Tracy on-demand profiling" OFF)
option(TRACY_CALLSTACK "Enable Tracy call stack sampling" ON)
set(TRACY_CALLSTACK_DEPTH "32" CACHE STRING "Tracy call stack depth")

if(TRACY_ENABLE)
    add_library(TracyClient STATIC extern/tracy/public/TracyClient.cpp)
    target_include_directories(TracyClient PUBLIC extern/tracy/public)
    target_compile_definitions(TracyClient PUBLIC TRACY_ENABLE)

    if(TRACY_ON_DEMAND)
        target_compile_definitions(TracyClient PUBLIC TRACY_ON_DEMAND)
    endif()

    if(TRACY_CALLSTACK)
        target_compile_definitions(TracyClient PUBLIC TRACY_CALLSTACK=${TRACY_CALLSTACK_DEPTH})
    endif()

    # Disable warnings for Tracy
    target_compile_options(TracyClient PRIVATE -w)

    # Link against libdl for dladdr (needed for call stack resolution)
    target_link_libraries(TracyClient PRIVATE ${CMAKE_DL_LIBS})
endif()

# reflect-cpp needs jsoncons includes available globally for CBOR
include_directories(${CMAKE_SOURCE_DIR}/extern/jsoncons/include)

# reflect-cpp
set(REFLECTCPP_BUILD_TESTS OFF)
set(REFLECTCPP_BUILD_BENCHMARKS OFF)
set(REFLECTCPP_INSTALL OFF)
set(REFLECTCPP_CBOR ON)
set(REFLECTCPP_USE_BUNDLED_DEPENDENCIES ON)  # Use bundled ctre
add_subdirectory(extern/reflect-cpp)

add_subdirectory(extern/entt)
add_subdirectory(extern/hfsm2)
add_subdirectory(extern/spdlog)
add_subdirectory(extern/glm)

# SDL3 (needed by common library for window context)
add_subdirectory(extern/sdl3)

set(ABSL_PROPAGATE_CXX_STD ON)  
#set(ABSL_BUILD_TESTING OFF)
#set(ABSL_BUILD_TEST_HELPERS OFF)
add_subdirectory(extern/abseil-cpp)

# Disable warnings for external dependencies
set_property(DIRECTORY extern/reflect-cpp PROPERTY COMPILE_OPTIONS "-w")
set_property(DIRECTORY extern/entt PROPERTY COMPILE_OPTIONS "-w")
set_property(DIRECTORY extern/hfsm2 PROPERTY COMPILE_OPTIONS "-w")
set_property(DIRECTORY extern/spdlog PROPERTY COMPILE_OPTIONS "-w")
set_property(DIRECTORY extern/glm PROPERTY COMPILE_OPTIONS "-w")
set_property(DIRECTORY extern/abseil-cpp PROPERTY COMPILE_OPTIONS "-w")
set_property(DIRECTORY extern/taskflow PROPERTY COMPILE_OPTIONS "-w")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# CMAKE_BUILD_TYPE is now set by CMakePresets.json

# Option to enable/disable address sanitizer
option(ENABLE_SANITIZER "Enable AddressSanitizer for debug builds" ON)

set(NO_WARNING_FLAGS "-Wno-unused-parameter -Wno-missing-braces -Wno-unused-variable -Wno-ignored-qualifiers -Wno-missing-field-initializers -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-sign-conversion")

set(CMAKE_CXX_FLAGS "-Wall -W -Wextra ${NO_WARNING_FLAGS} -ffast-math -march=native -mavx2 -fcolor-diagnostics")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # release build

    # prod spdlog conf
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
    add_compile_definitions(SPDLOG_NO_SOURCE_LOC)

    # clang flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -fno-plt -fuse-ld=lld -funroll-loops -finline-functions -fomit-frame-pointer -ftree-vectorize -fvectorize -fslp-vectorize -DNDEBUG -DV_RELEASE")
else()
    # debug build
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

    # clang flags
    set(BASE_DEBUG_FLAGS "-g3 -Og -fno-omit-frame-pointer -DDEBUG")

    if(ENABLE_SANITIZER)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BASE_DEBUG_FLAGS} -fsanitize=address -fsanitize-recover=address")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BASE_DEBUG_FLAGS}")
    endif()
endif()

# Add subdirectories for each component
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)

# tests (build after core targets)
add_subdirectory(tests)

# experiment stuff
add_subdirectory(experiments)

# Set output directories for all targets (will be applied to client/server when they're created)
function(set_output_directories TARGET_NAME)
    set_target_properties(${TARGET_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endfunction()


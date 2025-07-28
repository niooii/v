cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_COMPILER clang++)
project(gdf
        VERSION 1.0
        DESCRIPTION "Not really a game dev final..."
        LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(GLSLC glslc REQUIRED)

# Common external dependencies used by both client and server
add_subdirectory(extern/ser20)
add_subdirectory(extern/entt)
add_subdirectory(extern/hfsm2)
add_subdirectory(extern/spdlog)
add_subdirectory(extern/glm)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# default build is debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

set(NO_WARNING_FLAGS "-Wno-unused-parameter -Wno-missing-braces -Wno-unused-variable -Wno-ignored-qualifiers -Wno-missing-field-initializers")

set(CMAKE_CXX_FLAGS "-Wall -W -Wextra ${NO_WARNING_FLAGS} -ffast-math -march=native -mavx2 -fcolor-diagnostics")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # release build

    # prod spdlog conf
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
    add_compile_definitions(SPDLOG_NO_SOURCE_LOC)

    # clang flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -fno-plt -fuse-ld=lld -funroll-loops -finline-functions -fomit-frame-pointer -ftree-vectorize -fvectorize -fslp-vectorize -DNDEBUG -DGDF_RELEASE")
else()
    # debug build
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

    # clang flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Og -fno-omit-frame-pointer -DDEBUG")
endif()

# Add subdirectories for each component
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)

# Set output directories for all targets (will be applied to client/server when they're created)
function(set_output_directories TARGET_NAME)
    set_target_properties(${TARGET_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endfunction()



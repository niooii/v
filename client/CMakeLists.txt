# Client-specific dependencies
add_subdirectory(
        ${CMAKE_SOURCE_DIR}/extern/sdl3
        ${CMAKE_BINARY_DIR}/extern/sdl3
)

# Add imgui as a library (header-only, needs manual setup)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui)
file(GLOB IMGUI_SOURCES
        ${IMGUI_DIR}/*.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

set(DAXA_ENABLE_UTILS_TASK_GRAPH ON CACHE BOOL "" FORCE)

add_subdirectory(
        ${CMAKE_SOURCE_DIR}/extern/daxa
        ${CMAKE_BINARY_DIR}/extern/daxa
)

# Find all client source files
file(GLOB_RECURSE CLIENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create client executable (rename to avoid directory conflict)
add_executable(vclient ${CLIENT_SOURCES}
        src/contexts/render/per_window_init.cpp)

target_compile_definitions(vclient PUBLIC GDF_CLIENT_BUILD)

target_include_directories(vclient PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Link client-specific libraries
target_link_libraries(vclient
        PRIVATE
        vlib
        SDL3::SDL3
        imgui
        daxa
)

# Copy sdl dlls
if(WIN32)

    add_custom_command(
        TARGET vclient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_BINARY_DIR}/extern/sdl3/SDL3.dll"
        "$<TARGET_FILE_DIR:vclient>"
        COMMENT "Copying SDL3.dll to output directory"
    )
else()

endif()

# Set output directories
set_target_properties(vclient
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Client-specific resource management
function(find_shader_files SHADER_FILES)
    file(GLOB VERT_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.vert")
    file(GLOB FRAG_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.frag")
    set(${SHADER_FILES} ${VERT_SHADERS} ${FRAG_SHADERS} PARENT_SCOPE)
endfunction()

function(get_spv_output_path SHADER_PATH OUTPUT_PATH)
    get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
    set(SPV_PATH "${CMAKE_BINARY_DIR}/resources/shaders/${SHADER_NAME}.spv")
    set(${OUTPUT_PATH} ${SPV_PATH} PARENT_SCOPE)
endfunction()

add_custom_target(cp_resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_BINARY_DIR}/resources
)

# Make vclient depend on resource copying
add_dependencies(vclient cp_resources)

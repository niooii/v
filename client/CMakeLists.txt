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
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# VMA is header-only
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE ${CMAKE_SOURCE_DIR}/extern/vma/include)

# Find Vulkan
if(DEFINED ENV{VULKAN_SDK})
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(Vulkan_INCLUDE_DIRS "$ENV{VULKAN_SDK}/Include")
    else()
        set(Vulkan_INCLUDE_DIRS "$ENV{VULKAN_SDK}/include")
    endif()
    include_directories(${Vulkan_INCLUDE_DIRS})
    find_package(Vulkan REQUIRED)
else()
    find_package(Vulkan REQUIRED)
    include_directories(${Vulkan_INCLUDE_DIRS})
endif()

# Find all client source files
file(GLOB_RECURSE CLIENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create client executable (rename to avoid directory conflict)
add_executable(v_client ${CLIENT_SOURCES})

target_compile_definitions(v_client PUBLIC GDF_CLIENT_BUILD)

target_include_directories(v_client PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Link client-specific libraries
target_link_libraries(v_client
    PRIVATE
    v_common
    Vulkan::Vulkan
    SDL3::SDL3
    imgui
    vma
)

# Set output directories
set_target_properties(v_client
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

# Make client depend on resource copying
add_dependencies(v_client cp_resources)
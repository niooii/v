# Find all server source files
file(GLOB_RECURSE SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create server executable (rename to avoid directory conflict)
add_executable(server ${SERVER_SOURCES})

# Link server-specific libraries
target_link_libraries(server
    PRIVATE
    v_lib
)

target_include_directories(server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Set output directories
set_target_properties(server
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
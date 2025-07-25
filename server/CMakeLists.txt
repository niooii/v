# Server-specific dependencies
# Add any server-specific external libraries here
# For example, if you need networking libraries like enet:
# add_subdirectory(${CMAKE_SOURCE_DIR}/extern/enet ${CMAKE_BINARY_DIR}/extern/enet)

# Find all server source files
file(GLOB_RECURSE SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Create server executable (rename to avoid directory conflict)
add_executable(v_server ${SERVER_SOURCES})

# Link server-specific libraries
target_link_libraries(v_server
    PRIVATE
    v_common
    # Add server-specific libraries here, e.g.:
    # enet
)

# Set output directories
set_target_properties(v_server
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
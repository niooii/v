# Common/shared game logic library

# Find all source files in common
file(GLOB_RECURSE COMMON_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Create the common library
add_library(v_common STATIC ${COMMON_SOURCES})

# Include directories for common library
target_include_directories(v_common
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/extern"
        "${CMAKE_SOURCE_DIR}/extern/hfsm2/include/"
)

# Link common dependencies
target_link_libraries(v_common
        PUBLIC
        ser20
        EnTT::EnTT
        spdlog::spdlog
		absl::base
    	# absl::container 
    	absl::synchronization
    	absl::strings
)

# Set output directories
set_target_properties(v_common
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

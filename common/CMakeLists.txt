# Common/shared game logic library

# Find all source files in common
file(GLOB_RECURSE COMMON_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Create the common library
add_library(vlib STATIC ${COMMON_SOURCES}
        include/engine/sigh.h)

# Include directories for common library
target_include_directories(vlib
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/extern"
        "${CMAKE_SOURCE_DIR}/extern/hfsm2/include/"
)

# Link common dependencies
target_link_libraries(vlib
        PUBLIC
        ser20
        EnTT::EnTT
        spdlog::spdlog
        Taskflow::Taskflow
		absl::base
		absl::debugging
	 	absl::algorithm
    	absl::synchronization
    	absl::strings
)

# Set output directories
set_target_properties(vlib
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

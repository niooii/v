cmake_minimum_required(VERSION 3.20)

# Tests build after core targets

function(v_add_test NAME SRC)
    add_executable(${NAME} ${SRC})
    target_link_libraries(${NAME}
        PRIVATE
            vlib
            vclient_deps
            vserver_deps
    )
    if(COMMAND set_output_directories)
        set_output_directories(${NAME})
    endif()
endfunction()

# Auto-discover tests from directory structure
file(GLOB TEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/")

foreach(TEST_DIR ${TEST_DIRS})
    if(IS_DIRECTORY ${TEST_DIR})
        get_filename_component(TEST_NAME ${TEST_DIR} NAME)
        file(GLOB TEST_SOURCES "${TEST_DIR}/*.cpp")

        if(TEST_SOURCES)
            v_add_test(vtest-${TEST_NAME} "${TEST_SOURCES}")
        endif()
    endif()
endforeach()


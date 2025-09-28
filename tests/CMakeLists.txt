cmake_minimum_required(VERSION 3.20)

# Tests build after core targets

function(v_add_test NAME SRC)
    add_executable(${NAME} ${SRC})
    target_include_directories(${NAME}
        PUBLIC
            ${CMAKE_SOURCE_DIR}/common/include
            ${CMAKE_SOURCE_DIR}/extern
            ${CMAKE_SOURCE_DIR}/extern/hfsm2/include
    )
    target_link_libraries(${NAME}
        PRIVATE
            vlib
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
        set(TEST_MAIN_FILE "${TEST_DIR}/main.cpp")

        if(EXISTS ${TEST_MAIN_FILE})
            v_add_test(vtest-${TEST_NAME} ${TEST_MAIN_FILE})
        endif()
    endif()
endforeach()


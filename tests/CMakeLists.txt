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

v_add_test(vtest_net ${CMAKE_CURRENT_SOURCE_DIR}/net/main.cpp)
v_add_test(vtest_svo ${CMAKE_CURRENT_SOURCE_DIR}/svo/main.cpp)


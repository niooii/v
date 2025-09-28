cmake_minimum_required(VERSION 3.20)

function(v_add_experiment NAME SRC)
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

file(GLOB EXPERIMENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/")

foreach(EXPERIMENT_DIR ${EXPERIMENT_DIRS})
    if(IS_DIRECTORY ${EXPERIMENT_DIR})
        get_filename_component(EXPERIMENT_NAME ${EXPERIMENT_DIR} NAME)
        set(EXPERIMENT_MAIN_FILE "${EXPERIMENT_DIR}/main.cpp")

        if(EXISTS ${EXPERIMENT_MAIN_FILE})
            v_add_experiment(vexpr_${EXPERIMENT_NAME} ${EXPERIMENT_MAIN_FILE})
        endif()
    endif()
endforeach()


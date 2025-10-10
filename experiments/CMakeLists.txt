cmake_minimum_required(VERSION 3.20)

function(v_add_experiment NAME SRC)
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

file(GLOB EXPERIMENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/")

foreach(EXPERIMENT_DIR ${EXPERIMENT_DIRS})
    if(IS_DIRECTORY ${EXPERIMENT_DIR})
        get_filename_component(EXPERIMENT_NAME ${EXPERIMENT_DIR} NAME)
        file(GLOB EXPERIMENT_SOURCES "${EXPERIMENT_DIR}/*.cpp")

        if(EXPERIMENT_SOURCES)
            v_add_experiment(vexpr-${EXPERIMENT_NAME} "${EXPERIMENT_SOURCES}")
        endif()
    endif()
endforeach()


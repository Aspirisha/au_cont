cmake_minimum_required(VERSION 3.0)
project(aucont)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(AUC_START_SOURCE_FILES
        aucont_start.cpp
        daemon_interaction.h
        daemon_interaction.cpp
        aucont_util.h
        aucont_util.cpp
        container_command.h
        container_command.cpp)

set(AUC_EXEC_SOURCE_FILES
        aucont_util.h
        aucont_util.cpp
        aucont_exec.cpp)

add_executable(__aucont_start ${AUC_START_SOURCE_FILES})
add_executable(__aucont_exec ${AUC_EXEC_SOURCE_FILES})

message(${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/../bin)
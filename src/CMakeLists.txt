cmake_minimum_required(VERSION 3.0)
project(aucont)

find_package(Boost 1.55 REQUIRED COMPONENTS program_options)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${Boost_INCLUDE_DIRS})
set(SOURCE_FILES main.cpp)
add_executable(aucont_start ${SOURCE_FILES})
target_link_libraries(aucont_start ${Boost_LIBRARIES})

set_target_properties(aucont_start PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/../bin
        )

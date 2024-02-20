cmake_minimum_required(VERSION 3.9)
project(reshaping_demo)

# include extra application dependencies
include(FetchContent)
include(cli11)
include(eigen)

file(GLOB APP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(reshaping_demo)
target_sources(reshaping_demo PRIVATE ${APP_SOURCES})

target_link_libraries(reshaping_demo PUBLIC 
    mesh_reshaping_lib
    Eigen3::Eigen
    CLI11::CLI11
)
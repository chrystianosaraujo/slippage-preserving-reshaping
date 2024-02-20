cmake_minimum_required(VERSION 3.6)

include(glfw REQUIRED)

file(GLOB HEADERS *.h)
file(GLOB SOURCE  *.cpp)

# Create a library for the viewer code
add_library(imgui ${SOURCE})
target_include_directories(imgui PUBLIC "./")    
target_link_libraries(imgui PUBLIC glfw::glfw)

set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

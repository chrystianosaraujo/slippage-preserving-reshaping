# The following variables can be set as arguments for the module.
# - DEPENDENCIES_BASE_DIR: Root library directory of stb_image folder
#
# The following variables are exported
# - IMGUIZMO_INCLUDE_DIR 
cmake_minimum_required(VERSION 3.6)

if(TARGET imguizmo::imguizmo)
    message(STATUS "Third-party (external): reusing target 'imguizmo::imguizmo'")
    return()
endif()

message(STATUS "Third-party (external): creating target 'imguizmo::imguizmo'")

# Find include files
find_path(
		IMGUIZMO_INCLUDE_DIR
		NAMES imguizmo.h
		PATHS
		${DEPENDENCIES_BASE_DIR}
		${DEPENDENCIES_BASE_DIR}/imguizmo
		DOC "The directory where imguizmo.h resides")

set(HEADERS ${IMGUIZMO_INCLUDE_DIR}/imguizmo.h)
set(SOURCES ${IMGUIZMO_INCLUDE_DIR}/imguizmo.cpp)

add_library(imguizmo ${HEADERS} ${SOURCES})
target_include_directories(imguizmo PUBLIC ${IMGUIZMO_INCLUDE_DIR})    
target_link_libraries(imguizmo PUBLIC
    imgui::imgui
)

set_target_properties(imguizmo PROPERTIES FOLDER third_party)
add_library(imguizmo::imguizmo ALIAS imguizmo)


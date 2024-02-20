cmake_minimum_required(VERSION 3.9)

#######################################################
# Policy definitions
#######################################################
set(CMAKE_POLICY_DEFAULT_CMP0127 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
cmake_policy(SET CMP0054 NEW)

project(MeshReshaping)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(BASE_LIBS_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/")
set(DEPENDENCIES_BASE_DIR "${PROJECT_SOURCE_DIR}/deps")

#######################################################
# Available Options
#######################################################
set(RESHAPING_DEMO TRUE CACHE BOOL "Build 3D Reshaping demo" FORCE)
set(RESHAPING_APP  TRUE CACHE BOOL "Build 3D Reshaping GUI Application" FORCE)

set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(_USE_MATH_DEFINES)
add_compile_definitions(NOMINMAX)

# Remove CTEST empty VS folders
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

#######################################################
# Lagrange options
#######################################################
set(LAGRANGE_CORE          TRUE  CACHE BOOL "" FORCE)
set(LAGRANGE_IO            TRUE  CACHE BOOL "" FORCE)
set(LAGRANGE_FS            FALSE CACHE BOOL "" FORCE)
set(LAGRANGE_UI            FALSE CACHE BOOL "" FORCE)
set(LAGRANGE_UNIT_TESTS    FALSE CACHE BOOL "" FORCE)

include(FetchContent)
include(eigen)
include(lagrange)
include(nlohmann_json)

add_subdirectory("${PROJECT_SOURCE_DIR}/libs/ca_essentials")

#####################################################################
# MeshDeformer lib
#####################################################################
set(MESH_RESHAPING_SOURCE_DIR    "${PROJECT_SOURCE_DIR}/source/mesh_reshaping/")
set(MESH_RESHAPING_INCLUDE_DIR   "${PROJECT_SOURCE_DIR}/include/mesh_reshaping/")
file(GLOB MESH_RESHAPING_SOURCES "${MESH_RESHAPING_INCLUDE_DIR}/*.h"
                                 "${MESH_RESHAPING_SOURCE_DIR}/*.cpp")

add_library(mesh_reshaping_lib)
set_target_properties(mesh_reshaping_lib PROPERTIES LINKER_LANGUAGE CXX)
target_sources(mesh_reshaping_lib PRIVATE "${MESH_RESHAPING_SOURCES}")
target_include_directories(mesh_reshaping_lib PUBLIC "${BASE_LIBS_INCLUDE_DIR}")
target_link_libraries(mesh_reshaping_lib PUBLIC
	Eigen3::Eigen
    lagrange::core
    lagrange::io
    nlohmann_json::nlohmann_json
    ca_essentials
)

#######################################################
# Building Selected Applications
#######################################################
if(RESHAPING_DEMO)
	message(STATUS "3D Reshaping demo enabled")
    add_subdirectory("${PROJECT_SOURCE_DIR}/apps/reshaping_demo")
endif()

if(RESHAPING_APP)
	message(STATUS "3D Reshaping GUI application enabled")
    add_subdirectory("${PROJECT_SOURCE_DIR}/apps/reshaping_app")
endif()

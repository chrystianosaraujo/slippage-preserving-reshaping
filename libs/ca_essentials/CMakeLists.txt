cmake_minimum_required(VERSION 3.9)
project(ca_essentials)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#######################################################
# Available Options
#######################################################
set(CA_ESSENTIALS_UI       TRUE CACHE BOOL "Include GUI module" FORCE)
set(CA_ESSENTIALS_RENDERER TRUE CACHE BOOL "Include Renderer module" FORCE)

set(GLM_ROOT_DIR "${DEPENDENCIES_BASE_DIR}/glm")

include(libigl)
include(spdlog)
include(lagrange)
include(stb_image REQUIRED)
include(nfd REQUIRED)
include(glad REQUIRED)
include(glm REQUIRED)
include(glfw REQUIRED)
include(imgui REQUIRED)

set(SOURCE_DIR  "${PROJECT_SOURCE_DIR}/source")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

set(DEPENDENCIES_LIB Eigen3::Eigen
					 igl::core
					 spdlog::spdlog
					 lagrange::core
					 lagrange::io)

file(GLOB CORE_SOURCES "${INCLUDE_DIR}/ca_essentials/core/*.h"
			  			"${SOURCE_DIR}/core/*.cpp")

file(GLOB IO_SOURCES "${INCLUDE_DIR}/ca_essentials/io/*.h"
		  			 "${SOURCE_DIR}/io/*.cpp")


file(GLOB MESHES_SOURCES "${INCLUDE_DIR}/ca_essentials/meshes/*.h"
			  			 "${SOURCE_DIR}/meshes/*.cpp")

# Adding RENDERER sources if option is ON
if(CA_ESSENTIALS_RENDERER)
	file(GLOB RENDERER_SOURCES "${INCLUDE_DIR}/ca_essentials/renderer/*.h"
					  		   "${SOURCE_DIR}/renderer/*.cpp")
	set(DEPENDENCIES_LIB ${DEPENDENCIES_LIB} glad::glad opengl32)
endif()

# Adding UI sources if option is ON
if(CA_ESSENTIALS_UI)
	file(GLOB UI_SOURCES "${INCLUDE_DIR}/ca_essentials/ui/*.h"
				  		 "${SOURCE_DIR}/ui/*.cpp")

	set(DEPENDENCIES_LIB ${DEPENDENCIES_LIB} imgui::imgui nfd::nfd)
	set(INCLUDE_DIR ${INCLUDE_DIR} ${STB_IMAGE_INCLUDE_DIRS})
endif()

add_library(${PROJECT_NAME} ${CORE_SOURCES} ${IO_SOURCES} ${MESHES_SOURCES} ${RENDERER_SOURCES} ${UI_SOURCES})

target_include_directories(ca_essentials PUBLIC ${INCLUDE_DIR} ${GLM_INCLUDE_DIRS})
target_link_libraries(ca_essentials PUBLIC ${DEPENDENCIES_LIB})

add_library(ca_essentials ALIAS ${PROJECT_NAME})
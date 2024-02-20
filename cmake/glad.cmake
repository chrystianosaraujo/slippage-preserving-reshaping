cmake_minimum_required(VERSION 3.6)

if(TARGET glad::glad)
    message(STATUS "Third-party (external): reusing target 'glad::glad'")
    return()
endif()

message(STATUS "Third-party (external): creating target 'glad::glad'")

add_subdirectory("${DEPENDENCIES_BASE_DIR}/glad" "${PROJECT_BINARY_DIR}/bin/glad")

add_library(glad::glad ALIAS glad)
set_target_properties(glad PROPERTIES FOLDER third_party)

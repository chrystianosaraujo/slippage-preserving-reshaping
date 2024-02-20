if(TARGET imgui::imgui)
    message(STATUS "Third-party (external): reusing target 'imgui::imgui'")
    return()
endif()

message(STATUS "Third-party (external): creating target 'imgui::imgui'")

add_subdirectory("${DEPENDENCIES_BASE_DIR}/imgui" "${PROJECT_BINARY_DIR}/bin/imgui")

add_definitions(-UIMGUI_DISABLE_OBSOLETE_FUNCTIONS)

add_library(imgui::imgui ALIAS imgui)
set_target_properties(imgui PROPERTIES FOLDER third_party)
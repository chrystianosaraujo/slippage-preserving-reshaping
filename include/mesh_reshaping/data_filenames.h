#pragma once

#include "globals.h"

#include <filesystem>

namespace reshaping {

// Converts mesh filename to edit operation filename
std::string get_edit_operation_fn(const std::string& mesh_fn);

// Converts mesh filename to camera filename
std::string get_camera_fn(const std::string& mesh_fn);

// Converts mesh filename to straightness filename
std::string get_straightness_fn(const std::string& mesh_fn);

// Converts mesh filename to curvature filename
std::string get_curvature_fn(const std::string& mesh_fn);

}

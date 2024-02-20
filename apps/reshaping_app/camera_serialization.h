#pragma once

#include "camera.h"

#include <string>
#include <filesystem>

bool serialize_camera(const std::string& label,
                      const CameraInfo& cam,
                      const std::filesystem::path& fn);

bool serialize_cameras(const std::vector<std::string>& labels, 
                       const std::vector<CameraInfo>& cameras,
                       const std::filesystem::path& fn);

bool deserialize_cameras(const std::filesystem::path& fn,
                         std::vector<CameraInfo>& cameras,
                         std::vector<std::string>& labels);

// The first pair's value indicates whether it was possible 
// to load the camera settings with the given identifier.
std::pair<bool, CameraInfo> deserialize_camera(const std::filesystem::path &fn,
                                                const std::string &label);

bool delete_camera_from_file(const std::filesystem::path& fn,
                             const std::string& label);

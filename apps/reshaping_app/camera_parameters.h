#pragma once

#include "glm/mat3x3.hpp"
#include "glm/vec3.hpp"

// Utility class to track the parameters of a camera
// Note that these DO NOT include any particular image discretization (which
// would be measured in pixels)
class CameraParameters {
public:
  CameraParameters();

  // Extrinsic transform
  glm::mat4x4 E;

  // Intrinsics
  // glm::vec2 imageCenter;   // measured in distance, NOT pixels
  // glm::vec2 focalLengths;  // measured in distance, NOT pixels
  float fov; // in degrees

  // Get various derived quantities
  glm::vec3 T() const;
  glm::mat3x3 R() const;
  glm::vec3 position() const;
  glm::vec3 look_dir() const;
  glm::vec3 up_dir() const;
  glm::vec3 right_dir() const;

    // Print GLM matrices in nice ways
  static void pretty_print(glm::mat4x4 M);
};

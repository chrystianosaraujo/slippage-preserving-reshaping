#include "camera_parameters.h"

#include <cstdio>
#include <iostream>

#include "glm/gtc/matrix_access.hpp"

CameraParameters::CameraParameters()
: E(1.0)
, fov(60.)
{}

glm::vec3 CameraParameters::T() const {
    return glm::vec3(E[3][0], E[3][1], E[3][2]); 
}

glm::mat3x3 CameraParameters::R() const {
  glm::mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = E[i][j];
    }
  }
  return R;
}

glm::vec3 CameraParameters::position() const {
    return -transpose(R()) * T();
}

glm::vec3 CameraParameters::look_dir() const {
    return normalize(transpose(R()) * glm::vec3(0.0, 0.0, -1.0));
}

glm::vec3 CameraParameters::up_dir() const {
    return normalize(transpose(R()) * glm::vec3(0.0, 1.0, 0.0)); 
}

glm::vec3 CameraParameters::right_dir() const {
    return normalize(transpose(R()) * glm::vec3(1.0, 0.0, 0.0));
}

void CameraParameters::pretty_print(glm::mat4x4 M) {
  char buff[50];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      std::snprintf(buff, 50, "%.5f  ", M[j][i]);
      std::cout << buff;
    }
    std::cout << std::endl;
  }
}

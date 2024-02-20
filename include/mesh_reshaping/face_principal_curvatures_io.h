#pragma once

#include <string>
#include <Eigen/Core>

namespace reshaping {

bool load_face_principal_curvature_values(const std::string& fn,
                                          Eigen::VectorXd& F_PV1,
                                          Eigen::VectorXd& F_PV2);

bool save_face_principal_curvature_values(const std::string& fn,
                                          const Eigen::VectorXd& F_PV1,
                                          const Eigen::VectorXd& F_PV2);

}
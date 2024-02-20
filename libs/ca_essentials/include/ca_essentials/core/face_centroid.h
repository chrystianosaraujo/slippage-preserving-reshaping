#pragma once

#include <Eigen/Core>

namespace ca_essentials {
namespace core {

Eigen::Vector3d face_centroid(const Eigen::MatrixXd& V,
                              const Eigen::MatrixXi& F,
                              int f);

}
}
#pragma once

#include <Eigen/Core>

namespace ca_essentials {
namespace core {

// Computes a new mesh with vertex positions duplicated for each triangle
void compute_flat_mesh(const Eigen::MatrixXd& V,
                       const Eigen::MatrixXi& T,
                       Eigen::MatrixXd& OV,
                       Eigen::MatrixXi& OT,
                       Eigen::MatrixXd& ON);

}
}

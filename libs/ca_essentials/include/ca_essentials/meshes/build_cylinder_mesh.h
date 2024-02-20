#pragma once

#include <Eigen/Core>

namespace ca_essentials {
namespace meshes {

void build_cylinder_mesh(double radius, double height,
                         Eigen::MatrixXd& V,
                         Eigen::MatrixXi& F,
                         int n_radial_segments = 10,
                         int n_height_segments = 4,
                         bool add_caps = true);

}
}

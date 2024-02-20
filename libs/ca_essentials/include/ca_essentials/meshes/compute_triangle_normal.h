#pragma once

#include <ca_essentials/meshes/trimesh.h>

#include <Eigen/Core>

namespace ca_essentials {
namespace meshes {

Eigen::MatrixXd compute_triangle_normal(const Eigen::MatrixXd& V,
                                        const Eigen::MatrixXi& F);

Eigen::MatrixXd compute_triangle_normal(const TriMesh& mesh);

}
}

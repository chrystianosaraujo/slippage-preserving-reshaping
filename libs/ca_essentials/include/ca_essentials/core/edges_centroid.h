#pragma once

#include <Eigen/Core>
#include <ca_essentials/meshes/trimesh.h>

namespace ca_essentials {
namespace core {

Eigen::MatrixXd edges_centroid(const ca_essentials::meshes::TriMesh& mesh,
                               const Eigen::MatrixXd& V);

}
}
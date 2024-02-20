#pragma once

#include <ca_essentials/meshes/trimesh.h>
#include <string>

namespace ca_essentials {
namespace meshes {

bool save_trimesh(const std::string& fn, const TriMesh& mesh);

bool save_trimesh(const std::string& fn, const Eigen::MatrixXd& V, const Eigen::MatrixXi& F);

}
}


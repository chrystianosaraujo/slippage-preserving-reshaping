#pragma once

#include <Eigen/Core>

namespace ca_essentials {
namespace meshes {
    
// Normalize the given vertex list to the unit box [0..1]^3
void normalize_to_unitbox(Eigen::MatrixXd& V);

// Normalize all meshes to the unit box [0..1]^3 based on the 1st mesh's bbox
void normalize_to_unitbox(std::vector<Eigen::MatrixXd>& meshes_V);

}
}

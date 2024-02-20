#pragma once

#include <ca_essentials/meshes/Trimesh.h>

#include <vector>

namespace ca_essentials {
namespace meshes {

inline Eigen::VectorXd
compute_mesh_dihedral_angles(const ca_essentials::meshes::TriMesh& mesh,
                             const Eigen::MatrixXd& F_normals) {

    const int num_edges = mesh.get_num_edges();

    Eigen::VectorXd angles(num_edges);

    const auto& adj_e2f = mesh.get_edge_face_adjacency();
    for(int e = 0; e < num_edges; ++e) {
        const Eigen::Vector3d& n1 = F_normals.row(adj_e2f.at(e)[0]);
        const Eigen::Vector3d& n2 = F_normals.row(adj_e2f.at(e)[1]);

        double dot = n1.dot(n2);
        dot = std::max(-1.0, std::min(dot, 1.0));

        double dihedral_angle = M_PI - acos(dot);
        angles(e) = dihedral_angle;
    }

    return angles;
}

}
}
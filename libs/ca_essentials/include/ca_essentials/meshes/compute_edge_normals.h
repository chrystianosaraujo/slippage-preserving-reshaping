#pragma once

#include <ca_essentials/meshes/compute_triangle_normal.h>

#include <Eigen/Core>

namespace ca_essentials {
namespace meshes {

// It simply averages the normal of the two adjacent triangles.
// This must be only be used for debugging purposes.
inline Eigen::MatrixXd compute_edge_normals(const Eigen::MatrixXd& V,
                                            const Eigen::MatrixXi& F,
                                            const std::vector<std::vector<int>>& adj_e2f) {
    Eigen::MatrixXd tri_normals = compute_triangle_normal(V, F);
        
    int num_edges = (int) adj_e2f.size();
    Eigen::MatrixXd edge_normals(num_edges, 3);
    for(int i = 0; i < num_edges; ++i) {
        edge_normals.row(i) = Eigen::Vector3d::Zero();

        for(int fid : adj_e2f.at(i))
            edge_normals.row(i) += tri_normals.row(fid);
    }

    for(int i = 0; i < num_edges; ++i)
        edge_normals.row(i).normalize();

    return edge_normals;
}

}
}

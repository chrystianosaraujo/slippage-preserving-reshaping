#pragma once

#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/reshaping_data.h>

namespace reshaping {

void compute_length_based_edge_weights(const reshaping::TriMesh& mesh,
                                       const double avg_edge_len,
                                       const Eigen::VectorXd& curr_edge_lens,
                                       Eigen::VectorXd& weights) {
    namespace meshes = ca_essentials::meshes;

    const int num_edges = mesh.get_num_edges();

    if(weights.rows() == 0) 
        weights.resize(num_edges);

    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        double curr_len = curr_edge_lens(e);
        weights(e) = std::max(1.0, pow(curr_len / avg_edge_len, 2.0));
    }
}

}
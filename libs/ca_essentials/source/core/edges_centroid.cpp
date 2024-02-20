#include <ca_essentials/core/edges_centroid.h>

namespace ca_essentials {
namespace core {

Eigen::MatrixXd edges_centroid(const ca_essentials::meshes::TriMesh& mesh,
                               const Eigen::MatrixXd& V) {
    const int num_edges = mesh.get_num_edges();

    Eigen::MatrixXd centroids(num_edges, 3);
    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        const Eigen::Vector3d& v0 = V.row(edge_verts[0]);
        const Eigen::Vector3d& v1 = V.row(edge_verts[1]);

        centroids.row(e) = (v0 + v1) / 2.0;
    }

    return centroids;
}

}
}
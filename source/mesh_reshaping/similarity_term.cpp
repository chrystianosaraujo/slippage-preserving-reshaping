#include <mesh_reshaping/similarity_term.h>

#include <ca_essentials/meshes/dihedral_angles.h>

#include <vector>

namespace reshaping {

void compute_similarity_term_edge_weights(const reshaping::TriMesh& mesh,
                                          const Eigen::MatrixXd& tri_N,
                                          double sigma,
                                          double angle_cutoff_tol,
                                          Eigen::VectorXd& weights) {
    namespace meshes = ca_essentials::meshes;

    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    Eigen::VectorXd dihedral_angles = meshes::compute_mesh_dihedral_angles(mesh, tri_N);
    weights.resize(num_edges);

    for(int e = 0; e < num_edges; ++e) {
        const double angle = dihedral_angles(e);

        double w = 0.0;
        if(angle > angle_cutoff_tol)
            w = exp(-pow(angle - M_PI, 2.0) /
                         (2.0 * pow(sigma, 2.0)));

        weights(e) = w;
    }
}

}

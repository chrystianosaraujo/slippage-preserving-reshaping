#pragma once

#include <mesh_reshaping/types.h>

#include <Eigen/Core>

namespace reshaping {

void compute_similarity_term_edge_weights(const reshaping::TriMesh& mesh,
                                          const Eigen::MatrixXd& tri_N,
                                          double sigma,
                                          double angle_cutoff_tol,
                                          Eigen::VectorXd& weights);

}
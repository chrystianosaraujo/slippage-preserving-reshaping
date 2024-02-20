#pragma once

#include <mesh_reshaping/reshaping_energy.h>
#include <mesh_reshaping/termination_criterion.h>
#include <mesh_reshaping/straight_chains.h>
#include <mesh_reshaping/sphericity_terms_info.h>
#include <ca_essentials/meshes/trimesh.h>

#include <Eigen/Geometry>
#include <Eigen/Sparse>

#include <vector>
#include <array>
#include <unordered_map>

namespace reshaping {

struct ReshapingData {
    // Enforcing a valid triangle mesh always
    ReshapingData(const ca_essentials::meshes::TriMesh& m)
    : mesh(m) {

    }

    // Input surface mesh
    const ca_essentials::meshes::TriMesh& mesh;

    // Optional input straight chains
    const StraightChains* straight_chains = nullptr;

    // Per-face principal curvature values
    Eigen::VectorXd PV1;
    Eigen::VectorXd PV2;

    // Boundary conditions
    // <key: vertex index, value: target position>
    std::unordered_map<int, Eigen::Vector3d> bc;

    // Pre-computed sphericity terms info
    std::vector<SphericityTermInfo> sphericity_terms_info;

    // Number of straight pairs
    // TODO: document it
    int num_straight_pairs = 0;

    // Initial average edge length 
    double avg_edge_len = 0.0;

    // Input and current vertices
    Eigen::MatrixXd orig_vertices;
    Eigen::MatrixXd curr_vertices;

    // Input edge vectors
    Eigen::MatrixXd orig_edges;

    // Initial and current per-triangle normals
    Eigen::MatrixXd orig_tri_N;
    Eigen::MatrixXd curr_tri_N;

    // For each edge, it stores the input normal of the two adjacent faces
    // Note: supporting manifold meshes only for now
    std::vector<std::array<Eigen::Vector3d, 2>> orig_edge_adj_N;

    // Per-triangle current and previous transformations (T_i)
    std::vector<Eigen::Matrix3d> curr_tri_T;
    std::vector<Eigen::Matrix3d> prev_tri_T;

    // Per-edge initial and target lengths
    Eigen::VectorXd orig_edge_lens;
    Eigen::VectorXd curr_edge_lens;
    Eigen::VectorXd target_edge_lens;

    // Per-edge similarity term weight - based on dihedral angles.
    Eigen::VectorXd similarity_term_edge_w;

    // Per-edge length-based weights (max(1.0, l_ij/L_avg))
    Eigen::VectorXd length_based_edge_w;

    ////////////////////////////////////////////////////////
    // Handle error distribution parameters
    // See Appendix A (Finalizing Outputs) for more details
    ////////////////////////////////////////////////////////

    // Stores the set of vertices outsize the zone of handle error
    // distribution.
    std::unordered_set<int> handle_error_dist_out_verts;

    // Current iteration
    int iter = 0;

    // Iteration where the best solution was found
    int best_sol_iter = -1;
    Eigen::MatrixXd best_sol;

    // Indicates all the termination criteria reached
    TerminationCriterion termination_type;

    // Vertex Position solve and status of the last solve call
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> vertex_solver;
    bool last_vertex_sol_succ = false;

    // Tranformation solver and status of the last solve call
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> transf_solver;
    bool last_transf_sol_succ = false;

    // Time measurements
    double total_time = 0.0;
    double avg_iter_time = 0.0;

    /**************************************************
     * Debug options and information only
     * TODO: add DEBUG flag
     **************************************************/
    std::vector<Eigen::MatrixXd> iter_vertex_sol;
    std::vector<ReshapingEnergy> iter_energy_costs;
    std::vector<double> iter_energy_delta;
    std::vector<double> iter_max_vertex_change;
    std::vector<double> iter_max_positional_constr_dist;
};

}

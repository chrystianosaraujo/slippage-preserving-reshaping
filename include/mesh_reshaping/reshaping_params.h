#pragma once

#include <mesh_reshaping/straight_chains.h>
#include <ca_essentials/core/geom_utils.h>

#include <Eigen/Core>

#include <filesystem>
#include <unordered_map>
#include <string>

namespace reshaping {

// Handle-error distribution parameters
// 
// See details in Appendix A (Finalizing Outputs)
struct HandleErrorDistribParams {
    // Indicates whether handle error distribution is active
    bool is_active = false;

    // Size (in number of rings) of the zone around handles where
    // the error distribution is performed
    int num_rings = 4;

    // Weight used to hold in place all vertices outside the distribution zone 
    double off_zone_hc_weight = 1e5;

    // Weakened constraint weight 
    double weakened_hc_weight = 1e2;

    // A vertex is flagged as requiring error distribution if the error at the
    // first ring is greater than the error at the last ring times this factor
    double inner_ring_error_tol = 2.0;

    // A vertex is flagged as requiring error distribution if the minimum error around it
    // is greater than this value
    double min_error_tol = ca_essentials::core::to_radians(10.0);
};

struct VertexSolveParams {
    double normal_weight = 10.0;
    double edge_weight = 1.0;
    double sphericity_weight = 1.0;
    double sphericity_k_max_const = 2.0;
    double straightness_weight = 1e5;
    double regularizer_weight = 1e-4;
    double bc_weight = 1e5;

    HandleErrorDistribParams handle_error_distrib;
};

struct TransfSolveParams {
    const double connect_weight = 1.0;
    const double normal_weight = 10.0;
    const double regularizer_weight = 1e-4;
    const double similarity_weight = 1.0;
    const double sphericity_term_weight = 1.0;

    const double similarity_sigma = M_PI / 6.0;
    const double similarity_cutoff_angle = ca_essentials::core::to_radians(90.0 + 15.0);
};

struct ReshapingParams {
    VertexSolveParams vertex_sol;
    TransfSolveParams transf_sol;

    /////////////////////////////////////////////
    // Termination criteria parameters
    /////////////////////////////////////////////
    int max_iters = 100;
    double hc_weight = 1e5;

    // Stops if: energy_delta < min_delta_tol * #edges
    double min_delta_tol = 1.0/100.0;

    // Stops if: max_vertex_change < avg_edge_len * max_vertex_change_tol
    // TODO: rename it
    double max_vertex_change_tol = 1.0/100.0;

    /////////////////////////////////////////////
    // Optimization options
    /////////////////////////////////////////////
    bool handle_error_distrib_enabled = false;

    /////////////////////////////////////////////
    // Debugging paramaters
    /////////////////////////////////////////////

    // Optional parameter used for exporting debugging files
    std::filesystem::path debug_folder;

    // Defines the frequency (in iterations) at which new vertex solutions are exported to OBJ files.
    int export_objs_every_n_iter = 10;

    // Optional parameter user for debugging only
    std::string input_name;
};

}

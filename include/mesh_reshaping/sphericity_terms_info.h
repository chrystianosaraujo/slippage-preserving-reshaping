#pragma once

#include <mesh_reshaping/types.h>
#include <Eigen/Geometry>
#include <vector>
#include <cmath>

namespace reshaping {

struct SphericityTermInfo {
    // Face index
    int fid  = 0;

    // vid_i is the center vertex shared by the two triangle edges
    int vid_i = 0;
    int vid_j = 0;
    int vid_k = 0;

    // Term weight
    double w = 0.0;

    // (vid_j - vid_i) =  ratio * (vid_k - vid_i)
    double ratio = 0.0;

    // Rotates (vid_k - vid_i) into (vid_j - vid_i) 
    Eigen::Matrix3d R;
};

// Check slippage-preserving paper for parameter details
struct SphericityTermsParams {
    double sigma1 = 0.2;
    double sigma2 = 0.3;
    double epsilon = 1e-3;
    double k_max_const = 2.0;
    double feature_angle_tol = 140.0 * M_PI / 180.0;

    std::string debug_folder="";
};

// Computes the sphericity terms for the given mesh
//
// Parameters:
//      F_PV1, F_PV2: per-face maximum and minimum curvature values
std::vector<SphericityTermInfo>
compute_sphericity_terms_info(const TriMesh& mesh,
                              const Eigen::VectorXd& F_PV1,
                              const Eigen::VectorXd& F_PV2,
                              const SphericityTermsParams& params);

}

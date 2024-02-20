#pragma once

#include <mesh_reshaping/reshaping_params.h>

namespace reshaping {
    struct ReshapingData;
}

namespace reshaping {

// Each term cost stores the accumulated cost (unweighted) of each term pair 
//
// NOTE: total_cost encodes the weighted sum of all terms
struct TransfSolveEnergy {
    double transform_cost = 0.0;
    double connect_cost = 0.0;
    double normal_cost = 0.0;
    double sphericity_cost = 0.0;
    double similarity_cost = 0.0;
    double regularizer_cost = 0.0;
    double total_cost = 0.0;
};

TransfSolveEnergy compute_transf_solve_energy(const TransfSolveParams& params,
                                              const ReshapingData& data,
                                              const Eigen::MatrixXd& V);

}
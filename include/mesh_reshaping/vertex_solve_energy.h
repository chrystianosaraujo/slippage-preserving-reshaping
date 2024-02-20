#pragma once

#include <mesh_reshaping/reshaping_params.h>

namespace reshaping {
    struct ReshapingData;
}

namespace reshaping {

// Each term cost stores the accumulated cost (unweighted) of each term pair 
//
// NOTE: total_cost encodes the weighted sum of all terms
struct VertexSolveEnergy {
    double normal_cost = 0.0;
    double edge_cost = 0.0;
    double straight_cost = 0.0;
    double sphericity_cost = 0.0;
    double constraints_cost = 0.0;
    double scale_cost = 0.0;
    double total_cost = 0.0;
};

VertexSolveEnergy compute_vertex_solve_energy(const VertexSolveParams& params,
                                              const ReshapingData& data,
                                              const Eigen::MatrixXd& V);

}
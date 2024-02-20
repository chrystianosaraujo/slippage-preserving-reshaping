#pragma once

#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/vertex_solve_energy.h>
#include <mesh_reshaping/transformation_solve_energy.h>

namespace reshaping {
    struct ReshapingData;
}

namespace reshaping {

struct ReshapingEnergy {
    VertexSolveEnergy vertex_sol;
    TransfSolveEnergy transf_sol;

    double total_cost = 0.0;
};

ReshapingEnergy compute_reshaping_energy(const ReshapingParams& params,
                                         const ReshapingData& data,
                                         const Eigen::MatrixXd& V);

}
#include <mesh_reshaping/reshaping_energy.h>

#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/vertex_solve_energy.h>
#include <mesh_reshaping/transformation_solve_energy.h>

namespace reshaping {

ReshapingEnergy compute_reshaping_energy(const ReshapingParams& params,
                                         const ReshapingData& data,
                                         const Eigen::MatrixXd& V) {
    ReshapingEnergy energy;
    energy.vertex_sol = compute_vertex_solve_energy(params.vertex_sol, data, V);
    energy.transf_sol = compute_transf_solve_energy(params.transf_sol, data, V);

    energy.total_cost = energy.vertex_sol.total_cost +
                        energy.transf_sol.total_cost;

    return energy;
}

}
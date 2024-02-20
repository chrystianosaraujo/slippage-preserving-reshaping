#include <mesh_reshaping/reshaping_tool.h>

#include <mesh_reshaping/globals.h>
#include <mesh_reshaping/types.h>
#include <mesh_reshaping/reshaping_energy.h>
#include <mesh_reshaping/vertex_solve.h>
#include <mesh_reshaping/transformation_solve.h>
#include <mesh_reshaping/reshaping_tool_io.h>
#include <mesh_reshaping/length_based_edge_weight.h>
#include <mesh_reshaping/compute_max_vertex_change.h>
#include <mesh_reshaping/handle_error_distribution.h>

#include <ca_essentials/meshes/compute_triangle_normal.h>
#include <ca_essentials/meshes/debug_utils.h>
#include <ca_essentials/core/timer.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {

void solve_for_vertex_positions(const reshaping::VertexSolveParams& params,
                                reshaping::ReshapingData& data) {

    bool solve_succ = reshaping::solve_for_vertices(params,
                                                    data,
                                                    data.curr_vertices);
    data.last_vertex_sol_succ = solve_succ;
}

void solve_for_transformations(const reshaping::TransfSolveParams& params,
                              reshaping::ReshapingData& data) {

    bool solve_succ = reshaping::solve_for_transformations(params,
                                                           data,
                                                           data.curr_tri_T);
    data.last_transf_sol_succ = solve_succ;
}

void update_current_edge_lengths(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const int num_edges = mesh.get_num_edges();

    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        double len = (data.curr_vertices.row(edge_verts[1]) -
                      data.curr_vertices.row(edge_verts[0])).norm();
        data.curr_edge_lens(e) = len;
    }
}

void update_target_edge_lenghts(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        int fid_0 = adj_e2f.at(e).at(0);
        int fid_1 = adj_e2f.at(e).at(1);

        const Eigen::Vector3d& orig_E = (data.orig_vertices.row(edge_verts[1]) -
                                         data.orig_vertices.row(edge_verts[0]));

        const Eigen::Matrix3d& T0 = data.curr_tri_T.at(fid_0);
        const Eigen::Matrix3d& T1 = data.curr_tri_T.at(fid_1);

        const Eigen::Vector3d T0_E = T0 * orig_E;
        const Eigen::Vector3d T1_E = T1 * orig_E;
        data.target_edge_lens(e) = (T0_E.norm() + T1_E.norm()) * 0.5;
    }
}

double compute_total_energy_delta(const reshaping::ReshapingData& data) {
    auto num_energies = data.iter_energy_costs.size();
    if(num_energies <= 1)
        return 0.0;
    else {
        auto num_energies = data.iter_energy_costs.size();
        return data.iter_energy_costs.at(num_energies - 2).total_cost -
               data.iter_energy_costs.at(num_energies - 1).total_cost;
    }
}

void compute_iteration_info(const reshaping::ReshapingParams& params,
                            reshaping::ReshapingData& data) {
    // New vertices
    data.iter_vertex_sol.push_back(data.curr_vertices);

    // Energy costs
    reshaping::ReshapingEnergy energy = reshaping::compute_reshaping_energy(params,
                                                                            data,
                                                                            data.curr_vertices);
    data.iter_energy_costs.push_back(energy);

    // Compute energy delta
    double energy_delta = compute_total_energy_delta(data);
    data.iter_energy_delta.push_back(energy_delta);

    // Maximum vertex change
    {
        const Eigen::MatrixXd& prev_V = data.iter == 0 ? data.orig_vertices
                                                       : data.iter_vertex_sol.at(data.iter - 1);

        double max_vertex_change = reshaping::compute_max_vertex_change(prev_V, data.curr_vertices);
        data.iter_max_vertex_change.push_back(max_vertex_change);
    }

    LOGGER.info("iter {:3}: energy {:.8}   energy_delta {:.8}   max_vertex_change {:.8}",
                data.iter,
                energy.total_cost,
                energy_delta,
                data.iter_max_vertex_change.back());

    //LOGGER.info("iter {:3}: Transf. {:.8}   Vertex {:.8}   Total {:.8}   Delta {:.8}   Max. Change {:.8}",
    //            data.iter,
    //            energy.transf_sol.total_cost,
    //            energy.vertex_sol.total_cost,
    //            energy.total_cost,
    //            energy_delta,
    //            data.iter_max_vertex_change.back());
}

void update_current_tri_normals(reshaping::ReshapingData& data) {
    namespace meshes = ca_essentials::meshes;

    const auto& mesh = data.mesh;
    data.curr_tri_N = meshes::compute_triangle_normal(data.curr_vertices,
                                                      mesh.get_facets());
}

void update_length_based_edge_weights(reshaping::ReshapingData& data) {
    reshaping::compute_length_based_edge_weights(data.mesh,
                                                 data.avg_edge_len,
                                                 data.curr_edge_lens,
                                                 data.length_based_edge_w);
}

void update_convergence(const reshaping::ReshapingParams& params,
                        reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const int num_edges = mesh.get_num_edges();

    if(data.iter <= 1)
        return;

    const auto& iter_info    = data.iter_energy_costs.back();
    double energy_delta      = data.iter_energy_delta.back();
    double max_vertex_change = data.iter_max_vertex_change.back();

    // checking if max. vertex change criterion was reached
    double vertex_change_tol = params.max_vertex_change_tol * data.avg_edge_len;
    bool max_vertex_change_reached = max_vertex_change < vertex_change_tol;

    // checking if last energy delta is small enough to stop
    const double delta_tol = params.min_delta_tol / num_edges;
    bool energy_delta_reached = energy_delta < delta_tol;

    // determines whether the negative energy delta criterion was reached
    bool neg_delta_reached = data.iter > 1 && energy_delta <= 0.0;

    // determines whether the max. number of iterations criterion was reached
    bool max_iter_reached = data.iter >= (params.max_iters - 1);

    bool converged = neg_delta_reached ||
                     energy_delta_reached ||
                     max_vertex_change_reached ||
                     max_iter_reached;

    LOGGER.debug("Iter {} termination:  NEG_DELTA({:.10f}, {})  MAX_CHANGE({:.10f}, {:10f}, {})  DELTA({:.10f}, {:10f}, {})",
                data.iter,
                energy_delta,
                neg_delta_reached,
                max_vertex_change,
                vertex_change_tol,
                max_vertex_change_reached,
                energy_delta,
                delta_tol,
                energy_delta_reached);

    if(converged) {
        data.termination_type.set_criteria(max_vertex_change_reached,
                                           neg_delta_reached,
                                           energy_delta_reached,
                                           max_iter_reached);
    }
}

void update_best_solution(reshaping::ReshapingData& data) {
    // Only updates if energy gets improved
    if(data.iter == 0 || data.iter_energy_delta.back() > 0.0) {
        LOGGER.debug("Best solution was updated with delta energy = {:.10f}",
                     data.iter_energy_delta.back());

        data.best_sol = data.curr_vertices;
        data.best_sol_iter = data.iter;
    }
    else {
        LOGGER.debug("Best solution was NOT updated due to negative delta energy = {:.10f}",
                     data.iter_energy_delta.back());
    }
}

void update_final_solution(reshaping::ReshapingData& data) {
    LOGGER.debug("Setting stage best solution from iteration {}", data.best_sol_iter);
    data.curr_vertices = data.best_sol;
}

Eigen::MatrixXd compute_reshaping_solution(const reshaping::ReshapingParams& params,
                                           reshaping::ReshapingData& data) {
    ca_essentials::core::Timer timer;
    timer.start("reshaping");

    // Iterate until convergence
    do {
        timer.start("iter");

        solve_for_vertex_positions(params.vertex_sol, data);
        update_current_edge_lengths(data);

        compute_iteration_info(params, data);

        update_convergence(params, data);
        update_best_solution(data);

        if(!data.termination_type.has_converged()) {
            update_current_tri_normals(data);
            update_length_based_edge_weights(data);

            solve_for_transformations(params.transf_sol, data);
            update_target_edge_lenghts(data);
        }

        double iter_duration = timer.elapsed("iter");
        data.avg_iter_time += iter_duration;

        // Save output mesh for the current iteration
        //if(data().iter < 10 ||
        //   ((data().iter + 1) % m_params->output_objs_every_n_iter == 0))
        //    save_optimization_iter_solution(curr_vertices(), m_trimesh->mesh->get_facets(),
        //                                    (int) m_curr_stage, data().iter, m_params->debug_folder.string());

        LOGGER.debug("Iter {}: elapsed {:.2f} s", data.iter, iter_duration / 1000.0);

        if(data.termination_type.has_converged())
            break;
    
        data.iter++;
    } while(true);

    update_final_solution(data);

    LOGGER.info("Optimization terminated: {}", data.termination_type.to_string());

    if(params.handle_error_distrib_enabled) {
        LOGGER.info("Performing handle-error distribution...");
        reshaping::perform_handle_error_distribution(params, data, data.curr_vertices);
    }

    data.avg_iter_time /= (data.iter + 1);
    data.total_time = timer.elapsed("reshaping");

    return data.curr_vertices;
}

}

namespace reshaping {

Eigen::MatrixXd reshaping_solve(const ReshapingParams& params,
                                ReshapingData& data) {
    const auto& mesh = data.mesh;

#if SPHERICITY_ON
    const int num_faces = mesh.get_num_facets();
    if(data.PV1.rows() != num_faces || data.PV2.rows() != num_faces) {
        LOGGER.error("Face curvature values were not provided." 
                     "3DReshapingTool cannot be used without this information when sphericity is enabled.");
        return Eigen::MatrixXd();
    }
#endif

    bool is_error_distrib_enabled = params.handle_error_distrib_enabled;
    LOGGER.info("Slippage-Preserving Reshaping... begin");
    LOGGER.debug("    Handle-error distribution ON: {}", is_error_distrib_enabled);

    Eigen::MatrixXd outV = compute_reshaping_solution(params, data);

    LOGGER.info("Slippage-Preserving Reshaping... done ({:.2f} s)", data.total_time / 1000.0);
    return outV;
}

}

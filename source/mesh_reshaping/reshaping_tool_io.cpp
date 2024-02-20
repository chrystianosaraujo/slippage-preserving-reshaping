#include <mesh_reshaping/reshaping_tool_io.h>
#include <mesh_reshaping/reshaping_tool.h>
#include <mesh_reshaping/termination_criterion.h>
#include <mesh_reshaping/globals.h>

#include <ca_essentials/core/logger.h>
#include <ca_essentials/meshes/save_trimesh.h>
#include <ca_essentials/meshes/debug_utils.h>

#include <igl/bounding_box_diagonal.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>

namespace reshaping {

bool save_mesh(const Eigen::MatrixXd& V,
               const Eigen::MatrixXi& F,
               const std::string& out_dir,
               const std::string& res_name,
               const std::string& suffix) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;
    
    auto fn = fs::path(out_dir) / (res_name + "_" + suffix + ".obj");
    return meshes::save_trimesh(fn.string(), V, F);
}

bool save_mesh(const TriMesh& mesh,
               const std::string& out_dir,
               const std::string& res_name,
               const std::string& suffix) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;
    
    auto fn = fs::path(out_dir) / (res_name + "_" + suffix + ".obj");
    return meshes::save_trimesh(fn.string(), mesh);
}

bool save_edit_operation_to_obj(const reshaping::EditOperation& edit_op,
                                const Eigen::MatrixXd& V,
                                const Eigen::MatrixXi& T,
                                const std::string& out_dir,
                                const std::string& res_name,
                                const double zero_tol) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

    const double bbox_diag = igl::bounding_box_diagonal(V);

    std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> segments;
    std::vector<Eigen::Vector3d> fixed_points;

    int vert_i = 0;
    for(const auto& [vid, disp] : edit_op.displacements) {
        const Eigen::Vector3d& orig_pos = V.row(vid);
        const Eigen::Vector3d disp_pos  = reshaping::displacement_to_abs_position(orig_pos,
                                                                                  disp, bbox_diag);

        if((orig_pos - disp_pos).norm() <= zero_tol * bbox_diag)
            fixed_points.push_back(orig_pos);
        else
            segments.emplace_back(orig_pos, disp_pos);
    }

    fs::path disp_fn  = fs::path(out_dir) / (res_name + "_handles.obj");
    fs::path fixed_fn = fs::path(out_dir) / (res_name + "_fixed_points.obj");

    return meshes::save_segments_to_obj(disp_fn.string(), segments) &&
           meshes::save_points_to_obj  (fixed_fn.string(), fixed_points);
}


bool save_optimization_iter_solution(const Eigen::MatrixXd& V,
                                     const Eigen::MatrixXi& F,
                                     int iter,
                                     const std::string& out_dir) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

    fs::path fn = fs::path(out_dir) / (std::string("output_") +
                                       "_iter_" +
                                       std::to_string(iter) + ".obj");

    return meshes::save_trimesh(fn.string(), V, F);
}

bool save_optimization_info_to_json(const ca_essentials::meshes::TriMesh& mesh,
                                    const reshaping::ReshapingData& opt_data,
                                    const reshaping::ReshapingParams& opt_params,
                                    const std::string& out_dir,
                                    const std::string& res_name) {
    namespace core = ca_essentials::core;
    namespace fs = std::filesystem;
    using json = nlohmann::json;

    json opt_json = {};

    const int num_edges = (int) opt_data.orig_edges.rows();
    const int num_straight_pairs = opt_data.num_straight_pairs;
    const int num_sphericity_pairs = (int) opt_data.sphericity_terms_info.size();

    int num_iters = (int) opt_data.iter_energy_costs.size();
    const auto& final_iter = opt_data.iter_energy_costs.back();

    opt_json["num_verts"]            = mesh.get_num_vertices();
    opt_json["num_edges"]            = mesh.get_num_edges();
    opt_json["num_tris"]             = mesh.get_num_facets();
    opt_json["num_iters"]            = num_iters;
    opt_json["avg_iter_time"]        = opt_data.avg_iter_time;
    opt_json["total_time"]           = opt_data.total_time;
    opt_json["num_straight_pairs"]   = num_straight_pairs;
    opt_json["num_sphericity_pairs"] = num_sphericity_pairs;
    opt_json["termination_type"]     = opt_data.termination_type.to_string();

    // Final sub-energies
    {
        const auto& vs_costs = final_iter.vertex_sol; 
        const auto& ts_costs = final_iter.transf_sol; 

        opt_json["vs_energy"]              = vs_costs.total_cost;
        opt_json["vs_normal_energy"]       = vs_costs.normal_cost;
        opt_json["vs_edge_energy"]         = vs_costs.edge_cost;
        opt_json["vs_sphericity_energy"]   = vs_costs.sphericity_cost;
        opt_json["vs_straightness_energy"] = vs_costs.straight_cost;
        opt_json["vs_constraints_energy"]  = vs_costs.constraints_cost;
        opt_json["ts_energy"]              = ts_costs.total_cost;

        opt_json["ts_tarnsform_energy"]    = ts_costs.transform_cost;
        opt_json["ts_connect_energy"]      = ts_costs.connect_cost;
        opt_json["ts_normal_energy"]       = ts_costs.normal_cost;
        opt_json["ts_sphericity_energy"]   = ts_costs.sphericity_cost;
        opt_json["ts_similarity_energy"]   = ts_costs.similarity_cost,
        opt_json["ts_regularizer_energy"]  = ts_costs.regularizer_cost,

        opt_json["total_energy"]           = final_iter.total_cost;
        opt_json["delta_energy"]           = opt_data.iter_energy_delta.back();
        opt_json["max_vertex_change"]      = opt_data.iter_max_vertex_change.back();
    }

    // Export per-iter info
    opt_json["per_iter"] = {};
    {
        json stage_json = {};
        for(int i = 0; i < num_iters; ++i) {
            const auto& iter = opt_data.iter_energy_costs.at(i);

            const auto& vs_costs = iter.vertex_sol; 
            const auto& ts_costs = iter.transf_sol; 

            stage_json.push_back({
                {"vs_energy"             , vs_costs.total_cost},
                {"vs_normal_energy"      , vs_costs.normal_cost},
                {"vs_edge_energy"        , vs_costs.edge_cost},
                {"vs_sphericity_energy"  , vs_costs.sphericity_cost},
                {"vs_straightness_energy", vs_costs.straight_cost},
                {"vs_constraints_energy" , vs_costs.constraints_cost},

                {"ts_energy"             , ts_costs.total_cost},
                {"ts_tarnsform_energy"   , ts_costs.transform_cost},
                {"ts_connect_energy"     , ts_costs.connect_cost},
                {"ts_normal_energy"      , ts_costs.normal_cost},
                {"ts_sphericity_energy"  , ts_costs.sphericity_cost},
                {"ts_similarity_energy"  , ts_costs.similarity_cost},
                {"ts_regularizer_energy" , ts_costs.regularizer_cost},

                {"total_energy"          , iter.total_cost},
                {"delta_energy"          , opt_data.iter_energy_delta.at(i)},
                {"max_vertex_change"     , opt_data.iter_max_vertex_change.at(i)},
            });
        }

        opt_json["per_iter"].push_back(stage_json);
    }

    // Optimization params
    opt_json["opt_params"] = {};
    opt_json["opt_params"]["bc_weight"] = opt_params.vertex_sol.bc_weight;
    opt_json["opt_params"]["max_vertex_change_tol"] = opt_params.max_vertex_change_tol;
    opt_json["opt_params"]["delta_energy_tol"] = opt_params.min_delta_tol;
    opt_json["opt_params"]["avg_edge_len"] = opt_data.avg_edge_len;

    const auto& vs_params = opt_params.vertex_sol;
    opt_json["opt_params"]["vs_straight_weight"] = vs_params.straightness_weight;
    opt_json["opt_params"]["vs_edge_weight"] = vs_params.edge_weight;
    opt_json["opt_params"]["vs_normal_weight"] = vs_params.normal_weight;
    opt_json["opt_params"]["vs_lscm_weight"] = vs_params.sphericity_weight;
    opt_json["opt_params"]["vs_regularizer_weight"] = vs_params.regularizer_weight;

    const auto& ts_params = opt_params.transf_sol;
    opt_json["opt_params"]["ts_normal_weight"] = ts_params.normal_weight;
    opt_json["opt_params"]["ts_sphericity_weight"] = ts_params.sphericity_term_weight;
    opt_json["opt_params"]["ts_similarity_weight"] = ts_params.similarity_weight;

#if SPHERICITY_ON
    opt_json["opt_params"]["spheriticy_on"] = "ENABLED";
#else
    opt_json["opt_params"]["spheriticy_on"] = "DISABLED";
#endif

    opt_json["opt_params"]["version"] = std::to_string(globals::MAJOR_VERSION) +
                                        "." +
                                        std::to_string(globals::MINOR_VERSION);
    opt_json["opt_params"]["version_tag"] = globals::VERSION_TAG;


    fs::path fn = fs::path(out_dir) / (res_name + "_opt.json");
    std::ofstream out_f(fn);
    if(!out_f)
        return false;

    out_f << std::setw(2) << opt_json;
    return true;
}

}
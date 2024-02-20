#pragma once

#include <mesh_reshaping/edit_operation.h>
#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/reshaping_params.h>

#include <ca_essentials/meshes/trimesh.h>

#include <string>

namespace reshaping {
    bool save_mesh(const Eigen::MatrixXd& V,
                   const Eigen::MatrixXi& F,
                   const std::string& out_dir,
                   const std::string& res_name,
                   const std::string& suffix);

    bool save_mesh(const TriMesh& mesh,
                   const std::string& out_dir,
                   const std::string& res_name,
                   const std::string& suffix);

    bool save_edit_operation_to_obj(const reshaping::EditOperation& edit_op,
                                    const Eigen::MatrixXd& V,
                                    const Eigen::MatrixXi& T,
                                    const std::string& folder,
                                    const std::string& res_name,
                                    const double zero_tol=1e-6);

    bool save_optimization_iter_solution(const Eigen::MatrixXd& V,
                                         const Eigen::MatrixXi& F,
                                         int iter,
                                         const std::string& folder);

    bool save_optimization_info_to_json(const ca_essentials::meshes::TriMesh& mesh,
                                        const reshaping::ReshapingData& opt_data,
                                        const reshaping::ReshapingParams& opt_params,
                                        const std::string& out_dir,
                                        const std::string& res_name);
}
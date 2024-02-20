#include "load_input_data.h"

#include <mesh_reshaping/data_filenames.h>
#include <mesh_reshaping/face_principal_curvatures_io.h>

#include <ca_essentials/meshes/load_trimesh.h>

#include <memory>
#include <filesystem>

namespace {

std::unique_ptr<reshaping::TriMesh>
load_mesh(const std::string& mesh_fn) {
    namespace meshes = ca_essentials::meshes;

    bool normalize_mesh = true;
    auto tri_mesh = meshes::load_trimesh(mesh_fn, normalize_mesh);
    if(!tri_mesh)
        LOGGER.error("Could not load model {}", mesh_fn);

    return tri_mesh;
}

std::unique_ptr<reshaping::EditOperation>
load_edit_operation(const std::string& mesh_fn,
                    const std::string& op_label) {
    namespace fs = std::filesystem;

    std::string edit_op_fn = reshaping::get_edit_operation_fn(mesh_fn);
    if(!fs::exists(edit_op_fn)) {
        LOGGER.warn("Could not find edit operation file \"{}\"", edit_op_fn);
        return nullptr;
    }

    auto load_res = reshaping::load_edit_operation_from_json(edit_op_fn,
                                                             op_label);
    if(!load_res.first) {
        LOGGER.error("Error while loading edit operation \"{}\" from \"{}\"", op_label, edit_op_fn);
        return nullptr;
    }

    return std::make_unique<reshaping::EditOperation>(load_res.second);
}

std::unique_ptr<reshaping::StraightChains>
load_straightness_info(const std::string& mesh_fn) {
    namespace fs = std::filesystem;

    std::string fn = reshaping::get_straightness_fn(mesh_fn);
    if(!fs::exists(fn)) {
        LOGGER.warn("Could not find straightness file {}", fn);
        return nullptr;
    }

    auto straight_info = std::make_unique<reshaping::StraightChains>();
    if(!straight_info->load_from_file(fn)) {
        LOGGER.warn("Error while loading straightness information from {}", fn);
        return nullptr;
    }
    else
        return straight_info;
}

void load_principal_curvature_values(const std::string& mesh_fn,
                                     Eigen::VectorXd& face_k1,
                                     Eigen::VectorXd& face_k2) {
    namespace fs = std::filesystem;

    std::string fn = reshaping::get_curvature_fn(mesh_fn);
    if(!fs::exists(fn)) {
        LOGGER.error("Could not find curvature file at {}", fn);
        return;
    }

    bool succ = reshaping::load_face_principal_curvature_values(fn,
                                                                face_k1,
                                                                face_k2);
    if(!succ)
        LOGGER.error("Error while loading face curvature information from {}", fn);
}

}

InputData load_input_data(const std::string& mesh_fn,
                          const std::string& edit_op_label) {
    InputData data;
    data.mesh = load_mesh(mesh_fn);
    data.edit_op = load_edit_operation(mesh_fn, edit_op_label);
    data.straight_info = load_straightness_info(mesh_fn);
    load_principal_curvature_values(mesh_fn, data.PV1, data.PV2);

    return data;
}

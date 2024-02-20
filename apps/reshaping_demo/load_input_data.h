#pragma once

#include <mesh_reshaping/globals.h> 
#include <mesh_reshaping/straight_chains.h> 
#include <mesh_reshaping/edit_operation.h> 

#include <Eigen/Core>

#include <string>

struct InputData {
    std::unique_ptr<reshaping::TriMesh> mesh;
    std::unique_ptr<reshaping::StraightChains> straight_info;
    std::unique_ptr<reshaping::EditOperation> edit_op;

    // Principal curvature values
    Eigen::VectorXd PV1;
    Eigen::VectorXd PV2;
};
InputData load_input_data(const std::string& mesh_fn,
                          const std::string& edit_op_label);

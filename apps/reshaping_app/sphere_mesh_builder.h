#pragma once

// TODO: move it to ca_essentials

#include <Eigen/Core>

// TODO: remove duplicate mesh structure
struct SphereMesh {
    Eigen::MatrixXd V;
    Eigen::MatrixXi T;
    Eigen::MatrixXd N;
    Eigen::MatrixXd Tex;
};

SphereMesh build_sphere_mesh(float radius, int n_slices, int n_stacks);

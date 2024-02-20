#pragma once

#include <Eigen/Core>

struct ArrowMesh {
    Eigen::MatrixXd V;
    Eigen::MatrixXi T;
    Eigen::MatrixXd N;
    //Eigen::MatrixXd UV;
};

ArrowMesh build_arrow_mesh(double radius, double height,
                           int n_radial_segments = 10,
                           bool add_bottom_cap = true);

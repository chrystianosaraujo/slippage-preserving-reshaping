#pragma once

#include <Eigen/Core>

struct CylinderMesh {
    Eigen::MatrixXd V;
    Eigen::MatrixXi T;
    Eigen::MatrixXd N;
    //Eigen::MatrixXd UV;
};

CylinderMesh build_cylinder_mesh(double radius, double height,
                                 int n_radial_segments = 10,
                                 int n_height_segments = 4,
                                 bool add_caps = true);

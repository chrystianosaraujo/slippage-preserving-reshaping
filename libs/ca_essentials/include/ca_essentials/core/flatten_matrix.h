#pragma once

#include <Eigen/Core>

Eigen::VectorXd flatten_matrix(const Eigen::MatrixXd& m) {
    Eigen::VectorXd flattened_m(m.size());

    for(int j = 0; j < m.cols(); j++)
        for(int i = 0; i < m.rows(); i++)
            flattened_m(j * m.rows() + i) = m(i, j);

    return flattened_m;
}

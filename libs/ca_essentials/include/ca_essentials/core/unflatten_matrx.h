#pragma once

#include <Eigen/Core>

Eigen::MatrixXd unflatten_matrix(const Eigen::VectorXd& m, int rows, int cols) {
    Eigen::MatrixXd unflattened_m(rows, cols);

    for(int i = 0; i < rows; i++)
        for(int j = 0; j < cols; j++)
            unflattened_m(i, j) = m(i * cols + j);

    return unflattened_m;
}
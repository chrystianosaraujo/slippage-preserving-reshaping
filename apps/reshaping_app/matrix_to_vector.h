#pragma once

#include <Eigen/Core>
#include <glad/glad.h>

template <typename DerivedM>
void matrix_to_vector(const Eigen::MatrixBase<DerivedM> &mat,
                      std::vector<typename DerivedM::Scalar> &vec) {

    vec.resize(mat.size());
    for(int i = 0; i < mat.rows(); i++)
        for(int j = 0; j < mat.cols(); j++)
            vec[i * mat.cols() + j] = mat(i, j);
}

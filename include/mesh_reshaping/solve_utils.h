#pragma once

#include <Eigen/Core>
#include <Eigen/Sparse>
#include <vector>

namespace reshaping {

using TripletsVec = std::vector<Eigen::Triplet<double>>;

enum VERTEX_COMP {
    X = 0,
    Y = 1,
    Z = 2
};

inline int vertex_to_sys_idx(int vid, VERTEX_COMP comp, int num_verts) {
    return comp * num_verts + vid;
}

inline int matrix_to_sys_idx(int tid, int T_row, int T_col) {
    return tid * 9 + T_row * 3 + T_col;
}

}
#pragma once

#include <Eigen/Core>

namespace reshaping {

double compute_max_vertex_change(const Eigen::MatrixXd& prev_V,
                                 const Eigen::MatrixXd& curr_V) {
    double max_change = 0.0;
    for(int i = 0; i < prev_V.rows(); ++i) {
        const Eigen::Vector3d& p0 = prev_V.row(i);
        const Eigen::Vector3d& p1 = curr_V.row(i);

        double change = (p1 - p0).squaredNorm();
        if(change > max_change)
            max_change = change;
    }

    return sqrt(max_change);
}

}

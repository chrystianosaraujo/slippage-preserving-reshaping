#pragma once

#include <Eigen/Geometry>

namespace ca_essentials {
namespace core {

inline Eigen::AlignedBox3d bounding_box(const Eigen::MatrixXd& V) {
    Eigen::AlignedBox3d bbox;
    for(int i = 0; i < V.rows(); ++i)
        bbox.extend((Eigen::Vector3d) V.row(i));

    return bbox;
}

}
}

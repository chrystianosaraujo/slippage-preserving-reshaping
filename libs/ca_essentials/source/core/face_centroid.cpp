#include <ca_essentials/core/face_centroid.h>

namespace ca_essentials {
namespace core {

Eigen::Vector3d face_centroid(const Eigen::MatrixXd& V,
                              const Eigen::MatrixXi& F,
                              int f) {
    return (V.row(F(f, 0)) + V.row(F(f, 1)) + V.row(F(f, 2))) / 3.0;
}

}
}
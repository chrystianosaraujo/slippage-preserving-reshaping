#include <ca_essentials/meshes/normalize_to_unitbox.h>

namespace ca_essentials{
namespace meshes {

void normalize_to_unitbox(Eigen::MatrixXd& V) {
    V = V.rowwise() - V.colwise().minCoeff();
    V /= V.maxCoeff();
}

void normalize_to_unitbox(std::vector<Eigen::MatrixXd>& meshes_V) {
    const Eigen::MatrixXd& base_V = meshes_V.front();

    Eigen::RowVector3d t = base_V.colwise().minCoeff().transpose();
    double s          = 1.0 / (base_V.rowwise() - t).maxCoeff();

    for(auto& V : meshes_V) {
        V = V.rowwise() - t;
        V *= s;
    }
}

}
}

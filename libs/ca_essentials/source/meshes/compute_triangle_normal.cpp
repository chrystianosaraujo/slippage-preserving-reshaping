#include <ca_essentials/meshes/compute_triangle_normal.h>

namespace ca_essentials {
namespace meshes {

Eigen::MatrixXd compute_triangle_normal(const Eigen::MatrixXd& V,
                                        const Eigen::MatrixXi& F) {
    Eigen::MatrixXd normals(F.rows(), 3);

    for(int f = 0; f < (int) F.rows(); ++f) {
        const Eigen::Vector3d& v0 = V.row(F(f, 0));
        const Eigen::Vector3d& v1 = V.row(F(f, 1));
        const Eigen::Vector3d& v2 = V.row(F(f, 2));
        normals.row(f) = (v1 - v0).cross(v2 - v0).normalized();
    }

    return normals;
}

Eigen::MatrixXd compute_triangle_normal(const TriMesh& mesh) {
    return compute_triangle_normal(mesh.get_vertices(),
                                   mesh.get_facets());
}

}
}
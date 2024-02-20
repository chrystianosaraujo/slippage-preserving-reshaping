#include <ca_essentials/core/compute_flat_mesh.h>

#include <Eigen/Core>
#include <igl/per_vertex_normals.h>

namespace ca_essentials {
namespace core {

void compute_flat_mesh(const Eigen::MatrixXd& V,
                       const Eigen::MatrixXi& T,
                       Eigen::MatrixXd& OV,
                       Eigen::MatrixXi& OT,
                       Eigen::MatrixXd& ON) {
    OV.resize(T.rows() * 3, 3);
    OT.resize(T.rows(), 3);
    
    for(int f = 0; f < T.rows(); ++f) {
        int vid0 = T(f, 0);
        int vid1 = T(f, 1);
        int vid2 = T(f, 2);
    
        OV.row(f * 3 + 0) = V.row(vid0);
        OV.row(f * 3 + 1) = V.row(vid1);
        OV.row(f * 3 + 2) = V.row(vid2);
    
        OT.row(f) << f * 3 + 0, f * 3 + 1, f * 3 + 2;
    }
    
    igl::per_vertex_normals(OV, OT, ON);
}

}
}

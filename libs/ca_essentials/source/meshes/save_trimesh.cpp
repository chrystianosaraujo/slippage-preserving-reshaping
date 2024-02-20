#include <ca_essentials/meshes/save_trimesh.h>

#include <igl/writeOBJ.h>

namespace ca_essentials {
namespace meshes {

bool save_trimesh(const std::string& fn, const TriMesh& mesh) {
    return igl::writeOBJ(fn, mesh.get_vertices(),
                             mesh.get_facets());
}

bool save_trimesh(const std::string& fn,
                  const Eigen::MatrixXd& V, const Eigen::MatrixXi& F) {
    return igl::writeOBJ(fn, V, F);
}

}
}

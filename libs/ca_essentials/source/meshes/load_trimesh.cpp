#include <ca_essentials/meshes/load_trimesh.h>
#include <ca_essentials/meshes/normalize_to_unitbox.h>
#include <ca_essentials/core/logger.h>

#include <igl/readOBJ.h>
#include <igl/readPLY.h>

#include <filesystem>

namespace ca_essentials {
namespace meshes {

std::unique_ptr<ca_essentials::meshes::TriMesh>
load_trimesh(const std::string& fn, bool normalize) {
    namespace fs = std::filesystem;
    using TriMesh = ca_essentials::meshes::TriMesh;

    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    bool succ = false;

    std::string ext = fs::path(fn).extension().string();
    if(ext == ".obj")
        succ = igl::readOBJ(fn, V, F);
    else if(ext == ".ply")
        succ = igl::readPLY(fn, V, F);
    else
        LOGGER.warn("Error while loading file {}. Extension is not supported.", fn);

     if(!succ)
        LOGGER.error("Error while loading file {}. ", fn);

    std::unique_ptr<TriMesh> mesh;
    if(succ) {
        if(normalize)
            normalize_to_unitbox(V);

        mesh = std::make_unique<TriMesh>(V, F);

        LOGGER.info("Triangle mesh loaded from {}", fn);
    }

    return mesh;
}

}
}

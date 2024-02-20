#include <mesh_reshaping/load_feature_edges.h>

#include <ca_essentials/core/logger.h>

#include <string>
#include <sstream>
#include <fstream>

namespace reshaping {

std::unordered_set<int> load_feature_edges(const std::string& fn,
                                           const reshaping::TriMesh& mesh) {
    std::ifstream in_f(fn);
    if(!in_f) {
        LOGGER.error("Error while loading feature edges from: {}", fn);
        return {};
    }

    std::unordered_set<int> feature_edges;

    std::string line;
    while(std::getline(in_f, line)) {
        std::istringstream ss(line);

        int vid0;
        int vid1;
        ss >> vid0 >> vid1;

        feature_edges.insert(mesh.get_edge_index(vid0, vid1));
    }

    return feature_edges;
}

}

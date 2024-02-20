#pragma once

#include <mesh_reshaping/types.h>
#include <unordered_set>

namespace reshaping {

std::unordered_set<int> load_feature_edges(const std::string& fn,
                                           const reshaping::TriMesh& mesh);

}

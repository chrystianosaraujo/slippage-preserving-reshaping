#pragma once

#include <ca_essentials/meshes/trimesh.h>

#include <memory>

namespace ca_essentials {
namespace meshes {

std::unique_ptr<ca_essentials::meshes::TriMesh> 
load_trimesh(const std::string& fn,
             bool normalize = true);
}
}
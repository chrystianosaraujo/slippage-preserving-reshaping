#include <mesh_reshaping/data_filenames.h>

#include <filesystem>

namespace {
    std::string edit_op_ext   = "deform";
    std::string camera_ext    = "cam";
    std::string straight_ext  = "straight";
    std::string curvature_ext = "fk";
}

namespace reshaping {

std::string get_edit_operation_fn(const std::string& mesh_fn) {
    namespace fs = std::filesystem;

    fs::path edit_fn(mesh_fn);
    edit_fn.replace_extension(edit_op_ext);

    return edit_fn.string();
}

std::string get_camera_fn(const std::string& mesh_fn) {
    namespace fs = std::filesystem;

    fs::path camera_fn(mesh_fn);
    camera_fn.replace_extension(camera_ext);

    return camera_fn.string();
}

std::string get_straightness_fn(const std::string& mesh_fn) {
    namespace fs = std::filesystem;

    fs::path straight_fn(mesh_fn);
    straight_fn.replace_extension(straight_ext);

    return straight_fn.string();
}

std::string get_curvature_fn(const std::string& mesh_fn) {
    namespace fs = std::filesystem;

    fs::path curvature_fn(mesh_fn);
    curvature_fn.replace_extension(curvature_ext);

    return curvature_fn.string();
}

}

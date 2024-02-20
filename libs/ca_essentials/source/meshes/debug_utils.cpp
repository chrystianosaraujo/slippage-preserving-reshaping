#include <ca_essentials/meshes/debug_utils.h>
#include <ca_essentials/meshes/build_cylinder_mesh.h>
#include <ca_essentials/core/logger.h>
#include <ca_essentials/core/face_centroid.h>
#include <ca_essentials/core/edges_centroid.h>
#include <ca_essentials/meshes/compute_edge_normals.h>
#include <ca_essentials/core/color.h>
#include <ca_essentials/io/saveOBJ.h>

#include <igl/bounding_box_diagonal.h>
#include <igl/avg_edge_length.h>
#include <igl/colormap.h>

#include <fstream>
#include <filesystem>
#include <iomanip>
#include <unordered_set>

namespace {
bool save_multiple_objects_to_obj(const std::string& fn,
                                  const std::vector<Eigen::MatrixXd>& Vs,
                                  const std::vector<Eigen::MatrixXi>& Fs,
                                  const std::vector<Eigen::Vector3d>& Ms) {
    namespace fs = std::filesystem;

    std::string mtl_fn = (fs::path(fn).parent_path() / (fs::path(fn).stem().string() + ".mtl")).string();

    std::ofstream out_f(fn);
    if(!out_f)
        return false;

    out_f << std::setprecision(10) << std::fixed;
    out_f << "mtllib " << fs::path(fn).stem().string() + ".mtl" << std::endl;

    int base_vid = 0;
    for(int o = 0; o < (int) Vs.size(); ++o) {
        const Eigen::MatrixXd& V = Vs.at(o);
        const Eigen::MatrixXi& F = Fs.at(o);

        out_f << "o object." << std::to_string(o) << std::endl;
        for(int i = 0; i < (int) V.rows(); ++i) {
            const Eigen::Vector3d& v = V.row(i);

            out_f << "v " << v.x() << " " << v.y() << " " << v.z() << std::endl;
        }

        out_f << "s off" << std::endl;
        out_f << "usemtl material_" + std::to_string(o) << std::endl;
        for(int i = 0; i < (int) F.rows(); ++i) {
            out_f << "f " << F(i, 0) + base_vid + 1 << " "
                          << F(i, 1) + base_vid + 1 << " "
                          << F(i, 2) + base_vid + 1 << std::endl;
        }

        base_vid += (int) V.rows();
    }


    // Writing MTL file
    std::ofstream mtlFile(mtl_fn);
    if(!mtlFile.good()) {
        LOGGER.error("Error while creating mtl file: {}", mtl_fn);
        return false;
    }

    for(int m = 0; m < Ms.size(); ++m) {
        const auto& color = Ms.at(m);

        mtlFile << "newmtl material_" + std::to_string(m) << std::endl;
        mtlFile << "Kd " << std::fixed << std::setprecision(8) << color.x() << " "
                                                               << color.y() << " "
                                                               << color.z() << std::endl;
    }

    return true;
}

}

namespace ca_essentials {
namespace meshes {

bool save_per_face_normals_to_obj(const std::string& fn,
                                  const TriMesh& mesh,
                                  const std::vector<Eigen::Vector3d>& normals,
                                  const double normal_vec_len) {

    const double diag = igl::bounding_box_diagonal(mesh.get_vertices());
    const double segment_len = diag * normal_vec_len;

    std::ofstream out_f(fn);
    if(!out_f) {
        LOGGER.warn("Could not export face normals to file {}", fn);
        return false;
    }

    const Eigen::MatrixXd& verts = mesh.get_vertices();
    const Eigen::MatrixXi& faces = mesh.get_facets();

    for(int f = 0; f < (int) faces.rows(); ++f) {
        const Eigen::Vector3d& v0 = verts.row(faces(f, 0));
        const Eigen::Vector3d& v1 = verts.row(faces(f, 1));
        const Eigen::Vector3d& v2 = verts.row(faces(f, 2));

        const Eigen::Vector3d c = (v0 + v1 + v2) / 3.0;
        const Eigen::Vector3d& n = normals.at(f);
        const Eigen::Vector3d seg_end = c + n * segment_len;

        out_f << "v " << c.x() << " " << c.y() << " " << c.z() << std::endl;
        out_f << "v " << seg_end.x() << " " << seg_end.y() << " " << seg_end.z() << std::endl;
        out_f << "l " << f * 2 + 1 << " " << f * 2 + 2 << std::endl;
    }

    return true;
}

bool save_segments_to_obj(const std::string& fn,
                          const std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>>& segs) {

    std::ofstream out_f(fn);
    if(!out_f) {
        LOGGER.warn("Could not export segments to file {}", fn);
        return false;
    }

    for(size_t i = 0; i < segs.size(); ++i) {
        const auto& v0 = segs.at(i).first;
        const auto& v1 = segs.at(i).second;

        out_f << "v " << v0.x() << " " << v0.y() << " " << v0.z() << std::endl;
        out_f << "v " << v1.x() << " " << v1.y() << " " << v1.z() << std::endl;
        out_f << "l " << i * 2 + 1 << " " << i * 2 + 2 << std::endl;
    }

    return true;
}

bool save_points_to_obj(const std::string& fn,
                        const std::vector<Eigen::Vector3d>& points,
                        const std::vector<Eigen::Vector3f>* colors) {

    std::ofstream out_f(fn);
    if(!out_f) {
        LOGGER.warn("Could not export colored points to file {}", fn);
        return false;
    }

    for(size_t i = 0; i < points.size(); ++i) {
        const Eigen::Vector3d& p = points.at(i);
        const Eigen::Vector3f* c = colors ? &colors->at(i) : nullptr;

        out_f << "v " << p.x() << " " << p.y() << " " << p.z();
        if(c)
            out_f << " " << c->x() << " " << c->y() << " " << c->z();

        out_f << std::endl;
    }

    return true;
}

bool save_triangles_local_frames(const std::string& fn,
                                 const Eigen::MatrixXd& V,
                                 const Eigen::MatrixXi& F,
                                 const std::vector<Eigen::Matrix3d>& frames) {
    namespace meshes = ca_essentials::meshes;

    double avg_edge_len = igl::avg_edge_length(V, F);

    std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> segs;
    for(int f = 0; f < F.rows(); ++f) { 
        const Eigen::Matrix3d& M = frames.at(f);

        Eigen::Vector3d v1 = V.row(F(f, 0));
        const Eigen::Vector3d c = ca_essentials::core::face_centroid(V, F, f);

        Eigen::Vector3d source = v1 + (c - v1) * 0.3;

        Eigen::Vector3d v2 = source + M.col(0);
        Eigen::Vector3d v3 = source + M.col(1);
        Eigen::Vector3d v4 = source + M.col(2);

        segs.emplace_back(source, v2);
        segs.emplace_back(source, v3);
        segs.emplace_back(source, v4);
    }

    return meshes::save_segments_to_obj(fn.c_str(), segs);
}

bool save_per_edge_scalar_property_to_obj(const std::string& fn,
                                          const TriMesh& mesh,
                                          const Eigen::MatrixXd& V,
                                          const Eigen::MatrixXd& edge_normals,
                                          const Eigen::VectorXd& per_edge_weights,
                                          const double max_vec_len) {
    namespace meshes = ca_essentials::meshes;

    std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> segs;

    double min_val = per_edge_weights.minCoeff();
    double max_val = per_edge_weights.maxCoeff();

    int num_edges = (int) edge_normals.rows();
    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        Eigen::Vector3d c = (V.row(edge_verts[0]) + V.row(edge_verts[1])) * 0.5;
        const Eigen::Vector3d& n = edge_normals.row(e);

        double f = 1.0;
        if((max_val - min_val) > 1e-7)
            f = (per_edge_weights(e) - min_val) / (max_val - min_val);

        segs.emplace_back(c, c + n * f * max_vec_len);
    }

    return meshes::save_segments_to_obj(fn.c_str(), segs);
}

bool save_triangle_groups_to_colored_obj(const std::string& fn,
                                         const Eigen::MatrixXd& V,
                                         const Eigen::MatrixXi& F,
                                         const Eigen::VectorXi& F_group,
                                         const Eigen::Vector3d& first_group_color) {

    std::unordered_set<int> groups;
    for(int i = 0; i < (int) F_group.rows(); ++i)
        groups.insert(F_group(i));

    // Making sure that the id for non-planar triangles are always present, even when not used=
    groups.insert(0);

    LOGGER.info("Saving Triangle Groups to Colored obj... #groups {}", groups.size());

    const auto& default_colors = ca_essentials::core::get_default_colors();
    if(groups.size() > default_colors.size()) {
        LOGGER.critical("Number of groups exceeds the number of available colors! Color are being reused");
        return false;
    }

    std::vector<ca_essentials::io::OBJMaterial> materials(groups.size());
    for(int i = 0; i < groups.size(); ++i) {
        Eigen::Vector3d color;
        if(i == 0)
            color = first_group_color;
        else
            color = default_colors[i - 1].to_eigen_vec().cast<double>().head(3);


        materials.at(i).Kd = color;
    }

    std::vector<int> face_mat_id((int) F.rows(), 0);
    for(int i = 0; i < (int) F_group.rows(); ++i)
        face_mat_id.at(i) = F_group(i);

    ca_essentials::io::saveOBJ(fn.c_str(), V, F, materials, face_mat_id);
    return true;
}

void compute_oriented_cylinder_on_edge(const Eigen::Vector3d& c,
                                       const Eigen::Vector3d& n,
                                       const Eigen::MatrixXd& cylinder_V,
                                       const double height,
                                       Eigen::MatrixXd& e_cylinder_V) {
    Eigen::IOFormat OctaveFmt(Eigen::StreamPrecision, 0, ", ", ";\n", "", "", "[", "]");

    Eigen::Vector3d base_center(0.0, 0.0, -height / 2.0);

    e_cylinder_V.resize(cylinder_V.rows(), 3);

    const Eigen::Vector3d Z(0.0, 0.0, 1.0);
    Eigen::Vector3d cross = n.cross(Z).normalized();

    // Computing rotation matrix
    double dot = n.dot(Z);
    dot = std::min(std::max(dot, -1.0), 1.0);

    double angle = acos(dot);

    Eigen::Matrix3d R = Eigen::AngleAxisd(-angle, cross).toRotationMatrix();

    const Eigen::Vector3d T = n * height/2.0;
    for(int i = 0; i < (int) cylinder_V.rows(); ++i) {
        const Eigen::Vector3d &v = cylinder_V.row(i);

        e_cylinder_V.row(i) = T + c + (R * v);
    }
}

bool save_per_edge_property_to_colored_obj(const std::string& fn,
                                           const TriMesh& mesh,
                                           const Eigen::MatrixXd& V,
                                           const Eigen::MatrixXd& EN,
                                           const Eigen::VectorXd& per_edge_value,
                                           const double max_len,
                                           const bool ensure_zero_black) {
    namespace meshes = ca_essentials::meshes;
    namespace core = ca_essentials::core;

    const Eigen::MatrixXi& F = mesh.get_facets();

    double radius  = max_len * 0.04;
    double height  = max_len; 

    Eigen::MatrixXd cylinder_V;
    Eigen::MatrixXi cylinder_F;
    meshes::build_cylinder_mesh(radius, height, cylinder_V, cylinder_F, 6, 2, true);

    Eigen::MatrixXd centroids = core::edges_centroid(mesh, V);

    Eigen::MatrixXd colors;
    igl::colormap(igl::COLOR_MAP_TYPE_TURBO, per_edge_value, true, colors);

    std::vector<Eigen::MatrixXd> e_V(mesh.get_num_edges());
    std::vector<Eigen::MatrixXi> e_F(mesh.get_num_edges());
    std::vector<Eigen::Vector3d> e_M(mesh.get_num_edges());

    for(int e = 0; e < mesh.get_num_edges(); ++e) {
        const Eigen::Vector3d& c = centroids.row(e);
        const Eigen::Vector3d& n = EN.row(e);

        Eigen::MatrixXd e_cylinder_V;
        compute_oriented_cylinder_on_edge(c, n, cylinder_V, height, e_cylinder_V);

        e_V.at(e) = e_cylinder_V;
        e_F.at(e) = cylinder_F;
        e_M.at(e) = colors.row(e);

        if(ensure_zero_black && per_edge_value(e) < 1e-10) {
            e_M.at(e).setZero();
        }
    }

    return save_multiple_objects_to_obj(fn, e_V, e_F, e_M);
}

}
}

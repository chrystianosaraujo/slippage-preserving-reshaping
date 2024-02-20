#include <mesh_reshaping/sphericity_terms_info.h>
#include <mesh_reshaping/globals.h>
#include <mesh_reshaping/types.h>

#include <ca_essentials/io/saveOBJ.h>
#include <ca_essentials/meshes/debug_utils.h>

#include <igl/colormap.h>
#include <igl/writeOBJ.h>
#include <igl/per_face_normals.h>

#include <igl/average_onto_faces.h>

#include <fstream>
#include <iomanip>
#include <filesystem>

namespace {

#if EXPORT_SPHERICITY_DEBUG_FILES_ON
void save_per_face_curvatures_to_obj(const std::string& debug_folder,
                                     const std::vector<ca_essentials::meshes::PrincipalCurvatures>& ks,
                                     const Eigen::MatrixXd& V,
                                     const Eigen::MatrixXi& F) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

    double max_k1 = -DBL_MAX;
    double min_k1 =  DBL_MAX;
    double max_k2 = -DBL_MAX;
    double min_k2 =  DBL_MAX;
    for(int i = 0; i < (int)F.rows(); ++i) {
        min_k1 = std::min(min_k1, ks.at(i).k1);
        max_k1 = std::max(max_k1, ks.at(i).k1);
        min_k2 = std::min(min_k2, ks.at(i).k2);
        max_k2 = std::max(max_k2, ks.at(i).k2);
    }

    double avg_edge_len = ca_essentials::core::avg_edge_length(V, F);
    double seg_len = avg_edge_len * 0.9;

    Eigen::MatrixXd FN;
    igl::per_face_normals(V, F, FN);

    std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> k1_pairs;
    std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> k2_pairs;

    for(int i = 0; i < (int) F.rows(); ++i) {
        const Eigen::Vector3d& n  = FN.row(i);

        const Eigen::Vector3d& v0 = V.row(F(i, 0));
        const Eigen::Vector3d& v1 = V.row(F(i, 1));
        const Eigen::Vector3d& v2 = V.row(F(i, 2));

        Eigen::Vector3d c = (v0 + v1 + v2) / 3.0;
        Eigen::Vector3d edge_c = (v1 + v2) / 2.0;
        Eigen::Vector3d vec = c - edge_c;

        Eigen::Vector3d k1_o = c + vec * 0.2;
        Eigen::Vector3d k2_o = c - vec * 0.2;

        double k1 = ks.at(i).k1;
        double k2 = ks.at(i).k2;

        double k1_f = (k1 - min_k1) / (max_k1 - min_k1);
        double k2_f = (k2 - min_k2) / (max_k2 - min_k2);

        k1_pairs.emplace_back(k1_o, k1_o + n * k1_f * seg_len);
        k2_pairs.emplace_back(k2_o, k2_o + n * k2_f * seg_len);
    }

    fs::path k1_fn = fs::path(debug_folder) / ("per_face_k1.obj");
    fs::path k2_fn = fs::path(debug_folder) / ("per_face_k2.obj");
    meshes::save_segments_to_obj(k1_fn.string(), k1_pairs);
    meshes::save_segments_to_obj(k2_fn.string(), k2_pairs);
}
#endif

#if EXPORT_SPHERICITY_DEBUG_FILES_ON
 void save_weights_to_obj(const std::string& debug_folder,
                          const ca_essentials::meshes::TriMesh& mesh,
                          const Eigen::VectorXd& weights) {
    namespace io = ca_essentials::io;
    namespace fs = std::filesystem;

     double min_w = 0.0;
     double max_w = 1.0;

     Eigen::MatrixXd C;
     igl::colormap(igl::ColorMapType::COLOR_MAP_TYPE_TURBO, weights, min_w, max_w, C);

     const Eigen::MatrixXd& V = mesh.get_vertices();
     const Eigen::MatrixXi& F = mesh.get_facets();

     fs::path fn = fs::path(debug_folder) / "sphericity_weights.obj";
     io::saveOBJ(fn.string().c_str(), V, F, C);
 }

 void test_colormap(const std::string& debug_folder) {
     double width = 10.0;
     double height = 1.0;

     int num_quads = 11;

     Eigen::MatrixXd V((num_quads + 1) * 2, 3);
     for(int i = 0; i <= num_quads; ++i) {
         Eigen::Vector3d v0(width / num_quads * i, 0.0, 0.0);
         Eigen::Vector3d v1(width / num_quads * i, height, 0.0);

         V.row(i * 2 + 0) = v0;
         V.row(i * 2 + 1) = v1;
     }

     Eigen::MatrixXi F(num_quads * 2, 3);
     Eigen::VectorXd Z(F.rows(), 1);
     for(size_t i = 0; i < (size_t) num_quads; ++i) {
         size_t bottom_l = i * 2;
         size_t bottom_r = ((i + 1) * 2);
         size_t top_l = bottom_l + 1;
         size_t top_r = bottom_r + 1;

         F(i * 2, 0) = (int) bottom_l;
         F(i * 2, 1) = (int) bottom_r;
         F(i * 2, 2) = (int) top_r;

         F(i * 2 + 1, 0) = (int) bottom_l;
         F(i * 2 + 1, 1) = (int) top_r;
         F(i * 2 + 1, 2) = (int) top_l;

         Z(i * 2)     = (double) i;
         Z(i * 2 + 1) = (double) i;
     }

     Eigen::MatrixXd C;
     igl::colormap(igl::COLOR_MAP_TYPE_TURBO, Z, true, C);

     std::string fn = (std::filesystem::path(debug_folder) / "colormap.obj").string();
     ca_essentials::io::saveOBJ(fn.c_str(), V, F, C);
 }
 #endif

Eigen::VectorXd compute_sphericity_weights(const Eigen::VectorXd& F_PV1,
                                           const Eigen::VectorXd& F_PV2,
                                           const double sigma1,
                                           const double sigma2,
                                           const double k_max_const,
                                           const double epsilon,
                                           const std::string& debug_folder) {
    namespace fs = std::filesystem;

#if EXPORT_SPHERICITY_DEBUG_FILES_ON
    std::ofstream out_f;
    if(!debug_folder.empty()) {
        out_f.open((fs::path(debug_folder) / "sphericity_weights.csv"));

        out_f << std::setprecision(10) << std::fixed;

        out_f << "F_1 sigma, " << sigma1 << std::endl;
        out_f << "F_2 sigma, " << sigma2 << std::endl;
        out_f << "Epsilon, " << epsilon << std::endl;
        out_f << "k_max_const, " << k_max_const << std::endl << std::endl;
        out_f << ", Min., Max." << std::endl;
        out_f << "k1, " << F_PV1.minCoeff() << ", " << F_PV1.maxCoeff() << std::endl;
        out_f << "k2, " << F_PV2.minCoeff() << ", " << F_PV2.maxCoeff() << std::endl;
        out_f << "\n";

        out_f << "FID, k1, k2, f1, f2, w" << std::endl;
    }
#endif

    const int num_tris = (int) F_PV1.rows();
    Eigen::VectorXd weights(num_tris);

    for(int fid = 0; fid < num_tris; ++fid) {
        const double k1 = F_PV1(fid);
        const double k2 = F_PV2(fid);

        double x = (k1 - k2) / (std::max(fabs(k1), fabs(k2)) + epsilon);
        double f1 = exp(-pow(x, 2.0) / (2.0 * pow(sigma1, 2.0)));

        if(x > 0.5)
            f1 = 0.0;

        double x2 = std::min(fabs(k1), fabs(k2)) / k_max_const;
        x2 = std::min(x2, 1.0);

        double f2 = 0.0;
        if(x2 > 1e-2)
            f2 = exp(-pow(x2 - 1.0, 2.0) / (2.0 * pow(sigma2, 2.0)));

        weights(fid) = f1 * f2;

#if EXPORT_SPHERICITY_DEBUG_FILES_ON
        out_f << fid << ", " << k1 << ", " << k2 << ", " << f1 << ", " << f2 << ", " << weights(fid) << std::endl;
#endif
    }

    return weights;
}

// Computes the rotation matrix and length scale so edges 
// (vid_j - vid_i) = R * ratio * (vid_k - vid_i)
void compute_rotation_matrix_and_ratio(const reshaping::TriMesh& mesh,
                                       const int vid_i,
                                       const int vid_j,
                                       const int vid_k,
                                       const Eigen::Vector3d& f_n,
                                       Eigen::Matrix3d& R,
                                       double& ratio) {
    const Eigen::MatrixXd& V = mesh.get_vertices();

    Eigen::Vector3d E0 = (V.row(vid_j) - V.row(vid_i));
    Eigen::Vector3d E1 = (V.row(vid_k) - V.row(vid_i));

    Eigen::Vector3d E0_n = E0.normalized();
    Eigen::Vector3d E1_n = E1.normalized();

    double len_e0 = E0.norm();
    double len_e1 = E1.norm();
    
    double dot = E0_n.dot(E1_n);
    dot = std::min(std::max(dot, -1.0), 1.0);
    double angle = -acos(dot);

    // check the angle sign
    Eigen::Vector3d cross = E0_n.cross(E1_n);
    if(cross.dot(f_n) < 0.0)
        angle *= -1.0;

    // Rotating (vid_k - vid_i) into (vid_j - vid_i)
    R = Eigen::AngleAxisd(angle, f_n).toRotationMatrix();

    // Scales E1 to match E0
    ratio = len_e0 / len_e1;
}

}

namespace reshaping {

std::vector<SphericityTermInfo> compute_sphericity_terms_info(const TriMesh& mesh,
                                                              const Eigen::VectorXd& F_PV1,
                                                              const Eigen::VectorXd& F_PV2,
                                                              const SphericityTermsParams& params) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

#if EXPORT_SPHERICITY_DEBUG_FILES_ON
    if(!params.debug_folder.empty())
        test_colormap(params.debug_folder);
#endif

    Eigen::VectorXd weights = compute_sphericity_weights(F_PV1, F_PV2,
                                                         params.sigma1, params.sigma2,
                                                         params.k_max_const, params.epsilon,
                                                         params.debug_folder);

    
    Eigen::MatrixXd V = mesh.get_vertices();
    Eigen::MatrixXi F = mesh.get_facets();
    Eigen::MatrixXd FN;
    igl::per_face_normals(V, F, FN);

    int edge_pairs[3][3] = {
        {1, 0, 2},
        {0, 1, 2},
        {0, 2, 1},
    };

    std::vector<SphericityTermInfo> sphericity_info;
    for(int fid = 0; fid < mesh.get_num_facets(); ++fid) {
        const Eigen::Vector3d fn = FN.row(fid);

        for(int x = 0; x < 3; ++x) {
            int vid_j = F(fid, edge_pairs[x][0]);
            int vid_i = F(fid, edge_pairs[x][1]);
            int vid_k = F(fid, edge_pairs[x][2]);

            Eigen::Matrix3d R;
            double ratio;
            compute_rotation_matrix_and_ratio(mesh, vid_i, vid_j, vid_k, fn, R, ratio);

            sphericity_info.emplace_back();
            sphericity_info.back().fid   = fid;
            sphericity_info.back().vid_i = vid_i;
            sphericity_info.back().vid_j = vid_j;
            sphericity_info.back().vid_k = vid_k;
            sphericity_info.back().w     = weights(fid);
            sphericity_info.back().R     = R;
            sphericity_info.back().ratio = ratio;
        }
    }

#if EXPORT_SPHERICITY_DEBUG_FILES_ON
    if(!params.debug_folder.empty())
        save_weights_to_obj(params.debug_folder, mesh, weights);
#endif

    return sphericity_info;
}

}

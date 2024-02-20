#include <mesh_reshaping/vertex_solve.h>
#include <mesh_reshaping/solve_utils.h>
#include <mesh_reshaping/types.h>
#include <mesh_reshaping/globals.h>

#include <Eigen/Core>

#include <vector>

namespace {

int matrix_to_sys_idx(int tid, int T_row, int T_col) {
    return reshaping::matrix_to_sys_idx(tid, T_row, T_col);
}

Eigen::Vector2i system_size(const reshaping::ReshapingData& data) {
    const int num_edges = data.mesh.get_num_edges();
    const int num_triangles = data.mesh.get_num_facets();

    int num_cols = num_triangles * 9;
    int num_rows = 0;

    // Connect term
    num_rows += num_edges * 3;

    // Similarity term
    num_rows += num_edges * 3;

    // Transform term
    num_rows += num_edges * 3 * 2;

    // Sphericity term
#if SPHERICITY_ON
    num_rows += (int) data.sphericity_terms_info.size() * 3;
#endif

    // Normal term
    num_rows += num_edges * 2;

    // Regularizer term
    num_rows +=  num_triangles * 9;

    return Eigen::Vector2i(num_rows, num_cols);
}

void compute_connect_entries(const reshaping::TransfSolveParams& params,
                             const reshaping::ReshapingData& data,
                             reshaping::TripletsVec& triplets,
                             Eigen::VectorXd& b,
                             Eigen::VectorXd& w,
                             int& curr_row) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    for(int e = 0; e < num_edges; ++e) {
        const int tid0 = adj_e2f.at(e).at(0);
        const int tid1 = adj_e2f.at(e).at(1);

        const Eigen::Vector3d E = data.orig_edges.row(e).normalized();
        const double w_e = 1.0;

        //   ||                        ||2
        //   || T_i e^0_ij - T_j e^0_ij||
        //   ||                        ||
        // 
        //  ||                                                                               ||2
        //  || [T_i^00 E^x + T_i^01 E^y + T_i^02 E^z - T_j^00 E^x - T_j^01 E^y - T_j^02 E^z] ||
        //  || [T_i^10 E^x + T_i^11 E^y + T_i^12 E^z - T_j^10 E^x - T_j^11 E^y - T_j^12 E^z] ||
        //  || [T_i^20 E^x + T_i^21 E^y + T_i^22 E^z - T_j^20 E^x - T_j^21 E^y - T_j^22 E^z] ||
        //  ||                                                                               ||   

        // First row entries
        //  [T_i^00 E^x + T_i^01 E^y + T_i^02 E^z - T_j^00 E^x - T_j^01 E^y - T_j^02 E^z] 
        {
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 0, 0), E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 0, 1), E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 0, 2), E.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 0, 0), -E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 0, 1), -E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 0, 2), -E.z());

            b(curr_row) = 0.0;
            w(curr_row) = w_e * params.connect_weight;
            curr_row++;
        }

        // Second row entries
        // [T_i^10 E^x + T_i^11 E^y + T_i^12 E^z - T_j^10 E^x - T_j^11 E^y - T_j^12 E^z]
        {
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 1, 0), E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 1, 1), E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 1, 2), E.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 1, 0), -E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 1, 1), -E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 1, 2), -E.z());

            b(curr_row) = 0.0;
            w(curr_row) = w_e * params.connect_weight;
            curr_row++;
        }

        // Third row entries
        // [T_i^20 E^x + T_i^21 E^y + T_i^22 E^z - T_j^20 E^x - T_j^21 E^y - T_j^22 E^z]
        {
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 2, 0), E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 2, 1), E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 2, 2), E.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 2, 0), -E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 2, 1), -E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 2, 2), -E.z());

            b(curr_row) = 0.0;
            w(curr_row) = w_e * params.connect_weight;
            curr_row++;
        }
    }
}


void compute_similarity_entries(const reshaping::TransfSolveParams& params,
                                const reshaping::ReshapingData& data,
                                reshaping::TripletsVec& triplets,
                                Eigen::VectorXd& b,
                                Eigen::VectorXd& w,
                                int& curr_row) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    for(int i = 0; i < num_edges; ++i) {
        const int tid0 = adj_e2f.at(i).at(0);
        const int tid1 = adj_e2f.at(i).at(1);

        int vid_k = mesh.vertex_opposite_to_edge(i, tid0);
        int vid_l = mesh.vertex_opposite_to_edge(i, tid1);

        const Eigen::Vector3d& vk = data.orig_vertices.row(vid_k);
        const Eigen::Vector3d& vl = data.orig_vertices.row(vid_l);
        const Eigen::Vector3d E   = (vk - vl).normalized();

        const double w_e = data.similarity_term_edge_w(i);

        //   ||                                      ||2
        //   || T_i e^0_kl/l^0_kl - T_j e^0_kl/l^0_kl||
        //   ||                                      ||
        // 
        //  ||                                                                               ||2
        //  || [T_i^00 E^x + T_i^01 E^y + T_i^02 E^z - T_j^00 E^x - T_j^01 E^y - T_j^02 E^z] ||
        //  || [T_i^10 E^x + T_i^11 E^y + T_i^12 E^z - T_j^10 E^x - T_j^11 E^y - T_j^12 E^z] ||
        //  || [T_i^20 E^x + T_i^21 E^y + T_i^22 E^z - T_j^20 E^x - T_j^21 E^y - T_j^22 E^z] ||
        //  ||                                                                               ||   
        //
        // where E = e^0_kl/l^0_kl

        // First row entries
        //  [T_i^00 E^x + T_i^01 E^y + T_i^02 E^z - T_j^00 E^x - T_j^01 E^y - T_j^02 E^z] 
        {
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 0, 0), E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 0, 1), E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 0, 2), E.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 0, 0), -E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 0, 1), -E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 0, 2), -E.z());

            b(curr_row) = 0.0;
            w(curr_row) = w_e * params.similarity_weight;
            curr_row++;
        }

        // Second row entries
        // [T_i^10 E^x + T_i^11 E^y + T_i^12 E^z - T_j^10 E^x - T_j^11 E^y - T_j^12 E^z]
        {
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 1, 0), E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 1, 1), E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 1, 2), E.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 1, 0), -E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 1, 1), -E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 1, 2), -E.z());

            b(curr_row) = 0.0;
            w(curr_row) = w_e * params.similarity_weight;
            curr_row++;
        }

        // Third row entries
        // [T_i^20 E^x + T_i^21 E^y + T_i^22 E^z - T_j^20 E^x - T_j^21 E^y - T_j^22 E^z]
        {
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 2, 0), E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 2, 1), E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid0, 2, 2), E.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 2, 0), -E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 2, 1), -E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid1, 2, 2), -E.z());

            b(curr_row) = 0.0;
            w(curr_row) = w_e * params.similarity_weight;
            curr_row++;
        }
    }
}


void compute_transform_entries(const reshaping::TransfSolveParams& params,
                               const reshaping::ReshapingData& data,
                               reshaping::TripletsVec& triplets,
                               Eigen::VectorXd& b,
                               Eigen::VectorXd& w,
                               int& curr_row) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    /*
     * |                   |^2  |                   |^2
     * | e_ij - T_i e^0_ij |  + | e_ij - T_j e^0_ij |  
     * |                   |    |                   |  
     */
    for(int e = 0; e < num_edges; ++e) {
        std::array<int, 2> edge_vids = mesh.get_edge_vertices(e);

        const Eigen::Vector3d& curr_v0  = data.curr_vertices.row(edge_vids[0]);
        const Eigen::Vector3d& curr_v1  = data.curr_vertices.row(edge_vids[1]);
        const Eigen::Vector3d curr_edge = curr_v1 - curr_v0;

        const Eigen::Vector3d& orig_edge = data.orig_edges.row(e);

        for(int f = 0; f < 2; ++f) {
            const int fid = adj_e2f.at(e).at(f);

            double w_ij = 1.0 / data.avg_edge_len;

            // x-component 
            // T^00_i E^0_x + T^01_i E^0_y + T^02 E^0_z = E_x
            {
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 0), orig_edge.x());
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 1), orig_edge.y());
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 2), orig_edge.z());

                b(curr_row) = curr_edge.x();
                w(curr_row) = w_ij;
                curr_row++;
            }

            // y-component 
            // T^10_i E^0_x + T^11_i E^0_y + T^12 E^0_z = E_y
            {
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 0), orig_edge.x());
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 1), orig_edge.y());
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 2), orig_edge.z());

                b(curr_row) = curr_edge.y();
                w(curr_row) = w_ij;
                curr_row++;
            }

            // z-component 
            // T^20_i E^0_x + T^21_i E^0_y + T^22 E^0_z = E_z
            {
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 0), orig_edge.x());
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 1), orig_edge.y());
                triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 2), orig_edge.z());

                b(curr_row) = curr_edge.z();
                w(curr_row) = w_ij;
                curr_row++;
            }
        }
    }
}

void compute_normal_entries(const reshaping::TransfSolveParams& params,
                            const reshaping::ReshapingData& data,
                            reshaping::TripletsVec& triplets,
                            Eigen::VectorXd& b,
                            Eigen::VectorXd& w,
                            int& curr_row) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    for(int e = 0; e < num_edges; ++e) {
        const Eigen::Vector3d E = data.orig_edges.row(e).normalized();

        const double w_e = 1.0;

        /*
         * ((T_i e^0_ij) . n_i)^2 + ((T_j e^0_ij) . n_j)^2
         */
        for(int f = 0; f < 2; ++f) {
            const int fid = adj_e2f.at(e).at(f);
            const int tid = fid;
            const Eigen::Vector3d& N = data.orig_tri_N.row(fid);

            // ( 
            //      n^x E_x T^{00} + n^x E_y T^{01} + n^x E_z T^{02} +
            //      n^y E_x T^{10} + n^y E_y T^{11} + n^y E_z T^{12} +
            //      n^z E_x T^{20} + n^z E_y T^{21} + n^z E_z T^{22} +
            // )^2

            // n^x E_x T^{00} + n^x E_y T^{01} + n^x E_z T^{02} +
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 0, 0), N.x() * E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 0, 1), N.x() * E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 0, 2), N.x() * E.z());

            // n^y E_x T^{10} + n^y E_y T^{11} + n^y E_z T^{12} +
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 1, 0), N.y() * E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 1, 1), N.y() * E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 1, 2), N.y() * E.z());

            // n^z E_x T^{20} + n^z E_y T^{21} + n^z E_z T^{22} +
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 2, 0), N.z() * E.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 2, 1), N.z() * E.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(tid, 2, 2), N.z() * E.z());

            b(curr_row) = 0.0;
            w(curr_row) = params.normal_weight * w_e;
            curr_row++;
        }
    }
}

void compute_sphericity_entries(const reshaping::TransfSolveParams& params,
                                const reshaping::ReshapingData& data,
                                reshaping::TripletsVec& triplets,
                                Eigen::VectorXd& b,
                                Eigen::VectorXd& w,
                                int& curr_row) {

    for(const auto& info : data.sphericity_terms_info) {
        const int fid            = info.fid;
        const int vid_i          = info.vid_i;
        const int vid_j          = info.vid_j;
        const int vid_k          = info.vid_k;
        const double face_weight = info.w;
        const Eigen::Matrix3d& R = info.R;

        const Eigen::Vector3d Eij = (data.orig_vertices.row(vid_j) -
                                     data.orig_vertices.row(vid_i)).normalized();

        const Eigen::Vector3d Eik = (data.orig_vertices.row(vid_k) -
                                     data.orig_vertices.row(vid_i)).normalized();

        // x-component equations
        // [T E_ij]_x - [s R T E_ik]_x
        {
            // E_ij_x
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 0), Eij.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 1), Eij.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 2), Eij.z());

            // [s R T E_ik]_x
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 0), -R(0, 0) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 1), -R(0, 0) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 2), -R(0, 0) * Eik.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 0), -R(0, 1) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 1), -R(0, 1) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 2), -R(0, 1) * Eik.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 0), -R(0, 2) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 1), -R(0, 2) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 2), -R(0, 2) * Eik.z());

            b(curr_row) = 0.0;
            w(curr_row) = params.sphericity_term_weight * face_weight;
            curr_row++;
        }

        // y-component equations
        // [T E_ij]_y - [s R T E_ik]_y
        {
            // E_ij_y
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 0), Eij.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 1), Eij.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 2), Eij.z());

            // [s R T E_ik]_y
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 0), -R(1, 0) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 1), -R(1, 0) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 2), -R(1, 0) * Eik.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 0), -R(1, 1) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 1), -R(1, 1) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 2), -R(1, 1) * Eik.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 0), -R(1, 2) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 1), -R(1, 2) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 2), -R(1, 2) * Eik.z());

            b(curr_row) = 0.0;
            w(curr_row) = params.sphericity_term_weight * face_weight;
            curr_row++;
        }

        // z-component equations
        // [T E_ij]_z - [s R T E_ik]_z
        {
            // E_ij_y
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 0), Eij.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 1), Eij.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 2), Eij.z());

            // [s R T E_ik]_y
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 0), -R(2, 0) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 1), -R(2, 0) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 0, 2), -R(2, 0) * Eik.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 0), -R(2, 1) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 1), -R(2, 1) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 1, 2), -R(2, 1) * Eik.z());

            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 0), -R(2, 2) * Eik.x());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 1), -R(2, 2) * Eik.y());
            triplets.emplace_back(curr_row, matrix_to_sys_idx(fid, 2, 2), -R(2, 2) * Eik.z());

            b(curr_row) = 0.0;
            w(curr_row) = params.sphericity_term_weight * face_weight;
            curr_row++;
        }
    }
}

void compute_regularizer_entries(const reshaping::TransfSolveParams& params,
                                 const reshaping::ReshapingData& data,
                                 reshaping::TripletsVec& triplets,
                                 Eigen::VectorXd& b,
                                 Eigen::VectorXd& w,
                                 int& curr_row) {
    const int num_tris = data.mesh.get_num_facets();

    for(int t = 0; t < num_tris; ++t) {
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 3; ++j) {
                triplets.emplace_back(curr_row, matrix_to_sys_idx(t, i, j),  1.0);

                b(curr_row) = i == j ? 1.0 : 0.0;
                w(curr_row) = params.regularizer_weight;
                curr_row++;
            }
        }
    }
}

void assemble_transformation_solve_system(const reshaping::TransfSolveParams& params,
                                          const reshaping::ReshapingData& data,
                                          Eigen::SparseMatrix<double>& A,
                                          Eigen::VectorXd& b,
                                          Eigen::VectorXd& w) {

    Eigen::Vector2i sys_size = system_size(data);

    reshaping::TripletsVec all_triplets;
    b.resize(sys_size.x());
    w.resize(sys_size.x());
    A.resize((Eigen::Index) sys_size.x(), (Eigen::Index) sys_size.y());

    int curr_row = 0;
    compute_connect_entries   (params, data, all_triplets, b, w, curr_row);
    compute_similarity_entries(params, data, all_triplets, b, w, curr_row);
    compute_transform_entries (params, data, all_triplets, b, w, curr_row);
    compute_normal_entries    (params, data, all_triplets, b, w, curr_row);
#if SPHERICITY_ON
    compute_sphericity_entries (params, data, all_triplets, b, w, curr_row);
#endif
    compute_regularizer_entries(params, data, all_triplets, b, w, curr_row);

    A.setFromTriplets(all_triplets.begin(), all_triplets.end());
}

}

namespace reshaping {

bool solve_for_transformations(const TransfSolveParams& params,
                               ReshapingData& data,
                               std::vector<Eigen::Matrix3d>& out_T) {
    Eigen::SparseMatrix<double> A;
    Eigen::VectorXd b;
    Eigen::VectorXd w;
    assemble_transformation_solve_system(params, data, A, b, w);

    const Eigen::Transpose<Eigen::SparseMatrix<double>> At = A.transpose();
    const Eigen::DiagonalWrapper<const Eigen::VectorXd> W  = w.asDiagonal();
    Eigen::SparseMatrix<double> AtWA = At * W * A;
    Eigen::VectorXd AtWb             = At * W * b;

    bool needs_prefactorization = data.iter == 0;
    if(needs_prefactorization)
        data.transf_solver.analyzePattern(AtWA);

    data.transf_solver.factorize(AtWA);

    // Check whether the decomposition has failed
    bool decomposition_succ = data.transf_solver.info() == Eigen::Success;
    if(!decomposition_succ) {
        LOGGER.error("Error while performing Cholesky decomposition in the transformation solver ({})",
                                      data.transf_solver.info());
        assert(false);
    }

    // Computing solution
    Eigen::VectorXd sol = data.transf_solver.solve(AtWb);

    const int num_tris = data.mesh.get_num_facets();
    for(int t = 0; t < num_tris; ++t) {
        // Retreiving each matrix's value
        for(int r = 0; r < 3; r++)
            for(int c = 0; c < 3; c++)
                out_T.at(t)(r, c) = sol(matrix_to_sys_idx(t, r, c));
    }

    bool solution_succ = data.vertex_solver.info() == Eigen::Success;
    if(!solution_succ) {
        LOGGER.error("Error while performing Cholesky::solve in the Transformation-solver");
        assert(false); // TODO: use release-compatible assert
    }

    return decomposition_succ && solution_succ;
}

}
#include <mesh_reshaping/vertex_solve.h>
#include <mesh_reshaping/solve_utils.h>
#include <mesh_reshaping/globals.h>

#include <Eigen/Core>

#include <vector>

namespace {

Eigen::Vector2i system_size(const reshaping::ReshapingData& data,
                            const bool handle_error_distrib_on) {
    const int num_verts = data.mesh.get_num_vertices();
    const int num_edges = data.mesh.get_num_edges();

    int num_cols = num_verts * 3;
    int num_rows = 0;

    // Edge term
    num_rows += num_edges * 3 * 2;

    // Normal term
    num_rows += num_edges * 2;

    // Straightness term
    num_rows += data.num_straight_pairs * 3;

    // Sphericity term
#if SPHERICITY_ON
    num_rows += (int) data.sphericity_terms_info.size() * 3;
#endif

    // Regularizer term
    num_rows += num_edges * 3;

    // Positional constraints
    num_rows += (int) data.bc.size() * 3;

    // Extra positional constraints to hold vertices in place
    // for "handle error distribution"
    if(handle_error_distrib_on) {
        int num_verts_hold = (int) data.handle_error_dist_out_verts.size();
        num_rows += num_verts_hold * 3;
    }

    return Eigen::Vector2i(num_rows, num_cols);
}

void compute_edge_term_entries(const reshaping::VertexSolveParams& params,
                               const reshaping::ReshapingData& data,
                               reshaping::TripletsVec& triplets,
                               Eigen::VectorXd& b,
                               Eigen::VectorXd& w,
                               int& curr_row) {

    using VERTEX_COMP = reshaping::VERTEX_COMP;

    const auto& mesh    = data.mesh;
    const int num_verts = mesh.get_num_vertices();
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    /*
     * |                                    |^2  |                                    |^2
     * | T^-1_i e_ij/l^0_ij - e^0_ij/l^0_ij |  + | T^-1_j e_ij/l^0_ij - e^0_ij/l^0_ij | 
     * |                                    |    |                                    |
     */
    for(int e = 0; e < num_edges; ++e) {
        const std::array<int, 2> edge_vids = mesh.get_edge_vertices(e);
        const int vid0 = edge_vids.at(0);
        const int vid1 = edge_vids.at(1);

        const Eigen::Vector3d& E = data.orig_edges.row(e);
        const double orig_len    = data.orig_edge_lens(e);
        const double target_len  = data.target_edge_lens(e);

        const double den = orig_len;

        const double w_ij = data.length_based_edge_w(e);

        for(int f = 0; f < 2; ++f) {
            const int fid = adj_e2f.at(e).at(f);

            const Eigen::Matrix3d& T = data.curr_tri_T.at(fid);
            const Eigen::Matrix3d T_inv = T.inverse();

            {
                // terms for x-component
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::X, num_verts),  T_inv(0, 0) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::X, num_verts), -T_inv(0, 0) / den);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Y, num_verts),  T_inv(0, 1) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Y, num_verts), -T_inv(0, 1) / den);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Z, num_verts),  T_inv(0, 2) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Z, num_verts), -T_inv(0, 2) / den);

                b(curr_row) = E.x() / orig_len;
                w(curr_row) = w_ij;
                curr_row++;
            }

            {
                // terms for y-component
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::X, num_verts),  T_inv(1, 0) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::X, num_verts), -T_inv(1, 0) / den);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Y, num_verts),  T_inv(1, 1) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Y, num_verts), -T_inv(1, 1) / den);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Z, num_verts),  T_inv(1, 2) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Z, num_verts), -T_inv(1, 2) / den);

                b(curr_row) = E.y() / orig_len;
                w(curr_row) = w_ij;
                curr_row++;
            }

            {
                // terms for z-component
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::X, num_verts),  T_inv(2, 0) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::X, num_verts), -T_inv(2, 0) / den);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Y, num_verts),  T_inv(2, 1) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Y, num_verts), -T_inv(2, 1) / den);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Z, num_verts),  T_inv(2, 2) / den);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Z, num_verts), -T_inv(2, 2) / den);

                b(curr_row) = E.z() / orig_len;
                w(curr_row) = w_ij;
                curr_row++;
            }
        }
    }
}

void compute_normal_entries(const reshaping::VertexSolveParams& params,
                            const reshaping::ReshapingData& data,
                            reshaping::TripletsVec& triplets,
                            Eigen::VectorXd& b,
                            Eigen::VectorXd& w,
                            int& curr_row) {

    using VERTEX_COMP = reshaping::VERTEX_COMP;

    const auto& mesh    = data.mesh;
    const int num_verts = mesh.get_num_vertices();
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    /*
     * CHANGED TO
     *      weight * (l^0_ij / L_avg) (n^i . e_(ij) / l_ij)^2 ... same for n^j
     */
    double normal_weight = params.normal_weight;
    for(int e = 0; e < num_edges; ++e) {
        const std::array<int, 2> edge_vids = mesh.get_edge_vertices(e);
        const int vid0 = edge_vids.at(0);
        const int vid1 = edge_vids.at(1);

        const double orig_len = data.orig_edge_lens(e);
        const double target_len = data.target_edge_lens(e);

        const auto& adj_faces = adj_e2f.at(e);
        assert(adj_faces.size() == 2 && "Non-manifold edge was found!");

        const double w_ij = normal_weight *
                            data.length_based_edge_w(e) *
                            (orig_len / data.avg_edge_len);

        for(int adj_tid : adj_faces) {
            const Eigen::Vector3d& n = data.orig_tri_N.row(adj_tid);

            // n^x_{ij} l^0/l_ij * e^x_{ij}
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::X, num_verts),  n.x() / target_len);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::X, num_verts), -n.x() / target_len);

            // n^y_{ij} l^0/l_ij * e^y_{ij}
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Y, num_verts),  n.y() / target_len);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Y, num_verts), -n.y() / target_len);

            // n^z_{ij} l^0/l_ij * e^z_{ij}
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Z, num_verts),  n.z() / target_len);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Z, num_verts), -n.z() / target_len);

            b(curr_row) = 0.0;
            w(curr_row) = w_ij;
            curr_row++;
        }
    }
}

void compute_straightness_entries(const reshaping::VertexSolveParams& params,
                                  const reshaping::ReshapingData& data,
                                  reshaping::TripletsVec& triplets,
                                  Eigen::VectorXd& b,
                                  Eigen::VectorXd& w,
                                  int& curr_row) {

    using VERTEX_COMP = reshaping::VERTEX_COMP;

    const auto& mesh = data.mesh;
    const auto& straight_chains = data.straight_chains;
    const int num_verts = mesh.get_num_vertices();

    if(data.num_straight_pairs == 0)
        return;

    for(int c = 0; c < straight_chains->num_chains(); ++c) {
        const auto& chain = straight_chains->get_chain(c);

        if(chain.size() < 3)
            continue;

        for(int i = 1; i < chain.size() - 1; ++i) {
            int vj = chain.at(i - 1);
            int vi = chain.at(i    );
            int vk = chain.at(i + 1);

            int eid_ij = mesh.get_edge_index(vi, vj);
            int eid_ki = mesh.get_edge_index(vk, vi);

            double l_ij = data.target_edge_lens(eid_ij);
            double l_ki = data.target_edge_lens(eid_ki);

            double inv_l_ij = 1.0 / l_ij;
            double inv_l_ki = 1.0 / l_ki;

            double w_ijk = params.straightness_weight;

            // x-component
            {
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vj, VERTEX_COMP::X, num_verts),  inv_l_ij);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::X, num_verts), -inv_l_ij);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::X, num_verts), -inv_l_ki);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::X, num_verts),  inv_l_ki);

                b(curr_row) = 0.0;
                w(curr_row) = w_ijk;
                curr_row++;
            }

            // y-component
            {
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vj, VERTEX_COMP::Y, num_verts),  inv_l_ij);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Y, num_verts), -inv_l_ij);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Y, num_verts), -inv_l_ki);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Y, num_verts),  inv_l_ki);

                b(curr_row) = 0.0;
                w(curr_row) = w_ijk;
                curr_row++;
            }

            // z-component
            {
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vj, VERTEX_COMP::Z, num_verts),  inv_l_ij);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Z, num_verts), -inv_l_ij);

                triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Z, num_verts), -inv_l_ki);
                triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Z, num_verts),  inv_l_ki);

                b(curr_row) = 0.0;
                w(curr_row) = w_ijk;
                curr_row++;
            }
        }
    }
}

void compute_sphericity_entries(const reshaping::VertexSolveParams& params,
                                const reshaping::ReshapingData& data,
                                reshaping::TripletsVec& triplets,
                                Eigen::VectorXd& b,
                                Eigen::VectorXd& w,
                                int& curr_row) {
    using VERTEX_COMP = reshaping::VERTEX_COMP;

    const auto& mesh    = data.mesh;
    const int num_verts = mesh.get_num_vertices();

    for(const auto& info : data.sphericity_terms_info) {
        const int vi  = info.vid_i;
        const int vj  = info.vid_j;
        const int vk  = info.vid_k;
        const double face_weight = info.w;
        const Eigen::Matrix3d& R = info.R;

        int eid_ij = mesh.get_edge_index(vi, vj);
        int eid_ik = mesh.get_edge_index(vi, vk);

        const double orig_len_ij = data.orig_edge_lens(eid_ij);
        const double orig_len_ik = data.orig_edge_lens(eid_ik);

        // x-component equations
        // [E_ij]_x - [s R E_ik]_x
        {
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vj, VERTEX_COMP::X, num_verts),  1.0 / orig_len_ij);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::X, num_verts), -1.0 / orig_len_ij);

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::X, num_verts), -(1.0 / orig_len_ik) * R(0, 0));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::X, num_verts),  (1.0 / orig_len_ik) * R(0, 0));

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Y, num_verts), -(1.0 / orig_len_ik) * R(0, 1));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Y, num_verts),  (1.0 / orig_len_ik) * R(0, 1));

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Z, num_verts), -(1.0 / orig_len_ik) * R(0, 2));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Z, num_verts),  (1.0 / orig_len_ik) * R(0, 2));

            b(curr_row) = 0.0;
            w(curr_row) = face_weight * params.sphericity_weight;
            curr_row++;
        }

        // y-component equations
        // [E_ij]_y - [s R E_ik]_y
        {
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vj, VERTEX_COMP::Y, num_verts),  1.0 / orig_len_ij);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Y, num_verts), -1.0 / orig_len_ij);

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::X, num_verts), -(1.0 / orig_len_ik) * R(1, 0));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::X, num_verts),  (1.0 / orig_len_ik) * R(1, 0));

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Y, num_verts), -(1.0 / orig_len_ik) * R(1, 1));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Y, num_verts),  (1.0 / orig_len_ik) * R(1, 1));

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Z, num_verts), -(1.0 / orig_len_ik) * R(1, 2));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Z, num_verts),  (1.0 / orig_len_ik) * R(1, 2));

            b(curr_row) = 0.0;
            w(curr_row) = face_weight * params.sphericity_weight;
            curr_row++;
        }

        // z-component equations
        // [E_ij]_z - [s R E_ik]_z
        {
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vj, VERTEX_COMP::Z, num_verts),  1.0 / orig_len_ij);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Z, num_verts), -1.0 / orig_len_ij);

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::X, num_verts), -(1.0 / orig_len_ik) * R(2, 0));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::X, num_verts),  (1.0 / orig_len_ik) * R(2, 0));

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Y, num_verts), -(1.0 / orig_len_ik) * R(2, 1));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Y, num_verts),  (1.0 / orig_len_ik) * R(2, 1));

            triplets.emplace_back(curr_row, vertex_to_sys_idx(vk, VERTEX_COMP::Z, num_verts), -(1.0 / orig_len_ik) * R(2, 2));
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vi, VERTEX_COMP::Z, num_verts),  (1.0 / orig_len_ik) * R(2, 2));

            b(curr_row) = 0.0;
            w(curr_row) = face_weight * params.sphericity_weight;
            curr_row++;
        }
    }
}

void compute_bc_entries(const reshaping::VertexSolveParams& params,
                        const reshaping::ReshapingData& data,
                        reshaping::TripletsVec& triplets,
                        Eigen::VectorXd& b,
                        Eigen::VectorXd& w,
                        int& curr_row) {
    using VERTEX_COMP = reshaping::VERTEX_COMP;

    const int num_verts = data.mesh.get_num_vertices();

    double bc_weight = params.bc_weight;
    if(params.handle_error_distrib.is_active)
        bc_weight = params.handle_error_distrib.weakened_hc_weight;

    for(const auto& [vid, pos] : data.bc) {
        for(int d = 0; d < 3; ++d) {
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid, (VERTEX_COMP) d, num_verts), 1.0);

            b(curr_row) = pos(d);
            w(curr_row) = bc_weight;

            curr_row++;
        }
    }

    // Hold every vertex off-ring (including boundary)
    if(params.handle_error_distrib.is_active) {
        for(int vid : data.handle_error_dist_out_verts) {
            const Eigen::Vector3d& v = data.curr_vertices.row(vid);

            for(int d = 0; d < 3; ++d) {
                triplets.emplace_back(curr_row,
                                      vertex_to_sys_idx(vid, (VERTEX_COMP) d, num_verts),
                                      1.0);

                b(curr_row) = v(d);
                w(curr_row) = params.handle_error_distrib.off_zone_hc_weight;

                curr_row++;
            }
        }
    }
}


void compute_regularizer_entries(const reshaping::VertexSolveParams& params,
                                 const reshaping::ReshapingData& data,
                                 reshaping::TripletsVec& triplets,
                                 Eigen::VectorXd& b,
                                 Eigen::VectorXd& w,
                                 int& curr_row) {
    using VERTEX_COMP = reshaping::VERTEX_COMP;

    const auto& mesh    = data.mesh;
    const int num_verts = mesh.get_num_vertices();
    const int num_edges = mesh.get_num_edges();
    const double weight = params.regularizer_weight *
                          (1.0 / data.avg_edge_len);

    for(int eid = 0; eid < num_edges; ++eid) {
        const auto edge_vids = mesh.get_edge_vertices(eid);

        const int vid0 = edge_vids.at(0);
        const int vid1 = edge_vids.at(1);

        const Eigen::Vector3d& orig_E = data.orig_edges.row(eid);

        // x-component
        {
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::X, num_verts),  1.0);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::X, num_verts), -1.0);

            b(curr_row) = orig_E.x();
            w(curr_row) = weight;
            curr_row++;
        }

        // y-component
        {
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Y, num_verts),  1.0);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Y, num_verts), -1.0);

            b(curr_row) = orig_E.y();
            w(curr_row) = weight;
            curr_row++;
        }

        // z-component
        {
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid1, VERTEX_COMP::Z, num_verts),  1.0);
            triplets.emplace_back(curr_row, vertex_to_sys_idx(vid0, VERTEX_COMP::Z, num_verts), -1.0);

            b(curr_row) = orig_E.z();
            w(curr_row) = weight;
            curr_row++;
        }
    }
}

void assemble_vertex_solve_system(const reshaping::VertexSolveParams& params,
                                  const reshaping::ReshapingData& data,
                                  Eigen::SparseMatrix<double>& A,
                                  Eigen::VectorXd& b,
                                  Eigen::VectorXd& w) {
                                  
    bool is_error_distrib_active = params.handle_error_distrib.is_active;
    Eigen::Vector2i sys_size = system_size(data, is_error_distrib_active);

    reshaping::TripletsVec all_triplets;
    b.resize(sys_size.x());
    w.resize(sys_size.x());
    A.resize((Eigen::Index) sys_size.x(), (Eigen::Index) sys_size.y());

    int curr_row = 0;
    compute_edge_term_entries   (params, data, all_triplets, b, w, curr_row);
    compute_normal_entries      (params, data, all_triplets, b, w, curr_row);
    compute_straightness_entries(params, data, all_triplets, b, w, curr_row);
#if SPHERICITY_ON
    compute_sphericity_entries  (params, data, all_triplets, b, w, curr_row);
#endif
    compute_bc_entries          (params, data, all_triplets, b, w, curr_row);
    compute_regularizer_entries (params, data, all_triplets, b, w, curr_row);

    A.setFromTriplets(all_triplets.begin(), all_triplets.end());
}

}

namespace reshaping {

bool solve_for_vertices(const VertexSolveParams& params,
                        ReshapingData& data,
                        Eigen::MatrixXd& outV) {
    Eigen::SparseMatrix<double> A;
    Eigen::VectorXd b;
    Eigen::VectorXd w;
    assemble_vertex_solve_system(params, data, A, b, w);

    const Eigen::Transpose<Eigen::SparseMatrix<double>> At = A.transpose();
    const Eigen::DiagonalWrapper<const Eigen::VectorXd> W  = w.asDiagonal();
    Eigen::SparseMatrix<double> AtWA = At * W * A;
    Eigen::VectorXd AtWb             = At * W * b;

    bool needs_prefactorization = data.iter == 0;
    if(needs_prefactorization)
        data.vertex_solver.analyzePattern(AtWA);

    data.vertex_solver.factorize(AtWA);

    // Check whether the decomposition has failed
    bool decomposition_succ = data.vertex_solver.info() == Eigen::Success;
    if(!decomposition_succ) {
        LOGGER.error("Error while performing Cholesky decomposition in the Vertex-Solver ({})",
                                      data.vertex_solver.info());
        assert(false && "Error while performing Cholesky decomposition in the Vertex-solver");
    }

    // Computing solution
    Eigen::VectorXd sol = data.vertex_solver.solve(AtWb);

    const int num_verts = data.mesh.get_num_vertices();
    for(int v = 0; v < num_verts; ++v) {
        outV.row(v) << sol(num_verts * 0 + v),
                       sol(num_verts * 1 + v),
                       sol(num_verts * 2 + v);
    }

    bool solution_succ = data.vertex_solver.info() == Eigen::Success;
    if(!solution_succ) {
        LOGGER.error("Error while performing Cholesky::solve in the vertex solve");
        assert(false); // TODO: use release-compatible assert
    }

    return decomposition_succ && solution_succ;
}

}
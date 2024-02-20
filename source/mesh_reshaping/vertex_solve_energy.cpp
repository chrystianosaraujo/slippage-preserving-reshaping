#include <mesh_reshaping/vertex_solve_energy.h>

#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/types.h>

namespace {

double compute_edge_term_cost(const reshaping::ReshapingData& data,
                              const Eigen::MatrixXd& V) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    double cost = 0.0;
    for(int e = 0; e < num_edges; ++e) {
        const std::array<int, 2> edge_vids = mesh.get_edge_vertices(e);
        const int vid0 = edge_vids.at(0);
        const int vid1 = edge_vids.at(1);

        const Eigen::Vector3d& curr_v0 = V.row(vid0);
        const Eigen::Vector3d& curr_v1 = V.row(vid1);

        const Eigen::Vector3d& orig_E = data.orig_edges.row(e);
        const Eigen::Vector3d curr_E = curr_v1 - curr_v0;

        const double orig_len = data.orig_edge_lens(e);
        const double curr_len = data.curr_edge_lens(e);

        const double w_ij = data.length_based_edge_w(e);
        for(int f = 0; f < 2; ++f) {
            int fid = adj_e2f.at(e).at(f);

            const Eigen::Matrix3d& T = data.curr_tri_T.at(fid);
            const Eigen::Matrix3d T_inv = T.inverse();

            cost += w_ij * (((T_inv * curr_E) / orig_len) - (orig_E / orig_len)).squaredNorm();
        }
    }

    return cost;
}

double compute_normal_term_cost(const reshaping::ReshapingData& data,
                                const Eigen::MatrixXd& V) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    double cost = 0.0;
    for(int e = 0; e < num_edges; ++e) {
        const std::array<int, 2> edge_vids = mesh.get_edge_vertices(e);
        const int vid0 = edge_vids.at(0);
        const int vid1 = edge_vids.at(1);

        const Eigen::Vector3d& curr_v0 = V.row(vid0);
        const Eigen::Vector3d& curr_v1 = V.row(vid1);

        const Eigen::Vector3d curr_E = curr_v1 - curr_v0;

        const double orig_len = data.orig_edge_lens(e);
        const double curr_len = data.curr_edge_lens(e);

        const double w_ij = data.length_based_edge_w(e) *
                            (orig_len / data.avg_edge_len);

        const auto& adj_faces = adj_e2f.at(e);
        for(int adj_tid : adj_faces) {
            const Eigen::Vector3d& n = data.orig_tri_N.row(adj_tid);

            cost += w_ij * pow(n.dot(curr_E / curr_len), 2.0);
        }
    }

    return cost;
}

double compute_sphericity_term_cost(const reshaping::ReshapingData& data,
                                    const Eigen::MatrixXd& V) {
    const auto& mesh = data.mesh;

    double cost = 0.0;
    for(const auto& info : data.sphericity_terms_info) {
        const int vi = info.vid_i;
        const int vj = info.vid_j;
        const int vk = info.vid_k;

        const double face_weight = info.w;
        const Eigen::Matrix3d& R = info.R;

        int eid_ij = mesh.get_edge_index(vi, vj);
        int eid_ik = mesh.get_edge_index(vi, vk);

        const double orig_len_ij = data.orig_edge_lens(eid_ij);
        const double orig_len_ik = data.orig_edge_lens(eid_ik);

        Eigen::Vector3d Eij = (V.row(vj) - V.row(vi));
        Eigen::Vector3d Eik = (V.row(vk) - V.row(vi));

        cost += face_weight * (Eij/orig_len_ij - R * (Eik/orig_len_ik)).squaredNorm();
    }

    return cost;
}

double compute_straightness_cost(const reshaping::ReshapingData& data,
                                 const Eigen::MatrixXd& V) {
    const auto& mesh = data.mesh;
    const auto& straight_chains = data.straight_chains;

    double cost = 0.0;
    if(!straight_chains)
        return cost;

    for(int c = 0; c < straight_chains->num_chains(); ++c) {
        const auto& chain = straight_chains->get_chain(c);

        if(chain.size() < 3)
            continue;

        for(int i = 1; i < chain.size() - 1; ++i) {
            int vid_j = chain.at(i - 1);
            int vid_i = chain.at(i);
            int vid_k = chain.at(i + 1);

            int eid_ij = mesh.get_edge_index(vid_i, vid_j);
            int eid_ki = mesh.get_edge_index(vid_k, vid_i);

            double l_ij = data.curr_edge_lens(eid_ij);
            double l_ki = data.curr_edge_lens(eid_ki);

            const Eigen::Vector3d& vj = V.row(vid_j);
            const Eigen::Vector3d& vi = V.row(vid_i);
            const Eigen::Vector3d& vk = V.row(vid_k);

            const Eigen::Vector3d edge_ij = (vj - vi);
            const Eigen::Vector3d edge_ki = (vi - vk);

            cost += (edge_ij/l_ij - edge_ki/l_ki).squaredNorm();
        }
    }

    return cost;
}

double compute_constraints_cost(const reshaping::ReshapingData& data,
                                const Eigen::MatrixXd& V) {
    double cost = 0.0;

    for(const auto& [vid, target_p] : data.bc) {
        const Eigen::Vector3d& curr_p = V.row(vid);
        cost += (target_p - curr_p).squaredNorm();
    }

    return cost;
}

double compute_scale_term_cost(const reshaping::ReshapingData& data,
                               const Eigen::MatrixXd& V) {
    const auto& mesh = data.mesh;
    const auto& num_edges = mesh.get_num_edges();

    const double weight = (1.0 / data.avg_edge_len);

    double cost = 0.0;
    for(int eid = 0; eid < num_edges; ++eid) {
        const auto edge_vids = mesh.get_edge_vertices(eid);

        const int vid0 = edge_vids.at(0);
        const int vid1 = edge_vids.at(1);

        const Eigen::Vector3d& orig_E = data.orig_edges.row(eid);
        const Eigen::Vector3d  curr_E = V.row(vid1) - V.row(vid0);

        cost += weight *  (curr_E - orig_E).squaredNorm();
    }

    return cost;
}

}

namespace reshaping {

VertexSolveEnergy compute_vertex_solve_energy(const VertexSolveParams& params,
                                              const ReshapingData& data,
                                              const Eigen::MatrixXd& V) {
    VertexSolveEnergy energy;

    energy.edge_cost        = compute_edge_term_cost(data, V);
    energy.normal_cost      = compute_normal_term_cost(data, V);
    energy.sphericity_cost  = compute_sphericity_term_cost(data, V);
    energy.straight_cost    = compute_straightness_cost(data, V);
    energy.constraints_cost = compute_constraints_cost(data, V);
    energy.scale_cost       = compute_scale_term_cost(data, V);

    energy.total_cost = params.edge_weight         * energy.edge_cost       +
                        params.normal_weight       * energy.normal_cost     +
                        params.straightness_weight * energy.straight_cost   +
                        params.sphericity_weight   * energy.sphericity_cost +
                        params.regularizer_weight  * energy.scale_cost      +
                        params.bc_weight           * energy.constraints_cost;

    return energy;
}

}
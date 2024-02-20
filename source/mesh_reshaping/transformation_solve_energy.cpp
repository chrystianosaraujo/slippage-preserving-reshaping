#include <mesh_reshaping/transformation_solve_energy.h>

#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/types.h>

namespace {
double compute_transform_term_cost(const reshaping::ReshapingData& data,
                                   const Eigen::MatrixXd& V) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    double cost = 0.0;
    for(int e = 0; e < num_edges; ++e) {
        std::array<int, 2> edge_vids = mesh.get_edge_vertices(e);

        const Eigen::Vector3d E = data.orig_edges.row(e).normalized();

        const double w_e = 1.0;

        /*
         * ((T_i e^0_ij) . n_i)^2 + ((T_j e^0_ij) . n_j)^2
         */
        for(int f = 0; f < 2; ++f) {
            const int fid = adj_e2f.at(e).at(f);
            const Eigen::Matrix3d& Ti = data.curr_tri_T.at(fid);
            const Eigen::Vector3d& N  = data.orig_tri_N.row(fid);

            cost += w_e * pow((Ti * E).dot(N), 2.0);
        }
    }

    return cost;
}

double compute_connect_term_cost(const reshaping::ReshapingData& data,
                                 const Eigen::MatrixXd& V) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    double cost = 0.0;

    // ||                        ||2
    // || T_i e^0_ij - T_j e^0_ij||
    // ||                        ||
    for(int e = 0; e < num_edges; ++e) {
        const int tid0 = adj_e2f.at(e).at(0);
        const int tid1 = adj_e2f.at(e).at(1);

        const Eigen::Matrix3d& T0 = data.curr_tri_T.at(tid0);
        const Eigen::Matrix3d& T1 = data.curr_tri_T.at(tid1);

        const Eigen::Vector3d E = data.orig_edges.row(e).normalized();

        double edge_cost = (T0 * E - T1 * E).squaredNorm();

        cost += edge_cost;
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
        std::array<int, 2> edge_vids = mesh.get_edge_vertices(e);

        const Eigen::Vector3d E = data.orig_edges.row(e).normalized();

        const double w_e = 1.0;

        /*
         * ((T_i e^0_ij) . n_i)^2 + ((T_j e^0_ij) . n_j)^2
         */
        for(int f = 0; f < 2; ++f) {
            const int fid = adj_e2f.at(e).at(f);
            const Eigen::Matrix3d& Ti = data.curr_tri_T.at(fid);
            const Eigen::Vector3d& N  = data.orig_tri_N.row(fid);

            cost += w_e * pow((Ti * E).dot(N), 2.0);
        }
    }

    return cost;
}

double compute_sphericity_term_cost(const reshaping::ReshapingData& data,
                                    const Eigen::MatrixXd& V) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    double cost = 0.0;

    if(data.sphericity_terms_info.size() == 0)
        return cost;

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

        const Eigen::Matrix3d& T = data.curr_tri_T.at(fid);

        cost += face_weight * (T * Eij - R * T * Eik).squaredNorm();
    }

    return cost;
}

double compute_similarity_term_cost(const reshaping::ReshapingData& data,
                                    const Eigen::MatrixXd& V) {
    const auto& mesh    = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    // ||                                        ||2
    // || T_i e^0_kl/l^0_kl  - T_j e^0_kl/l^0_kl ||
    // ||                                        ||
    double cost = 0.0;
    for(int e = 0; e < num_edges; ++e) {
        const int tid0 = adj_e2f.at(e).at(0);
        const int tid1 = adj_e2f.at(e).at(1);

        const Eigen::Matrix3d& T0 = data.curr_tri_T.at(tid0);
        const Eigen::Matrix3d& T1 = data.curr_tri_T.at(tid1);

        int vid_k = mesh.vertex_opposite_to_edge(e, tid0);
        int vid_l = mesh.vertex_opposite_to_edge(e, tid1);

        const Eigen::Vector3d& vk = data.orig_vertices.row(vid_k);
        const Eigen::Vector3d& vl = data.orig_vertices.row(vid_l);
        const Eigen::Vector3d E   = (vk - vl).normalized();

        const double w_e = data.similarity_term_edge_w(e);
        double edge_cost = w_e * (T0 * E - T1 * E).squaredNorm();

        cost += edge_cost;
    }

    return cost;
}

double compute_regularizer_term_cost(const reshaping::ReshapingData& data,
                                     const Eigen::MatrixXd& V) {
    const auto& mesh    = data.mesh;
    const int num_tris  = mesh.get_num_facets();
    const auto& adj_e2f = mesh.get_edge_face_adjacency();

    const Eigen::Matrix3d I = Eigen::Matrix3d::Identity();

    double cost = 0.0;
    for(int t = 0; t < num_tris; ++t) {
        const Eigen::Matrix3d& T = data.curr_tri_T.at(t);

        cost += (T - I).squaredNorm();
    }

    return cost;
}

}

namespace reshaping {

TransfSolveEnergy compute_transf_solve_energy(const TransfSolveParams& params,
                                              const ReshapingData& data,
                                              const Eigen::MatrixXd& V) {
    TransfSolveEnergy energy;
    energy.transform_cost   = compute_transform_term_cost(data, V);
    energy.connect_cost     = compute_connect_term_cost(data, V);
    energy.normal_cost      = compute_normal_term_cost(data, V);
    energy.sphericity_cost  = compute_sphericity_term_cost(data, V);
    energy.similarity_cost  = compute_similarity_term_cost(data, V);
    energy.regularizer_cost = compute_regularizer_term_cost(data, V);

    energy.total_cost = energy.transform_cost  +
                        energy.connect_cost    +
                        energy.sphericity_cost +
                        energy.similarity_cost +
                        params.normal_weight      * energy.normal_cost +
                        params.regularizer_weight * energy.regularizer_cost;

    return energy;
}

}
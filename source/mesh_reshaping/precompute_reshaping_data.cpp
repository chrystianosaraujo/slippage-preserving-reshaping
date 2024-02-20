#include <mesh_reshaping/precompute_reshaping_data.h>

#include <mesh_reshaping/straight_chains.h>
#include <mesh_reshaping/similarity_term.h>

#include <ca_essentials/meshes/compute_triangle_normal.h>

#include <igl/avg_edge_length.h>

namespace {

void init_vertices(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    data.orig_vertices = mesh.get_vertices();
    data.curr_vertices = mesh.get_vertices();
}

// init input edge vectors (non-normalized) and lengths
void init_edge_vectors_and_lenghts(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const int num_edges = mesh.get_num_edges();

    data.orig_edges.resize(num_edges, 3);
    data.orig_edge_lens.resize(num_edges);

    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        data.orig_edges.row(e) = data.orig_vertices.row(edge_verts[1]) -
                                 data.orig_vertices.row(edge_verts[0]);

        data.orig_edge_lens(e) = data.orig_edges.row(e).norm();
    }
}

void init_current_and_target_edge_lenghts(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const int num_edges = mesh.get_num_edges();

    data.orig_edges.resize(num_edges, 3);
    data.orig_edge_lens.resize(num_edges);

    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        data.orig_edges.row(e) = data.orig_vertices.row(edge_verts[1]) -
                                 data.orig_vertices.row(edge_verts[0]);

        data.orig_edge_lens(e) = data.orig_edges.row(e).norm();
    }
}

void init_average_edge_length(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;

    data.avg_edge_len = igl::avg_edge_length(mesh.get_vertices(),
                                             mesh.get_facets());
}

void init_triangle_normals(reshaping::ReshapingData& data) {
    namespace meshes = ca_essentials::meshes;

    const auto& mesh = data.mesh;
    const int num_faces = mesh.get_num_facets();

    // TODO: replace by IGL
    data.orig_tri_N = meshes::compute_triangle_normal(data.mesh);
}

// init the two adjacent face-normals for each edge
void init_edge_adjacent_normals(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const int num_edges = mesh.get_num_edges();
    const auto& adj_e2f = data.mesh.get_edge_face_adjacency();

    data.orig_edge_adj_N.resize(num_edges);
    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_faces = adj_e2f.at(e);
        assert(edge_faces.size() == 2);

        for(int i = 0; i < 2; ++i) {
            int adj_fid = edge_faces.at(i);
            data.orig_edge_adj_N.at(e).at(i) = data.orig_tri_N.row(adj_fid);
        }
    }
}

// init per-triangle current and previous transformations
void init_triangle_transformations(reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const int num_faces = mesh.get_num_facets();

    data.curr_tri_T.resize(num_faces, Eigen::Matrix3d::Identity());
    data.prev_tri_T.resize(num_faces, Eigen::Matrix3d::Identity());
}

// init per-edge current and target lengths
void init_edge_current_and_target_lengths(reshaping::ReshapingData& data) {
    data.curr_edge_lens   = data.orig_edge_lens;
    data.target_edge_lens = data.orig_edge_lens;
}

void init_num_straight_pairs(reshaping::ReshapingData& data) {
    data.num_straight_pairs = 0;

    if(!data.straight_chains)
        return;

    for(int c = 0; c < data.straight_chains->num_chains(); ++c) {
        const auto& chain = data.straight_chains->get_chain(c);

        if(chain.size() >= 3)
            data.num_straight_pairs += (int) chain.size() - 2; 
    }
}

void init_sphericity_terms(const reshaping::ReshapingParams& params,
                           reshaping::ReshapingData& data) {

    reshaping::SphericityTermsParams sphericity_params;
    sphericity_params.k_max_const  = params.vertex_sol.sphericity_k_max_const;
    sphericity_params.debug_folder = params.debug_folder.string();

    data.sphericity_terms_info = reshaping::compute_sphericity_terms_info(data.mesh,
                                                                          data.PV1,
                                                                          data.PV2,
                                                                          sphericity_params);
}

void init_similarity_edge_weights(const reshaping::ReshapingParams& params,
                                  reshaping::ReshapingData& data) {

     reshaping::compute_similarity_term_edge_weights(data.mesh,
                                                     data.orig_tri_N,
                                                     params.transf_sol.similarity_sigma,
                                                     params.transf_sol.similarity_cutoff_angle,
                                                     data.similarity_term_edge_w);
}

void init_length_based_edge_weights(reshaping::ReshapingData& data) {
    namespace meshes = ca_essentials::meshes;

    const auto& mesh = data.mesh;
    const int num_edges = mesh.get_num_edges();

    if(data.length_based_edge_w.rows() == 0) 
        data.length_based_edge_w.resize(num_edges);

    for(int e = 0; e < num_edges; ++e) {
        const auto& edge_verts = mesh.get_edge_vertices(e);

        double curr_len = data.curr_edge_lens(e);
        data.length_based_edge_w(e) = std::max(1.0, pow(curr_len / data.avg_edge_len, 2.0));
    }
}

}

namespace reshaping {

std::unique_ptr<ReshapingData>
precompute_reshaping_data(const ReshapingParams& params,
                          const TriMesh& mesh,
                          const Eigen::VectorXd& PV1,
                          const Eigen::VectorXd& PV2,
                          const StraightChains* straight_chains) {

    const std::unordered_map<int, Eigen::Vector3d> bc;
    return precompute_reshaping_data(params, mesh, PV1, PV2, bc, straight_chains);
}

std::unique_ptr<ReshapingData>
precompute_reshaping_data(const ReshapingParams& params,
                          const TriMesh& mesh,
                          const Eigen::VectorXd& PV1,
                          const Eigen::VectorXd& PV2,
                          const std::unordered_map<int, Eigen::Vector3d>& bc,
                          const StraightChains* straight_chains) {

    auto data = std::make_unique<ReshapingData>(mesh);
    data->PV1 = PV1;
    data->PV2 = PV2;
    data->straight_chains = straight_chains;
    data->bc = bc;

    init_vertices(*data);
    init_edge_vectors_and_lenghts(*data);
    init_average_edge_length(*data);
    init_triangle_normals(*data);
    init_edge_adjacent_normals(*data);
    init_triangle_transformations(*data);
    init_edge_current_and_target_lengths(*data);
    init_num_straight_pairs(*data);
    init_sphericity_terms(params, *data);
    init_similarity_edge_weights(params, *data);
    init_length_based_edge_weights(*data);

    return data;
}

}

#include <mesh_reshaping/handle_error_distribution.h>

#include <mesh_reshaping/globals.h>
#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/vertex_solve.h>

#include <ca_essentials/core/logger.h>
#include <ca_essentials/io/saveOBJ.h>

#include <igl/per_face_normals.h>

#include <filesystem>

namespace {

// Exports the set of triangles in a ring to an OBJ file
void export_ring_to_obj(const std::string& debug_dir,
                        const std::string& suffix,
                        const Eigen::MatrixXd& V,
                        const Eigen::MatrixXi& F,
                        const std::unordered_set<int>& ring) {
    namespace fs = std::filesystem;

    auto fn = fs::path(debug_dir) / (std::string("dimples_fix_rings_") + suffix + ".obj");

    std::vector<int> sel_faces(ring.begin(), ring.end());
    ca_essentials::io::saveOBJ(fn.string().c_str(), V, F, sel_faces);
}

// Computes the first ring of triangles around the given vertex
std::unordered_set<int>
compute_first_ring_around_vertex(const reshaping::TriMesh& mesh,
                                 int vid) {

    const auto& adj_v2f = mesh.get_vertex_facet_adjacency();

    std::unordered_set<int> ring;
    for(int fid : adj_v2f.at(vid))
        ring.insert(fid);

    return ring;
}

// Computes the next level of adjacent triangles
//
// Parameters:
//   prev_rings: set of triangles already included in the previous rings
std::unordered_set<int>
compute_next_ring(const reshaping::TriMesh& mesh,
                  const std::unordered_set<int>& prev_rings) {
    const auto& adj_v2f = mesh.get_vertex_facet_adjacency();

    const Eigen::MatrixXi& F = mesh.get_facets();

    std::unordered_set<int> ring;
    for(int fid : prev_rings)
        for(int i = 0; i < 3; ++i) {
            int vid = F(fid, i);

            for(int adj_fid : adj_v2f.at(vid))
                if(prev_rings.count(adj_fid) == 0)
                    ring.insert(adj_fid);
        }

    return ring;
}

// Computes the set of triangles in the k-th ring around the given vertices
//
// Returns:
//   vertex_rings: each vector item i constains the list of triangles in the k-th
//                 ring around the ith given vertex
//
//   face_to_ring: returns a map from face id to the ring level
void compute_krings_around_constraints(const reshaping::TriMesh& mesh,
                                       const std::vector<int>& verts,
                                       int k,
                                       std::vector<std::unordered_set<int>>& vertex_rings,
                                       std::vector<std::unordered_map<int, int>>& face_to_ring) {
    vertex_rings.resize(verts.size());
    face_to_ring.resize(verts.size());

    // Initializing the first ring of triangles around every given vertex
    for(int i = 0; i < (int)verts.size(); ++i) {
        int vid = verts.at(i);
        vertex_rings.at(i) = compute_first_ring_around_vertex(mesh, vid);

        for(int fid : vertex_rings.at(i))
            face_to_ring.at(i).insert({fid, 1});
    }

    // Computing further rings for every given vertex
    for(int i = 1; i < k; ++i) {
        for(int v = 0; v < (int)verts.size(); ++v) {
            int vid = verts.at(v);

            auto next_ring = compute_next_ring(mesh, vertex_rings.at(v));
            vertex_rings.at(v).insert(next_ring.begin(), next_ring.end());

            for(int fid : next_ring)
                face_to_ring.at(v).insert({fid, i + 1});
        }
    }
}

// Returns a vector of size #verts with a flag indicating if a vertex needs error distribution.
// 
// Parameters:
//   mesh: input mesh
//   verts: list of vertices to be checked
//   vertex_rings: list of face rings around each vertex in "verts"
//   faces_to_ring: map from face id to ring level
//   origFN: original face normals
//   currFN: current face normals
//   ring_size: number of rings to be checked
//   inner_ring_error_tol: vertex is flagged if the error at the first ring is greater than
//                         the error at the last ring times this factor
//   min_error: minimum error to be considered for handle error distribution
std::vector<bool> error_distribution_vertex_flag(const reshaping::TriMesh& mesh,
                                                 const std::vector<int>& verts,
                                                 const std::vector<std::unordered_set<int>>& vertex_rings,
                                                 const std::vector<std::unordered_map<int, int>>& faces_to_ring,
                                                 const Eigen::MatrixXd& origFN,
                                                 const Eigen::MatrixXd& currFN,
                                                 int ring_size,
                                                 double inner_ring_error_tol,
                                                 double min_error) {

    const size_t num_verts_to_check = verts.size();
    std::vector<bool> has_high_error(num_verts_to_check, false);

    for(auto i = 0; i < num_verts_to_check; ++i) {
        const int ring_vid = verts.at(i);
        const auto& fid_to_ring_level = faces_to_ring.at(i);

        std::vector<double> ring_level_errors(ring_size, -DBL_MAX);

        // Computes the maximum error at each ring level
        for(const auto& [fid, ring_i] : fid_to_ring_level) {

            const Eigen::Vector3d orig_N = origFN.row(fid).normalized();
            const Eigen::Vector3d curr_N = currFN.row(fid).normalized();

            double dot = orig_N.dot(curr_N);
            dot = std::max(std::min(dot, 1.0), -1.0);

            double angle = acos(dot);

            ring_level_errors[ring_i - 1] = std::max(ring_level_errors[ring_i - 1],
                                                     angle);
        }

        has_high_error.at(i) = ring_level_errors[0] > min_error &&
                               ring_level_errors[0] > (ring_level_errors[ring_size - 1] * inner_ring_error_tol);

        LOGGER.debug("Check if handle error distribution is needed at vertex {}", ring_vid);

        for(int i = 0; i < ring_level_errors.size(); ++i)
            LOGGER.debug("    Ring ({}):  max error ({:.10f})", i + 1, ring_level_errors[i] * 180.0 / M_PI);

        LOGGER.debug("    .... needs handle error distribution? {}", has_high_error.at(i));
    }

    return has_high_error;
}

// Stores the list of triangles and edges inside and outside of
// the error distribution zone, as well as the face to ring level map.
struct ErrorDistributionZoneInfo {
    std::unordered_set<int> in_zone_tris;
    std::unordered_set<int> in_zone_edges;
    std::unordered_set<int> off_zone_edges;
    std::unordered_set<int> off_zone_tris;
    std::unordered_map<int, int> face_to_ring;
    std::unordered_set<int> high_error_verts;
};

// Computes the list of triangles and edges inside and outside of
// the error distribution zone
ErrorDistributionZoneInfo
compute_distribution_zone_info(const reshaping::TriMesh& mesh,
                               const std::unordered_map<int, Eigen::Vector3d>& constraints,
                               const Eigen::MatrixXd& orig_tri_N,
                               const Eigen::MatrixXd& curr_tri_N,
                               const reshaping::HandleErrorDistribParams& params) {

    // Defines the set of all constraint vertices (handles and anchors)
    std::unordered_set<int> hcs;
    for(const auto& [vid, pos] : constraints)
        hcs.insert(vid);

    // Computes face rings around the constraint vertices
    std::vector<std::unordered_set<int>> per_vertex_rings;
    std::vector<std::unordered_map<int, int>> per_vertex_fids_to_ring_level;
    std::vector<int> hcs_vec(hcs.begin(), hcs.end());
    compute_krings_around_constraints(mesh,
                                      hcs_vec,
                                      params.num_rings,
                                      per_vertex_rings,
                                      per_vertex_fids_to_ring_level);

    // Computes the flag indicating if a constraint vertex needs error distribution
    auto high_error_verts = error_distribution_vertex_flag(mesh,
                                                           hcs_vec,
                                                           per_vertex_rings,
                                                           per_vertex_fids_to_ring_level,
                                                           orig_tri_N,
                                                           curr_tri_N,
                                                           params.num_rings,
                                                           params.inner_ring_error_tol,
                                                           params.min_error_tol);

    ErrorDistributionZoneInfo zones_info;

    // Setting high-error vertices
    for(int i = 0; i < (int) hcs_vec.size(); ++i)
        if(high_error_verts.at(i))
            zones_info.high_error_verts.insert(hcs_vec.at(i));

    if(zones_info.high_error_verts.size() == 0)
        return zones_info;

    // Merge all the rings containing high error vertices
    for(int i = 0; i < (int) hcs_vec.size(); ++i) {
        if(!high_error_verts.at(i))
            continue;

        // Merging all triangles in the zone
        for(int fid : per_vertex_rings.at(i))
            zones_info.in_zone_tris.insert(fid);

        for(const auto& [fid, ring_i] : per_vertex_fids_to_ring_level.at(i)) {
            const auto& itr = zones_info.face_to_ring.find(fid);

            if(itr == zones_info.face_to_ring.end())
                zones_info.face_to_ring[fid] = ring_i;
            else {
                int new_ring_i = std::min(ring_i, itr->second);
                zones_info.face_to_ring[fid] = new_ring_i;
            }
        }
    }

    // Computing the set of edges in the error distribution zone
    for(int fid : zones_info.in_zone_tris) {
        for(int i = 0; i < 3; ++i) {
            int eid = -1;
            {
                int vid0 = mesh.get_facets()(fid, i);
                int vid1 = mesh.get_facets()(fid, (i + 1) % 3);
                eid = mesh.get_edge_index(vid0, vid1);
            }
            assert(eid >= 0);

            zones_info.in_zone_edges.insert(eid);
        }
    }

    // Computing the set of edges and triangles outside of the error distribution zone
    for(int eid = 0; eid < mesh.get_num_edges(); ++eid)
        if(zones_info.in_zone_edges.count(eid) == 0)
            zones_info.off_zone_edges.insert(eid);

    for(int fid = 0; fid < mesh.get_num_facets(); ++fid)
        if(zones_info.in_zone_tris.count(fid) == 0)
            zones_info.off_zone_tris.insert(fid);

    return zones_info;
}

// Prepares reshaping data for a vertex solve step enabling handle error distribution
bool prepare_data_for_handle_error_distribution(const reshaping::ReshapingParams& params,
                                                reshaping::ReshapingData& data) {
    const auto& mesh = data.mesh;
    const Eigen::MatrixXi& F = mesh.get_facets();

    Eigen::MatrixXd curr_tri_N;
    igl::per_face_normals(data.curr_vertices, F, curr_tri_N);

    auto zones_info = compute_distribution_zone_info(mesh,
                                                     data.bc,
                                                     data.orig_tri_N,
                                                     curr_tri_N,
                                                     params.vertex_sol.handle_error_distrib);

    if(zones_info.high_error_verts.size() == 0) {
        LOGGER.debug("No vertex flagged as requiring handle error distribution");
        return false;
    }
    else
        LOGGER.debug("{} vertices flagged as requiring handle error distribution", zones_info.high_error_verts.size());

  
    //////////////////////////////////////////
    // Preparing input data
    //////////////////////////////////////////

    // off-zone edges: target edges are defined as the ones from reshaping call output
    for(int eid : zones_info.off_zone_edges) {
        const auto edge_verts = mesh.get_edge_vertices(eid);

        Eigen::Vector3d curr_E = data.curr_vertices.row(edge_verts[1]) -
                                 data.curr_vertices.row(edge_verts[0]);

        data.orig_edges.row(eid) = curr_E;
        data.orig_edge_lens(eid) = curr_E.norm();
    }

    // off-zone tris: target face normals are defined as the ones from reshaping call output
    for(int fid : zones_info.off_zone_tris)
        data.orig_tri_N.row(fid) = curr_tri_N.row(fid);

    // off-zone tris: target transformations are defined as identity
    for(int fid : zones_info.off_zone_tris)
        data.curr_tri_T.at(fid).setIdentity();

    // on-zone edges: target lengths are defined as the original ones
    for(int eid : zones_info.in_zone_edges)
        data.target_edge_lens(eid) = data.orig_edge_lens(eid);

    // on-zone tris: target normals are interpolated between original and output ones
    for(const auto [fid, ring_i] : zones_info.face_to_ring) {
        if(ring_i == 1)
            continue;

        // Factor of the new normal
        double f = (ring_i - 1) *  (1.0 / params.vertex_sol.handle_error_distrib.num_rings);

        const Eigen::Vector3d& orig_N = data.orig_tri_N.row(fid);
        const Eigen::Vector3d& curr_N = curr_tri_N.row(fid);

        Eigen::Vector3d mix_N = (f * curr_N + (1.0 - f) * orig_N).normalized();
        data.orig_tri_N.row(fid) = mix_N;
    }

    // Define the set of all vertices (outside rings) that need to held in place
    for(int fid : zones_info.off_zone_tris)
        for(int i = 0; i < 3; ++i)
            data.handle_error_dist_out_verts.insert(mesh.get_facets()(fid, i));

    return true;
}

}

namespace reshaping {

void perform_handle_error_distribution(const ReshapingParams& params,
                                       ReshapingData& data,
                                       Eigen::MatrixXd& V) {
    const auto& mesh = data.mesh;
    const int num_verts = mesh.get_num_vertices();

    const Eigen::MatrixXi& F = mesh.get_facets();

    if(!prepare_data_for_handle_error_distribution(params, data))
        return;

#if !OPTIMIZED_VERSION_ON
    {
        auto fn = m_params->debug_folder / (params.input_name + "_before_dimples_fix.obj");
        ca_essentials::meshes::save_trimesh(fn.string(),
                                            data.curr_vertices,
                                            mesh.get_facets());
    }
#endif

    LOGGER.debug("Performing dimple fix");


    // Enabling handle-error distribution locally only
    VertexSolveParams vs_params = params.vertex_sol;
    vs_params.handle_error_distrib.is_active = true;

    bool succ = solve_for_vertices(vs_params, data, V);
}

}

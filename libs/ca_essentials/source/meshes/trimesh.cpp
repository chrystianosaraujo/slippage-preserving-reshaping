#include <ca_essentials/meshes/trimesh.h>

#include <lagrange/create_mesh.h>
#include <lagrange/GenuineMeshGeometry.h>
#include <lagrange/corner_to_edge_mapping.h>
#include <lagrange/utils/assert.h>

#include <igl/per_face_normals.h>

namespace ca_essentials {
namespace meshes {

TriMesh::TriMesh() {

}

TriMesh::TriMesh(const Eigen::MatrixXd& verts,
                 const Eigen::MatrixXi& tris) {

    this->initialize(verts, tris);
    this->initialize_edge_data();
    this->initialize_connectivity();

    update_bbox();
    update_face_normals();
    initialize_edge_face_adjacency();
    initialize_edge_map();
}

TriMesh::TriMesh(const TriMesh& o) {

    this->initialize(o.get_vertices(), o.get_facets());
    this->initialize_edge_data();
    this->initialize_connectivity();

    this->initialize_edge_data();
    this->initialize_connectivity();

    this->m_e2f_adj = o.m_e2f_adj;
    this->m_edge_index_map = o.m_edge_index_map;
    this->bbox = o.bbox;
    this->m_FN = o.m_FN;
}

const TriMesh::AdjacencyList&
TriMesh::get_edge_face_adjacency() const {
    return m_e2f_adj;
}

int TriMesh::vertex_opposite_to_edge(int eid, int fid) const {
    const auto& F = get_facets();
    const auto& edge_verts = get_edge_vertices(eid);

    for(int i = 0; i < 3; ++i) {
        int vid = F(fid, i);

        if(vid != edge_verts.front() && vid != edge_verts.back())
            return vid;
    }

    la_runtime_assert(false && "Could not find vertex opposite to given edge");
    return -1;
}

int TriMesh::get_edge_index(int vid0, int vid1) const {
    Edge edge(vid0, vid1);

    auto itr = m_edge_index_map.find(edge);

    la_runtime_assert(itr != m_edge_index_map.end() && "Invalid edge vertices");
    return itr->second;
}

bool TriMesh::are_vertices_adjacent(int vid0, int vid1) const {
    Edge edge(vid0, vid1);
    return m_edge_index_map.count(edge) > 0;
}

const Eigen::MatrixXd& TriMesh::get_face_normals() const {
    return m_FN;
}

std::array<int, 3> TriMesh::get_face_edges(int fid) const {
    const Eigen::MatrixXi& F = get_facets();
    return {
        get_edge_index(F(fid, 0), F(fid, 1)),
        get_edge_index(F(fid, 0), F(fid, 2)),
        get_edge_index(F(fid, 1), F(fid, 2)),
    };
}

int TriMesh::get_shared_edge(int fid0, int fid1) const {
    const Eigen::MatrixXi& F = get_facets();

    std::array<int, 3> f0_edges = get_face_edges(fid0);
    std::array<int, 3> f1_edges = get_face_edges(fid1);
    for(int i = 0; i < 3; ++i)
        for(int j = 0; j < 3; ++j)
            if(f0_edges[i] == f1_edges[j])
                return f0_edges[i];

    return -1;
}

int TriMesh::get_opposite_face(int fid, int eid) const {
    auto& adj_faces = get_edge_face_adjacency().at(eid);

    if(adj_faces.front() == fid)
        return adj_faces.back();
    else if(adj_faces.back() == fid)
        return adj_faces.front();
    else {
        la_runtime_assert(false && "Could not find face opposite to face and edge");
        return -1;
    }
}

void TriMesh::initialize_edge_face_adjacency() {
    using Index = typename lagrange::TriangleMesh3D::Index;

    const int num_edges = get_num_edges();
    m_e2f_adj.resize(num_edges);

    for(int e = 0; e < num_edges; ++e) {
        m_e2f_adj.at(e).reserve(2);

        foreach_facets_around_edge(e, [e, this](Index f) {
            m_e2f_adj.at(e).push_back(f);
        });
    }
}

void TriMesh::initialize_edge_map() {
    int num_edges = get_num_edges();
    m_edge_index_map.reserve(num_edges);

    for(int e = 0; e < num_edges; ++e) {
        const std::array<int, 2> edge_vids = get_edge_vertices(e);

        Edge edge(edge_vids[0], edge_vids[1]);
        m_edge_index_map.emplace(edge, e);
    }
}

void TriMesh::update_bbox() {
    bbox = Eigen::AlignedBox3d();

    for(int i = 0; i < get_vertices().rows(); ++i)
        bbox.extend((Eigen::Vector3d) get_vertices().row(i));
}

void TriMesh::update_face_normals() {
    igl::per_face_normals(get_vertices(), get_facets(), m_FN);
}

const Eigen::AlignedBox3d& TriMesh::get_bbox() const {
    return bbox;
}

}
}

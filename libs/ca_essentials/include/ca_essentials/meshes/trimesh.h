/*
 * This class is an extension of the TriangleMesh class available in the Adobe's lagrange library
 *
 * The original code can be found at: https://github.com/adobe/lagrange
 *
 * Copyright 2020 Adobe. All rights reserved.
 * This file is licensed to you under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 * OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */
#pragma once

#include <Eigen/Core>
#include <lagrange/Mesh.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace ca_essentials {
namespace meshes {

class TriMesh : public lagrange::TriangleMesh3D {
public:
    TriMesh();
    TriMesh(const Eigen::MatrixXd& verts,
             const Eigen::MatrixXi& tris);
    TriMesh(const TriMesh& o);

    // Returns edge to face adjacency information.
    const AdjacencyList& get_edge_face_adjacency() const;

    // Returns the vertex opposite to the given edge and face.
    int vertex_opposite_to_edge(int eid, int fid) const;

    // Returns index of the edge given its vertices.
    int get_edge_index(int vid0, int vid1) const;

    // Returns whether the given vertices are adjacent.
    bool are_vertices_adjacent(int vid0, int vid1) const;

    // Returns pre-computed face normals.
    const Eigen::MatrixXd& get_face_normals() const;

    // Returns the edges of the given face.
    std::array<int, 3> get_face_edges(int fid) const;

    // Returns the shared edge between the given faces.
    int get_shared_edge(int fid0, int fid1) const;

    // Returns the face opposite to the given edge and face.
    int get_opposite_face(int fid, int eid) const;

    // Recomputes cached bounding box.
    void update_bbox();

    // Recomputes cached face normals.
    void update_face_normals();

    // Returns cached bounding box
    const Eigen::AlignedBox3d& get_bbox() const;

private:
    // initializes edge to face adjacency information.
    void initialize_edge_face_adjacency();

    // initializes edge to index mapping.
    void initialize_edge_map();

private:
    // pre-computed mesh bounding box.
    // NOTE: Whenever mesh vertices are modified,
    //       TriMesh::update_bbox must be explicitly called.
    Eigen::AlignedBox3d bbox;

    // Stores edge to face adjacency information.
    std::vector<std::vector<int>> m_e2f_adj;

    // Stores edge to index mapping.
    std::unordered_map<Edge, int> m_edge_index_map;

    // pre-computed face normals.
    // NOTE: Whenever mesh vertices are modified,
    //       TriMesh::update_face_normals must be explicitly called.
    Eigen::MatrixXd m_FN;
};

}
}
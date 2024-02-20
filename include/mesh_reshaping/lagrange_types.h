#pragma once

#include <lagrange/common.h>
#include <lagrange/Mesh.h>
#include <lagrange/Edge.h>

namespace reshaping {
    using TriangleMesh = lagrange::TriangleMesh3D;
    using Edge = lagrange::EdgeType<TriangleMesh::Index>;
    using VertexType = typename lagrange::TriangleMesh3D::VertexType;
    using Index = typename lagrange::TriangleMesh3D::Index;
}
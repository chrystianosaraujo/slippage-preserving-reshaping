#include "arrow_mesh_builder.h"
#include "cylinder_mesh_builder.h"

#include <ca_essentials/core/compute_flat_mesh.h>

#include <algorithm>

ArrowMesh build_arrow_mesh(double radius, double height,
                           int n_radial_segments,
                           bool add_bottom_cap) {
    namespace core = ca_essentials::core;

    CylinderMesh head_mesh = build_cylinder_mesh(radius, height,
                                                 n_radial_segments,
                                                 2,
                                                 add_bottom_cap);

    // -2 top/bottom cap center
    int top_cap_first_idx = (int) head_mesh.V.rows() - n_radial_segments - 2;
    for(int i = 0; i < n_radial_segments; ++i) {
        int vid = top_cap_first_idx + i;
        head_mesh.V(vid, 0) = head_mesh.V(vid, 1) = 0.0;
    }

    ArrowMesh out_mesh;
    core::compute_flat_mesh(head_mesh.V, head_mesh.T, 
                            out_mesh.V, out_mesh.T, out_mesh.N);

    return out_mesh;
}

#include <ca_essentials/meshes/build_cylinder_mesh.h>

#include <algorithm>

namespace ca_essentials {
namespace meshes {

void build_cylinder_mesh(double radius, double height,
                         Eigen::MatrixXd& V,
                         Eigen::MatrixXi& F,
                         int n_radial_segments, int n_height_segments,
                         bool add_caps) {

    n_radial_segments = std::max(3, n_radial_segments);
    n_height_segments = std::max(1, n_height_segments);

    const double half_height = height / 2.0;
    const double radius_seg_len = 1.0 / n_radial_segments;
    const double height_seg_len = 1.0 / (n_height_segments - 1);

    int num_verts = n_height_segments * n_radial_segments;
    int num_tris  = (n_height_segments - 1) * n_radial_segments * 2;

    if(add_caps) {
        // one vertex at the center of top and bottom caps
        num_verts = num_verts + 2;

        // triangle fan on both caps
        num_tris = num_tris + n_radial_segments * 2;
    }

    V.resize(num_verts, 3);
    F.resize(num_tris , 3);

    double two_PI = M_PI * 2.0;

    // Building Vertices
    int vert_idx = 0;
    for(int i = 0; i < n_height_segments; i++) {
        double f_y = i * height_seg_len;

        for(int j = 0; j < n_radial_segments; j++) {
            double f_x = j * radius_seg_len;

            double x = cos(f_x * two_PI);
            double y = sin(f_x * two_PI);
            double z = 1.0 * f_y - 0.5;

            V.row(vert_idx) << radius * x,
                               radius * y,
                               height * z;

            //N.row(vert_idx) << x, y, z;

            vert_idx++;
        }
    }

    // Building triangles
    int tri_idx = 0;
    for(int i = 0; i < n_height_segments - 1; i++)
        for(int j = 0; j < n_radial_segments; j++) {
            int next_i = i + 1;
            int next_j = (j + 1) % n_radial_segments;

            int tri_a = n_radial_segments * i + j;
            int tri_b = n_radial_segments * i + next_j;
            int tri_c = n_radial_segments * next_i + j;
            int tri_d = n_radial_segments * next_i + next_j;

            F.row(tri_idx++) << tri_a, tri_b, tri_c;
            F.row(tri_idx++) << tri_b, tri_d, tri_c;
        }

    if(add_caps) {
        int bottom_idx = vert_idx++;
        V.row(bottom_idx) << 0.0, 0.0, -half_height;
        //N.row(bottom_idx) << 0.0, 0.0, -1.0;

        // Bottom Triangles
        for(int j = 0; j < n_radial_segments; j++) {
            int next_j = (j + 1) % n_radial_segments;
            int tri_a = j;
            int tri_b = next_j;
            int tri_c = bottom_idx;

            F.row(tri_idx++) << tri_a, tri_b, tri_c;
        }

        int top_idx = vert_idx++;
        V.row(top_idx) << 0.0, 0.0, half_height;
        //N.row(top_idx) << 0.0, 0.0, 1.0;

        // Top Triangles
        for(int j = 0; j < n_radial_segments; j++) {
            int next_j = (j + 1) % n_radial_segments;

            int tri_a = n_radial_segments * (n_height_segments - 1) + j;
            int tri_b = n_radial_segments * (n_height_segments - 1) + next_j;
            int tri_c = top_idx;

            F.row(tri_idx++) << tri_a, tri_b, tri_c;
        }
    }
}

}
}

#include "sphere_mesh_builder.h"

#include <glm/gtc/constants.hpp>

SphereMesh build_sphere_mesh(float radius, int n_slices, int n_stacks) {
    SphereMesh mesh;

	int n_verts = (n_slices + 1) * (n_stacks + 1);
	int n_elems = (n_slices * 2  * (n_stacks - 1));

	mesh.V.resize(n_verts, 3);
	mesh.N.resize(n_verts, 3);
	mesh.T.resize(n_elems, 3);
	mesh.Tex.resize(n_elems, 2);

	// Generate positions and normals
	float theta;
	float phi;
	float theta_fac = glm::two_pi<float>() / n_slices;
	float phi_fac   = glm::pi<float>()     / n_stacks;
	float nx, ny, nz, s, t;
	int idx = 0;
	for(int i = 0; i <= n_slices; i++) {
		theta = i * theta_fac;
		s = (float) i / n_slices;
		for(int j = 0; j <= n_stacks; j++) {
			phi = j * phi_fac;
			t = (float) j / n_stacks;

			nx = sinf(phi) * cosf(theta);
			ny = sinf(phi) * sinf(theta);
			nz = cosf(phi);

			mesh.V.row(idx) << radius * nx, radius * ny, radius * nz;
			mesh.N.row(idx) << nx, ny, nz;
			mesh.Tex.row(idx) << s, t;

			idx ++;
		}
	}

	// Generate the element list
	idx = 0;
	for(int i = 0; i < n_slices; i++) {
		int stack_start = i * (n_stacks + 1);
		int next_stack_start = (i + 1) * (n_stacks + 1);
		for(int j = 0; j < n_stacks; j++) {
			if(j == 0) {
				mesh.T.row(idx) << stack_start, stack_start + 1, next_stack_start + 1;
				idx++;
			}
			else if(j == n_stacks - 1) {
				mesh.T.row(idx) << stack_start + j, stack_start + j + 1, next_stack_start + j;
				idx++;
			}
			else {
				mesh.T.row(idx) << stack_start + j, stack_start + j + 1, next_stack_start + j + 1;
				idx++;

				mesh.T.row(idx) << next_stack_start + j, stack_start + j, next_stack_start + j + 1;
				idx++;
			}
		}
	}

    return mesh;
}

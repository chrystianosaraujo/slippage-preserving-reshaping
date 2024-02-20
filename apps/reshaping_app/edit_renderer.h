#pragma once

#include "sphere_mesh_builder.h"
#include "cylinder_mesh_builder.h"
#include "arrow_mesh_builder.h"
#include <ca_essentials/renderer/glslprogram.h>
#include <ca_essentials/renderer/material.h>
#include <ca_essentials/renderer/light.h>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class EditRenderer {
public:
    EditRenderer();
    virtual ~EditRenderer();

    void set_edit(const std::vector<glm::fvec3>& fixed_verts);

    void set_edit(const std::vector<glm::fvec3>& fixed_verts,
                  const std::vector<std::pair<glm::fvec3, glm::fvec3>>& displacements);

    void set_matrices(const glm::mat4& model,
                      const glm::mat4& view,
                      const glm::mat4& proj,
                      const glm::vec3& cam_pos,
                      const glm::vec3& center);

    void set_min_bbox_ext(float ext);

    void set_points_radius(float radius);
    float& get_points_radius();
    
    void set_colors(const glm::fvec3& fixed_points_color,
                    const glm::fvec3& disp_points_color);

    void set_fixed_points_color(const glm::fvec3& c);
    const glm::fvec3& get_fixed_points_color() const;

    void set_displacement_points_color(const glm::fvec3& c);
    const glm::fvec3& get_displacement_points_color() const;

    void enable_displacement_render(bool val);
    bool is_displacement_render_enabled() const;

    void set_light(const Light& light);
    Light& get_light();

    virtual void render();

private:
    void setup_shaders();
    void setup_buffers();
    void update_buffers();
    void delete_buffers();

    void setup_light();

    void render_spheres();
    void render_displacement_vectors();

private:
    ca_essentials::renderer::GLSLProgram m_prog;
    GLuint m_sphere_vao = 0;
    GLuint m_cylinder_vao = 0;
    GLuint m_arrow_vao = 0;
    std::vector<GLuint> m_cylinder_buffers;
    std::vector<GLuint> m_arrow_buffers;
    std::vector<GLuint> m_sphere_buffers;

    // Matrices
    glm::mat4 m_model_mat;
    glm::mat4 m_view_mat;
    glm::mat4 m_proj_mat;

    float m_min_bbox_ext = 1.0f;
    float m_sphere_radius = 1.0f;
    float m_arrow_radius = 1.0;
    float m_arrow_height = 2.0;
    float m_arrow_radius_perc_of_cylinder = 7.0f;

    // Mesh
    SphereMesh m_sphere_mesh;
    CylinderMesh m_cylinder_mesh;
    ArrowMesh m_arrow_mesh;

    // Edit info
    std::vector<glm::fvec3> m_fixed_verts;
    std::vector<std::pair<glm::fvec3, glm::fvec3>> m_displacements;

    glm::fvec3 m_fixed_points_color     = glm::fvec3(0.87f, 0.06f, 0.06f);
    //glm::fvec3 m_displaced_points_color = glm::fvec3(0.0f, 1.0f, 0.0f);
    //glm::fvec3 m_displaced_points_color = glm::fvec3(232.0/255.0, 16.0/255.0, 9.0/255.0);
    glm::fvec3 m_displaced_points_color = glm::fvec3(0.98f, 0.63f, 0.06f);

    glm::fvec3 m_base_mat_Ks = glm::fvec3(0.29f, 0.25f, 0.25f);
    float m_base_mat_shininess = 100.0f;

    bool m_displacement_render_on = true;

    Light m_light;
    glm::vec4 m_cam_pos;
};

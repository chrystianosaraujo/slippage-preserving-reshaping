#pragma once

#include "selection_handler.h"
#include <ca_essentials/renderer/glslprogram.h>
#include <ca_essentials/renderer/material.h>
#include <ca_essentials/renderer/light.h>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class SelectionRenderer {
public:
    using TriMesh = ca_essentials::meshes::TriMesh;

public:
    SelectionRenderer(const SelectionHandler& sel_handler);
    virtual ~SelectionRenderer();

    void set_mesh(const TriMesh& mesh);

    void set_matrices(const glm::mat4& model,
                      const glm::mat4& view,
                      const glm::mat4& proj,
                      const glm::vec3& cam_pos);

    void set_bounding_box(const glm::vec3& bbox_min,
                          const glm::vec3& bbox_max);

    virtual void render();

private:
    void setup_shaders();
    void setup_buffers();
    void update_buffers();
    void delete_buffers();

    void setup_light();

    void draw_sphere(const glm::vec3& center,
                     const glm::vec3& color,
                     float radius);

    void draw_cylinder(const glm::vec3& p0, const glm::vec3& p1,
                       const glm::vec3& c, double radius);

private:
    const TriMesh* m_mesh = nullptr;
    const SelectionHandler& m_sel_handler;

    ca_essentials::renderer::GLSLProgram m_prog;
    GLuint m_sphere_vao = 0;
    std::vector<GLuint> m_sphere_buffers;
    int m_num_sphere_faces;

    GLuint m_cylinder_vao = 0;
    std::vector<GLuint> m_cylinder_buffers;
    int m_num_cylinder_faces;

    // Matrices
    glm::mat4 m_model_mat;
    glm::mat4 m_view_mat;
    glm::mat4 m_proj_mat;
    glm::vec4 m_cam_pos;

    float m_sphere_radius = 1.0f;
    float m_cylinder_radius = 1.0f;

    // Light & Material (basic)
    Light m_light;
    glm::fvec3 m_base_mat_Ks = glm::fvec3(0.29f, 0.25f, 0.25f);
    float m_base_mat_shininess = 100.0f;

    glm::fvec3 m_hover_color = glm::fvec3(0.5f, 0.5f, 0.0f);
    glm::fvec3 m_selected_color = glm::fvec3(1.0f, 1.0f, 0.0f);
    glm::fvec3 m_path_color = glm::fvec3(1.0f, 1.0f, 0.0f);
};

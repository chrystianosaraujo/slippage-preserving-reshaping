#pragma once

#include "sphere_mesh_builder.h"
#include "cylinder_mesh_builder.h"
#include <ca_essentials/renderer/glslprogram.h>
#include <ca_essentials/renderer/material.h>
#include <ca_essentials/renderer/light.h>

#include <mesh_reshaping/straight_chains.h>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class StraightChainsRenderer {
public:
    StraightChainsRenderer();
    virtual ~StraightChainsRenderer();

    // TODO: Remove this copy
    void set_geometry(const Eigen::MatrixXd& V);

    void set_straight_chains(const reshaping::StraightChains* chains);

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

    void render_edges();
    void render_endpoints();

    void draw_sphere(const glm::fvec3& center,
                     const glm::fvec3& color,
                     float radius);
    void draw_cylinder(const glm::fvec3& p0,
                       const glm::fvec3& p1,
                       const glm::fvec3& color,
                       float radius);

private:
    const reshaping::StraightChains* m_chains = nullptr;
    Eigen::MatrixXd m_V;

    ca_essentials::renderer::GLSLProgram m_prog;
    GLuint m_sphere_vao = 0;
    GLuint m_cylinder_vao = 0;
    std::vector<GLuint> m_cylinder_buffers;
    std::vector<GLuint> m_sphere_buffers;

    // Matrices
    glm::mat4 m_model_mat;
    glm::mat4 m_view_mat;
    glm::mat4 m_proj_mat;
    glm::vec4 m_cam_pos;

    glm::vec3 m_bbox_min;
    glm::vec3 m_bbox_max;
    glm::vec3 m_bbox_center;

    float m_max_bbox_ext = 1.0f;
    float m_sphere_radius = 1.0f;

    int m_num_sphere_faces = 0;
    int m_num_cylinder_faces = 0;

    // Light & Material (basic)
    Light m_light;
    glm::fvec3 m_base_mat_Ks = glm::fvec3(0.29f, 0.25f, 0.25f);
    float m_base_mat_shininess = 100.0f;
};

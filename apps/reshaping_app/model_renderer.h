#pragma once

#include <ca_essentials/renderer/glslprogram.h>
#include <ca_essentials/renderer/material.h>
#include <ca_essentials/renderer/light.h>

#include <glad/glad.h>
#include <glm/glm.hpp> // TODO: use only eigen

#include <Eigen/Core>
#include <vector>

class ModelRenderer {
public:
    ModelRenderer();

    // V and T must be provided as row-major
    ModelRenderer(const Eigen::MatrixXd& V, 
                  const Eigen::MatrixXi& T,
                  const Eigen::MatrixXd& N);

// TODO: remove this version
private:
    ModelRenderer(const std::vector<float>&  V,
                  const std::vector<GLuint>& T,
                  const std::vector<float>&  N);

public:

    virtual ~ModelRenderer();

    void set_mesh(const Eigen::MatrixXd& V, 
                  const Eigen::MatrixXi& T,
                  const Eigen::MatrixXd& N);

// TODO: remove this version
private:
    void set_mesh(const std::vector<float>&  V,
                  const std::vector<GLuint>& T,
                  const std::vector<float>&  N);

public:
    void set_matrices(const glm::mat4& model,
                      const glm::mat4& view,
                      const glm::mat4& proj,
                      const glm::mat4& viewport,
                      const glm::vec3& cam_pos,
                      const glm::vec3& center);

    virtual void render();

    void enable_wireframe(bool val);
    bool is_wireframe_enabled() const;

    void set_wireframe_edge_width(float width);
    float get_wireframe_edge_width() const;

    void set_wireframe_blend_width(float width);
    float get_wireframe_blend_width() const;

    void set_wireframe_color(const glm::vec4& color);
    const glm::vec4& get_wireframe_color() const;

    void enable_surface_rendering(bool val);
    bool is_surface_rendering_enabled() const;

    void set_material(const Material& mat);
    const Material& get_material() const;

    void set_light(const Light& light);
    const Light& get_light() const;

private:
    void init();
    void setup_shaders();
    void setup_buffers();

    void update_buffers_data(const std::vector<float>& V,
                             const std::vector<GLuint>& T,
                             const std::vector<float>& N);
    void delete_buffers();

    void setup_default_light();
    void setup_default_material();

private:
    // Mesh data
    GLuint m_num_verts = 0;
    GLuint m_num_tris = 0;

    // GL
    ca_essentials::renderer::GLSLProgram m_prog;
    GLuint m_vao = 0;
    std::vector<GLuint> m_buffers;

    // Matrices
    glm::mat4 m_model_mat;
    glm::mat4 m_view_mat;
    glm::mat4 m_proj_mat;
    glm::mat4 m_viewport_mat;
    glm::mat4 m_modelview_mat;
    glm::mat3 m_normal_mat;

    glm::vec3 m_center;

    // Wireframe states
    bool m_wireframe_enabled = false;
    float m_wireframe_edge_width = 0.1f;
    float m_wireframe_blend_width = 0.1f;
    glm::vec4 m_wireframe_color = glm::vec4(0.05f, 0.0f, 0.05f, 1.0f);

    // Surface states
    bool m_surface_enabled = true;

    Light m_light;
    Material m_material;

    glm::vec4 m_cam_pos;
};

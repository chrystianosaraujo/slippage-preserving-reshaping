#include "straight_chains_renderer.h"
#include "globals.h"
#include "matrix_to_vector.h"

#include <ca_essentials/core/logger.h>
#include <ca_essentials/core/compute_flat_mesh.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>

namespace {
    enum BufferType {
        POSITION = 0,
        NORMAL = 1,
        ELEMENT = 2,

        LAST_BUFFER = ELEMENT,
        NUM_BUFFERS = LAST_BUFFER + 1
    };

    // TODO: remove this method
    static glm::fvec3 eigen_to_glm_vec3f(const Eigen::Vector3d& v) {
        return glm::vec3(v.x(), v.y(), v.z());
    }

    static glm::fvec3 eigen_to_glm_vec3f(const Eigen::Vector3f& v) {
        return glm::fvec3(v.x(), v.y(), v.z());
    }
}

StraightChainsRenderer::StraightChainsRenderer() {
    setup_shaders();
    setup_buffers();
    setup_light();
}

StraightChainsRenderer::~StraightChainsRenderer() {
    delete_buffers();
}

void StraightChainsRenderer::set_geometry(const Eigen::MatrixXd& V) {
    m_V = V;
}

void StraightChainsRenderer::set_straight_chains(const reshaping::StraightChains* chains) {
    m_chains = chains;
}

void StraightChainsRenderer::set_matrices(const glm::mat4& model,
                                const glm::mat4& view,
                                const glm::mat4& proj,
                                const glm::vec3& cam_pos) {
    m_model_mat = model;
    m_view_mat  = view;
    m_proj_mat  = proj;
    m_cam_pos   = glm::vec4(cam_pos, 1.0);
}

void StraightChainsRenderer::set_bounding_box(const glm::vec3& bbox_min,
                                              const glm::vec3& bbox_max) {
    m_bbox_min = bbox_min;
    m_bbox_max = bbox_max;

    float x_ext = m_bbox_max.x - m_bbox_min.x;
    float y_ext = m_bbox_max.y - m_bbox_min.y;
    float z_ext = m_bbox_max.z - m_bbox_min.z;

    m_max_bbox_ext = std::max(x_ext, std::max(y_ext, z_ext));
    m_bbox_center = (bbox_max + bbox_min) * 0.5f;
}

void StraightChainsRenderer::render() {
    namespace straight_chains = globals::render::straight_chains;

    if(!m_chains || m_V.rows() == 0)
        return;

    render_edges();
    render_endpoints();
}

void StraightChainsRenderer::draw_sphere(const glm::fvec3& center,
                                         const glm::fvec3& color,
                                         float radius) {
    m_prog.use();
    glBindVertexArray(m_sphere_vao);

    m_prog.setUniform("Material.Kd", color);
    m_prog.setUniform("Material.Ka", color * 0.85f);
    m_prog.setUniform("Material.Ks" , m_base_mat_Ks);
    m_prog.setUniform("Material.Shininess", m_base_mat_shininess);

    m_prog.setUniform("Light.Ld", color);
    m_prog.setUniform("Light.La", color * 0.55f);
    m_prog.setUniform("Light.Ls", m_light.Ls);

    m_prog.setUniform("Light.Position", m_view_mat * m_cam_pos);
    m_prog.setUniform("Light.Ls", m_light.Ls);
    m_prog.setUniform("Wire.Enabled", false);

    glm::mat4 scale_mat = glm::mat4(1.0f);
    scale_mat = glm::scale(scale_mat, glm::vec3(radius));

    glm::mat4 trans_mat(1.0f);
    trans_mat = glm::translate(trans_mat, center);

    glm::mat4 mv_mat = m_view_mat * m_model_mat * trans_mat * scale_mat;
    glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(mv_mat));

    m_prog.setUniform("NormalMatrix", normal_mat);
    m_prog.setUniform("ModelViewMatrix", mv_mat);
    m_prog.setUniform("MVP", m_proj_mat * mv_mat);
    glDrawElements(GL_TRIANGLES, (GLsizei) m_num_sphere_faces * 3, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void StraightChainsRenderer::draw_cylinder(const glm::fvec3& p0,
                                           const glm::fvec3& p1,
                                           const glm::fvec3& color,
                                           float radius) {
    m_prog.use();
    glBindVertexArray(m_cylinder_vao);

    m_prog.setUniform("Material.Kd", color);
    m_prog.setUniform("Material.Ka", color * 0.85f);
    m_prog.setUniform("Material.Ks" , m_base_mat_Ks);
    m_prog.setUniform("Material.Shininess", m_base_mat_shininess);

    m_prog.setUniform("Light.Ld", color);
    m_prog.setUniform("Light.La", color * 0.55f);
    m_prog.setUniform("Light.Ls", m_light.Ls);

    m_prog.setUniform("Light.Position", m_view_mat * m_cam_pos);
    m_prog.setUniform("Light.Ls", m_light.Ls);
    m_prog.setUniform("Wire.Enabled", false);

    const double length = glm::length(p1 - p0);

    // Scale
    glm::mat4 scale_mat = glm::mat4(1.0f);
    glm::vec3 scale_vec = glm::vec3(radius, radius, length);
    scale_mat = glm::scale(scale_mat, scale_vec);

    // Translation
    glm::mat4 trans_mat(1.0f);
    trans_mat = glm::translate(trans_mat, p0);

    // Move pivot to bottom center
    glm::mat4 trans_center_mat(1.0f);
    trans_center_mat = glm::translate(trans_center_mat, glm::vec3(0.0, 0.0, length * 0.5));

    // Computing rotation
    glm::mat4 rot_mat(1.0);
    {
        glm::vec3 seg_vec = glm::normalize(p1 - p0);
        glm::vec3 default_dir = glm::vec3(0.0, 0.0, 1.0);

        float angle = acos(glm::dot(seg_vec, default_dir));

        // if angle is equal to -180 or 180, make default_dir = X_AXIS
        if(fabs(angle - M_PI) < 1e-3)
            default_dir = glm::vec3(1.0, 0.0, 0.0);

        // Check if both vectors are the same. If so, the cross product
        // it undefined and there is nothing to rotate
        if(fabs(angle) > 1e-3) {
            glm::vec3 rot_vec = glm::cross(seg_vec, default_dir);
            rot_mat = glm::rotate(rot_mat, -angle, rot_vec);
        }
    }


    glm::mat4 local_mat = trans_mat * rot_mat * trans_center_mat * scale_mat;
    glm::mat4 mv_mat = m_view_mat * m_model_mat * local_mat;
    glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(mv_mat));

    m_prog.setUniform("NormalMatrix", normal_mat);
    m_prog.setUniform("ModelViewMatrix", mv_mat);
    m_prog.setUniform("MVP", m_proj_mat * mv_mat);
    glDrawElements(GL_TRIANGLES, (GLsizei) m_num_cylinder_faces * 3, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void StraightChainsRenderer::setup_shaders() {
    namespace renderer = ca_essentials::renderer;
    namespace paths = globals::paths;

    try {
        m_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.vs").string().c_str());
        m_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.gs").string().c_str());
        m_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.fs").string().c_str());
        m_prog.link();
        m_prog.use();
    } catch (renderer::GLSLProgramException &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    if constexpr(globals::logging::LOG_SHADER_SETUP_MESSAGES) {
        LOGGER.info("");
        LOGGER.info("StraightChainsRenderer Shader Info");
        m_prog.printActiveAttribs();
        LOGGER.info("");
        m_prog.printActiveUniforms();
        LOGGER.info("\n");
    }
    else
        LOGGER.debug("StraightChainsRenderer successfully created");
}

void StraightChainsRenderer::setup_buffers() {
    if(!m_sphere_buffers.empty() || !m_cylinder_buffers.empty())
        delete_buffers();

    m_sphere_buffers  .resize(BufferType::NUM_BUFFERS);
    m_cylinder_buffers.resize(BufferType::NUM_BUFFERS);
    glGenBuffers((GLsizei) m_sphere_buffers.size(),  &m_sphere_buffers[0]);
    glGenBuffers((GLsizei) m_cylinder_buffers.size(), &m_cylinder_buffers[0]);

    // Sphere Vertex Array
    {
        glGenVertexArrays(1, &m_sphere_vao);
        glBindVertexArray(m_sphere_vao);

        // Index
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphere_buffers[BufferType::ELEMENT]);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, m_sphere_buffers[BufferType::POSITION]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, m_sphere_buffers[BufferType::NORMAL]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    // Cylinder Vertex Array
    {
        glGenVertexArrays(1, &m_cylinder_vao);
        glBindVertexArray(m_cylinder_vao);

        // Index
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cylinder_buffers[BufferType::ELEMENT]);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, m_cylinder_buffers[BufferType::POSITION]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, m_cylinder_buffers[BufferType::NORMAL]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    update_buffers();
}

void StraightChainsRenderer::update_buffers() {
    namespace render = globals::render;
    namespace core = ca_essentials::core;

    // Sphere buffers
    {
        SphereMesh mesh = build_sphere_mesh(1.0f, render::sphere_num_radial_slices, render::sphere_num_radial_slices);
        m_num_sphere_faces = (int) mesh.T.rows();

        std::vector<float> vecV;
        matrix_to_vector(mesh.V.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(mesh.T.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(mesh.N.cast<float>(), vecN);

        // Position Buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_sphere_buffers[BufferType::POSITION]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecV.size(), vecV.data(), GL_STATIC_DRAW);

        // Index Buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphere_buffers[BufferType::ELEMENT]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * vecT.size(), vecT.data(), GL_STATIC_DRAW);

        // Normal Buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_sphere_buffers[BufferType::NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecN.size(), vecN.data(), GL_STATIC_DRAW);
    }

    // Cylinder buffers
    {
        CylinderMesh mesh = build_cylinder_mesh(1.0f, 1.0f,
                                                render::cylinder_num_radial_slices,
                                                render::cylinder_num_vertical_slices, true);
        Eigen::MatrixXd Vf;
        Eigen::MatrixXi Tf;
        Eigen::MatrixXd Nf;
        core::compute_flat_mesh(mesh.V, mesh.T, Vf, Tf, Nf);

        m_num_cylinder_faces = (int) Tf.rows();

        std::vector<float> vecV;
        matrix_to_vector(Vf.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(Tf.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(Nf.cast<float>(), vecN);

        // Position Buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_cylinder_buffers[BufferType::POSITION]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecV.size(), vecV.data(), GL_STATIC_DRAW);

        // Index Buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cylinder_buffers[BufferType::ELEMENT]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * vecT.size(), vecT.data(), GL_STATIC_DRAW);

        // Normal Buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_cylinder_buffers[BufferType::NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecN.size(), vecN.data(), GL_STATIC_DRAW);
    }
}

void StraightChainsRenderer::delete_buffers() {
    if(!m_sphere_buffers.empty()) {
        glDeleteBuffers((GLsizei) m_sphere_buffers.size(), m_sphere_buffers.data());
        m_sphere_buffers.clear();
    }

    if(!m_cylinder_buffers.empty()) {
        glDeleteBuffers((GLsizei) m_cylinder_buffers.size(), m_cylinder_buffers.data());
        m_cylinder_buffers.clear();
    }

    if(m_sphere_vao != 0) {
        glDeleteVertexArrays(1, &m_sphere_vao);
        m_sphere_vao = 0;
    }

    if(m_cylinder_vao != 0) {
        glDeleteVertexArrays(1, &m_cylinder_vao);
        m_cylinder_vao = 0;
    }
}

void StraightChainsRenderer::setup_light() {
    m_light.Ld = glm::fvec3(0.80f, 0.80f, 0.80f);
    m_light.Ls = glm::fvec3(0.00f, 0.00f, 0.00f);
    m_light.La = glm::fvec3(0.79f, 0.79f, 0.79f);
}

void StraightChainsRenderer::render_edges() {
    namespace straight_chains = globals::render::straight_chains;

    assert(m_chains);

    float radius = m_max_bbox_ext * straight_chains::cylinder_radius_perc;
    const glm::fvec3 color = eigen_to_glm_vec3f(straight_chains::edge_color);

    for(int i = 0; i < m_chains->num_chains(); ++i) {
        const auto& chain = m_chains->get_chain(i);

        for(int j = 1; j < chain.size(); ++j) {
            int vid0 = chain.at(j - 1);
            int vid1 = chain.at(j);

            const glm::fvec3 p0 = eigen_to_glm_vec3f((Eigen::Vector3d) m_V.row(vid0));
            const glm::fvec3 p1 = eigen_to_glm_vec3f((Eigen::Vector3d) m_V.row(vid1));

            draw_cylinder(p0, p1, color, radius);
        }
    }
}

void StraightChainsRenderer::render_endpoints() {
    namespace straight_chains = globals::render::straight_chains;

    assert(m_chains);

    float sphere_radius = m_max_bbox_ext * straight_chains::sphere_radius_perc;
    const glm::fvec3 endpoint_color = eigen_to_glm_vec3f(straight_chains::endpoint_color);

    //glDisable(GL_DEPTH_TEST);
    for(int i = 0; i < m_chains->num_chains(); ++i) {
        const auto& chain = m_chains->get_chain(i);

        int vid0 = chain.front();
        int vid1 = chain.back();

        const glm::fvec3 p0 = eigen_to_glm_vec3f((Eigen::Vector3d) m_V.row(vid0));
        const glm::fvec3 p1 = eigen_to_glm_vec3f((Eigen::Vector3d) m_V.row(vid1));

        draw_sphere(p0, endpoint_color, sphere_radius);
        draw_sphere(p1, endpoint_color, sphere_radius);
    }
    //glEnable(GL_DEPTH_TEST);
}

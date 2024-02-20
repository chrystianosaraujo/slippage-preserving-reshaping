#include "edit_renderer.h"
#include "globals.h"
#include "matrix_to_vector.h"

#include <ca_essentials/core/logger.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace {
    enum BufferType {
        POSITION = 0,
        NORMAL = 1,
        ELEMENT = 2,

        LAST_BUFFER = ELEMENT,
        NUM_BUFFERS = LAST_BUFFER + 1
    };

    int num_sphere_slices = 50;
    int num_sphere_stacks = 50;
    int num_cylinder_radial_slices = 50;
    int num_cylinder_height_slices = 10;
}

EditRenderer::EditRenderer() {
    m_sphere_mesh   = build_sphere_mesh(1.0f, num_sphere_slices, num_sphere_stacks);
    m_cylinder_mesh = build_cylinder_mesh(1.0f, 1.0f, num_cylinder_radial_slices, num_cylinder_height_slices);
    m_arrow_mesh = build_arrow_mesh(m_arrow_radius, m_arrow_height, num_cylinder_radial_slices);
    setup_shaders();
    setup_buffers();
    setup_light();
}

EditRenderer::~EditRenderer() {
    delete_buffers();
}

void EditRenderer::set_edit(const std::vector<glm::fvec3>& fixed_verts) {
    m_fixed_verts = fixed_verts;
    m_displacements.clear();
    update_buffers();
}

void EditRenderer::set_edit(const std::vector<glm::fvec3>& fixed_verts,
                            const std::vector<std::pair<glm::fvec3, glm::fvec3>>& displacements) {
    m_fixed_verts = fixed_verts;
    m_displacements = displacements;
    update_buffers();
}

void EditRenderer::set_matrices(const glm::mat4& model,
                                const glm::mat4& view,
                                const glm::mat4& proj,
                                const glm::vec3& cam_pos,
                                const glm::vec3& center) {
    m_model_mat = glm::mat4(1.0);

    m_view_mat  = view;
    m_proj_mat  = proj;
    m_cam_pos   = glm::vec4(cam_pos, 1.0);
}


void EditRenderer::set_min_bbox_ext(float ext) {
    m_min_bbox_ext = ext;
}

void EditRenderer::set_points_radius(float radius) {
    m_sphere_radius = radius;
}

float& EditRenderer::get_points_radius() {
    return m_sphere_radius;
}

void EditRenderer::set_colors(const glm::fvec3& fixed_points_color,
                              const glm::fvec3& disp_points_color) {
    m_fixed_points_color = fixed_points_color;
    m_displaced_points_color = disp_points_color;
}

void EditRenderer::set_fixed_points_color(const glm::fvec3& c) {
    m_fixed_points_color = c;
}

const glm::fvec3& EditRenderer::get_fixed_points_color() const {
    return m_fixed_points_color;
}

void EditRenderer::set_displacement_points_color(const glm::fvec3& c) {
    m_displaced_points_color = c;
}

const glm::fvec3& EditRenderer::get_displacement_points_color() const {
    return m_displaced_points_color;
}

void EditRenderer::enable_displacement_render(bool val) {
    m_displacement_render_on = val;
}

bool EditRenderer::is_displacement_render_enabled() const {
    return m_displacement_render_on;
}

void EditRenderer::set_light(const Light& light) {
   m_light = light;
}

Light& EditRenderer::get_light() {
    return m_light;
}

void EditRenderer::render() {
    render_spheres();
    if(is_displacement_render_enabled())
        render_displacement_vectors();
}

void EditRenderer::render_spheres() {
    if(m_sphere_vao == 0)
        return;

    glm::mat4 scale_mat = glm::mat4(1.0f);
    double radius = m_min_bbox_ext * m_sphere_radius * 1.5;
    //double radius = m_min_bbox_ext * m_sphere_radius;
    scale_mat = glm::scale(scale_mat, glm::vec3((float) radius));

    m_prog.use();
    glBindVertexArray(m_sphere_vao);

    m_prog.setUniform("Material.Ks" , m_base_mat_Ks);
    m_prog.setUniform("Material.Shininess", m_base_mat_shininess);
    m_prog.setUniform("Light.Position", m_view_mat * m_cam_pos);
    m_prog.setUniform("Light.Ls", m_light.Ls);
    m_prog.setUniform("Wire.Enabled", false);

    // Render Fixed Points 
    {
        m_prog.setUniform("Material.Kd", m_fixed_points_color);
        m_prog.setUniform("Material.Ka", m_fixed_points_color * 0.65f);
        m_prog.setUniform("Light.Ld", m_light.Ld);
        m_prog.setUniform("Light.La", m_light.La);

        for(const auto& v : m_fixed_verts) {
            glm::mat4 trans_mat(1.0f);
            trans_mat = glm::translate(trans_mat, v);

            glm::mat4 mv_mat = m_view_mat * m_model_mat * trans_mat * scale_mat;
            glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(mv_mat));

            m_prog.setUniform("NormalMatrix", normal_mat);
            m_prog.setUniform("ModelViewMatrix", mv_mat);
            m_prog.setUniform("MVP", m_proj_mat * mv_mat);
            glDrawElements(GL_TRIANGLES, (GLsizei) m_sphere_mesh.T.rows() * 3, GL_UNSIGNED_INT, 0);
        }
    }

    // Render Displaced Points 
    {
        m_prog.setUniform("Material.Kd", m_displaced_points_color);
        m_prog.setUniform("Material.Ka", m_displaced_points_color);
        m_prog.setUniform("Light.Ld", m_displaced_points_color);
        m_prog.setUniform("Light.La", m_displaced_points_color * 0.45f);

        bool skip_orig_point = !is_displacement_render_enabled();
        for(const auto& disp : m_displacements) {
            for(int i = 0; i < 2; ++i) {
                if(i == 0 && skip_orig_point)
                    continue;

                const auto& p = i == 0 ? disp.first : disp.second;

                glm::mat4 trans_mat(1.0f);
                trans_mat = glm::translate(trans_mat, p);

                glm::mat4 mv_mat = m_view_mat * m_model_mat * trans_mat * scale_mat;
                glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(mv_mat));

                m_prog.setUniform("NormalMatrix", normal_mat);
                m_prog.setUniform("ModelViewMatrix", mv_mat);
                m_prog.setUniform("MVP", m_proj_mat * mv_mat);
                glDrawElements(GL_TRIANGLES, (GLsizei) m_sphere_mesh.T.rows() * 3, GL_UNSIGNED_INT, 0);
            }
        }
    }
    glBindVertexArray(0);
}

void EditRenderer::render_displacement_vectors() {
    if(m_cylinder_vao == 0)
        return;

    // wc = world coordinate system
    float sphere_radius_wc   = m_min_bbox_ext * m_sphere_radius;
    float cylinder_radius_wc = sphere_radius_wc * 0.6f;
    //float cylinder_radius_wc = sphere_radius_wc * 0.6f;
    //float cylinder_radius_wc = sphere_radius_wc * 0.3f;

    float arrow_radius_wc = sphere_radius_wc * m_arrow_radius_perc_of_cylinder * 0.3f; 
    //float arrow_radius_wc = cylinder_radius_wc * m_arrow_radius_perc_of_cylinder; 
    float arrow_height_wc = arrow_radius_wc; 

    m_prog.use();
    glBindVertexArray(m_cylinder_vao);

    m_prog.setUniform("Material.Kd", m_displaced_points_color);
    m_prog.setUniform("Material.Ka", m_displaced_points_color * 0.85f);

    m_prog.setUniform("Light.Ld", m_displaced_points_color);
    m_prog.setUniform("Light.La", m_displaced_points_color * 0.55f);
    m_prog.setUniform("Light.Ls", m_light.Ls);

    for(const auto& [p0, p1] : m_displacements) {
        if(glm::length(p0 - p1) <= globals::reshaping::zero_displacement_tol)
            continue;

        float cylinder_length = glm::length(p1 - p0) - sphere_radius_wc * 2.0f;

        // Scale
        glm::mat4 scale_mat = glm::mat4(1.0f);
        glm::vec3 scale_vec = glm::vec3(cylinder_radius_wc, cylinder_radius_wc, cylinder_length);
        scale_mat = glm::scale(scale_mat, scale_vec);

        // Translation
        glm::mat4 trans_mat(1.0f);
        trans_mat = glm::translate(trans_mat, p0);

        // Move pivot to bottom center
        glm::mat4 trans_center_mat(1.0f);
        trans_center_mat = glm::translate(trans_center_mat, glm::vec3(0.0, 0.0, cylinder_length * 0.5));

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
        glDrawElements(GL_TRIANGLES, (GLsizei)m_cylinder_mesh.T.rows() * 3, GL_UNSIGNED_INT, 0);
    }

    // Render displacement arrows
    {
        m_prog.use();
        glBindVertexArray(m_arrow_vao);

        for(const auto& [p0, p1] : m_displacements) {
            if(glm::length(p0 - p1) <= globals::reshaping::zero_displacement_tol)
                continue;

            glm::vec3 seg_vec = glm::normalize(p1 - p0);

            // Scale
            glm::mat4 scale_mat = glm::mat4(1.0f);
            glm::vec3 scale_vec = glm::vec3(arrow_radius_wc, arrow_radius_wc, arrow_height_wc);
            scale_mat = glm::scale(scale_mat, scale_vec);

            // Translation
            glm::mat4 trans_mat(1.0f);
            //glm::vec3 t = p1 - glm::vec3(0.0, 0.0, -arrow_height_wc);
            //trans_mat = glm::translate(trans_mat, t);
            trans_mat = glm::translate(trans_mat, p1);

            // Move pivot to bottom center
            glm::mat4 trans_center_mat(1.0f);
            trans_center_mat = glm::translate(trans_center_mat, glm::vec3(0.0, 0.0, -arrow_height_wc - sphere_radius_wc));

            // Computing rotation
            glm::mat4 rot_mat(1.0);
            {
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
            glDrawElements(GL_TRIANGLES, (GLsizei)m_cylinder_mesh.T.rows() * 3, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
}

void EditRenderer::setup_shaders() {
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
        LOGGER.info("EditRenderer Shader Info");
        m_prog.printActiveAttribs();
        LOGGER.info("");
        m_prog.printActiveUniforms();
        LOGGER.info("\n");
    }
    else
        LOGGER.debug("EditRenderer successfully created");
}

void EditRenderer::setup_buffers() {
    if(!m_sphere_buffers.empty())
        delete_buffers();

    if(!m_cylinder_buffers.empty())
        delete_buffers();

    if(!m_arrow_buffers.empty())
        delete_buffers();

    m_sphere_buffers  .resize(BufferType::NUM_BUFFERS);
    m_cylinder_buffers.resize(BufferType::NUM_BUFFERS);
    m_arrow_buffers.resize(BufferType::NUM_BUFFERS);
    glGenBuffers((GLsizei) m_sphere_buffers.size(),  &m_sphere_buffers[0]);
    glGenBuffers((GLsizei) m_cylinder_buffers.size(), &m_cylinder_buffers[0]);
    glGenBuffers((GLsizei) m_arrow_buffers.size(), &m_arrow_buffers[0]);

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

    // Arrow Vertex Array
    {
        glGenVertexArrays(1, &m_arrow_vao);
        glBindVertexArray(m_arrow_vao);

        // Index
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_arrow_buffers[BufferType::ELEMENT]);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, m_arrow_buffers[BufferType::POSITION]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, m_arrow_buffers[BufferType::NORMAL]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }
}

void EditRenderer::update_buffers() {
    // Sphere buffers
    {
        std::vector<float> vecV;
        matrix_to_vector(m_sphere_mesh.V.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(m_sphere_mesh.T.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(m_sphere_mesh.N.cast<float>(), vecN);

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
        std::vector<float> vecV;
        matrix_to_vector(m_cylinder_mesh.V.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(m_cylinder_mesh.T.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(m_cylinder_mesh.N.cast<float>(), vecN);

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

    // Arrow buffers
    {
        std::vector<float> vecV;
        matrix_to_vector(m_arrow_mesh.V.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(m_arrow_mesh.T.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(m_arrow_mesh.N.cast<float>(), vecN);

        // Position Buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_arrow_buffers[BufferType::POSITION]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecV.size(), vecV.data(), GL_STATIC_DRAW);

        // Index Buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_arrow_buffers[BufferType::ELEMENT]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * vecT.size(), vecT.data(), GL_STATIC_DRAW);

        // Normal Buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_arrow_buffers[BufferType::NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecN.size(), vecN.data(), GL_STATIC_DRAW);
    }
}

void EditRenderer::delete_buffers() {
    if(!m_sphere_buffers.empty()) {
        glDeleteBuffers((GLsizei) m_sphere_buffers.size(), m_sphere_buffers.data());
        m_sphere_buffers.clear();
    }

    if(!m_cylinder_buffers.empty()) {
        glDeleteBuffers((GLsizei) m_cylinder_buffers.size(), m_cylinder_buffers.data());
        m_cylinder_buffers.clear();
    }

    if(!m_arrow_buffers.empty()) {
        glDeleteBuffers((GLsizei) m_arrow_buffers.size(), m_arrow_buffers.data());
        m_arrow_buffers.clear();
    }

    if(m_sphere_vao != 0) {
        glDeleteVertexArrays(1, &m_sphere_vao);
        m_sphere_vao = 0;
    }

    if(m_cylinder_vao != 0) {
        glDeleteVertexArrays(1, &m_cylinder_vao);
        m_cylinder_vao = 0;
    }

    if(m_arrow_vao != 0) {
        glDeleteVertexArrays(1, &m_arrow_vao);
        m_arrow_vao = 0;
    }
}

//void EditRenderer::setup_light() {
//    m_light.Ld = glm::fvec3(0.94, 0.25, 0.25);
//    m_light.La = glm::fvec3(0.91, 0.08, 0.08);
//    m_light.Ls = glm::fvec3(0.82, 0.82, 0.82);
//}

void EditRenderer::setup_light() {
    m_light.Ld = glm::fvec3(0.80f, 0.80f, 0.80f);
    m_light.Ls = glm::fvec3(0.00f, 0.00f, 0.00f);
    m_light.La = glm::fvec3(0.79f, 0.79f, 0.79f);
}

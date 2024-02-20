#include "selection_renderer.h"
#include "globals.h"
#include "matrix_to_vector.h"
#include "sphere_mesh_builder.h"
#include "cylinder_mesh_builder.h"

#include <ca_essentials/core/logger.h>
#include <ca_essentials/core/compute_flat_mesh.h>

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
}

SelectionRenderer::SelectionRenderer(const SelectionHandler& sel_handler)
: m_sel_handler(sel_handler)
{
    setup_shaders();
    setup_buffers();
    setup_light();
}

SelectionRenderer::~SelectionRenderer() {
    delete_buffers();
}

void SelectionRenderer::set_mesh(const ca_essentials::meshes::TriMesh& mesh) {
    m_mesh = &mesh;
}

void SelectionRenderer::set_matrices(const glm::mat4& model,
                                const glm::mat4& view,
                                const glm::mat4& proj,
                                const glm::vec3& cam_pos) {
    m_model_mat = model;
    m_view_mat  = view;
    m_proj_mat  = proj;
    m_cam_pos   = glm::vec4(cam_pos, 1.0);
}

void SelectionRenderer::set_bounding_box(const glm::vec3& bbox_min,
                                         const glm::vec3& bbox_max) {
    namespace selection = globals::render::selection;

    float x_ext = bbox_max.x - bbox_min.x;
    float y_ext = bbox_max.y - bbox_min.y;
    float z_ext = bbox_max.z - bbox_min.z;
    float avg_bbox_ext = (x_ext + y_ext + z_ext) / 3.0f;

    m_sphere_radius = selection::sphere_radius_perc * avg_bbox_ext;
    m_cylinder_radius = m_sphere_radius * 0.2f;
}

void SelectionRenderer::render() {
    namespace selection = globals::render::selection;
    if(!m_mesh)
        return;

    const Eigen::MatrixXd& V = m_mesh->get_vertices();

    const auto& sel_verts = m_sel_handler.get_selection();
    for(int i = 0; i < sel_verts.size(); ++i) {
        bool connected_to_prev = false;
        bool connected_to_next = false;

        if(sel_verts.size() > 1) {
            // Check connection with next
            if(i < (sel_verts.size() - 1)) {
                int curr_vid = sel_verts.at(i);
                int next_vid = sel_verts.at(i + 1);

                connected_to_next = m_mesh->are_vertices_adjacent(curr_vid, next_vid);
            }

            // Check connection with previous
            if(i > 0) {
                int curr_vid = sel_verts.at(i);
                int prev_vid = sel_verts.at(i - 1);

                connected_to_prev = m_mesh->are_vertices_adjacent(curr_vid, prev_vid);
            }
        }

        if(!connected_to_prev || !connected_to_next) {
            int vid = sel_verts.at(i);
            glm::vec3 p0(V(vid, 0), V(vid, 1), V(vid, 2));

            draw_sphere(p0, m_selected_color, m_sphere_radius);
        }

        if(connected_to_next) {
            int curr_vid = sel_verts.at(i);
            int next_vid = sel_verts.at(i + 1);

            glm::vec3 p0(V(curr_vid, 0), V(curr_vid, 1), V(curr_vid, 2));
            glm::vec3 p1(V(next_vid, 0), V(next_vid, 1), V(next_vid, 2));
            draw_cylinder(p0, p1, m_path_color, m_cylinder_radius);
        }
    }

    // Draw hover selection
    {
        glDisable(GL_DEPTH_TEST);
        auto& [hover_sel_on, vid] = m_sel_handler.get_hover_selected_entity();
        if(hover_sel_on) {
            glm::vec3 p(V(vid, 0), V(vid, 1), V(vid, 2));
            draw_sphere(p, m_hover_color, m_sphere_radius * 0.85f);
        }
        glEnable(GL_DEPTH_TEST);
    }
}

void SelectionRenderer::draw_sphere(const glm::vec3& center,
                                    const glm::vec3& color,
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

void SelectionRenderer::draw_cylinder(const glm::vec3& p0, const glm::vec3& p1,
                                      const glm::vec3& c, double radius) {
    m_prog.use();
    glBindVertexArray(m_cylinder_vao);

    m_prog.setUniform("Material.Ks" , m_base_mat_Ks);
    m_prog.setUniform("Material.Shininess", m_base_mat_shininess);
    m_prog.setUniform("Light.Ls", m_light.Ls);

    m_prog.setUniform("Light.Position", m_view_mat * m_cam_pos);
    m_prog.setUniform("Light.Ls", m_light.Ls);
    m_prog.setUniform("Wire.Enabled", false);

    m_prog.setUniform("Material.Kd", c);
    m_prog.setUniform("Material.Ka", c * 0.85f);
    
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

void SelectionRenderer::setup_shaders() {
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
        LOGGER.info("SelectionRenderer Shader Info");
        m_prog.printActiveAttribs();
        LOGGER.info("");
        m_prog.printActiveUniforms();
        LOGGER.info("\n");
    }
    else
        LOGGER.debug("SelectionRenderer successfully created");
}

void SelectionRenderer::setup_buffers() {
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

void SelectionRenderer::update_buffers() {
    namespace render = globals::render;
    namespace core = ca_essentials::core;

    {
        SphereMesh sphere_mesh = build_sphere_mesh(1.0f, render::cylinder_num_radial_slices,
                                                   render::cylinder_num_vertical_slices);
        m_num_sphere_faces = (int) sphere_mesh.T.rows();

        std::vector<float> vecV;
        matrix_to_vector(sphere_mesh.V.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(sphere_mesh.T.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(sphere_mesh.N.cast<float>(), vecN);

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

void SelectionRenderer::delete_buffers() {
    if(!m_sphere_buffers.empty()) {
        glDeleteBuffers((GLsizei) m_sphere_buffers.size(), m_sphere_buffers.data());
        m_sphere_buffers.clear();
    }

    if(m_sphere_vao != 0) {
        glDeleteVertexArrays(1, &m_sphere_vao);
        m_sphere_vao = 0;
    }
}

void SelectionRenderer::setup_light() {
    m_light.Ld = glm::fvec3(0.80f, 0.80f, 0.80f);
    m_light.Ls = glm::fvec3(0.00f, 0.00f, 0.00f);
    m_light.La = glm::fvec3(0.79f, 0.79f, 0.79f);
}

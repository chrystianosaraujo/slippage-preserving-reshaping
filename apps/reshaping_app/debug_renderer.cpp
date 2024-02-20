#include "debug_renderer.h"
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

ca_essentials::renderer::GLSLProgram DebugRenderer::s_prog;

GLuint DebugRenderer::s_sphere_vao = 0;
GLuint DebugRenderer::s_cylinder_vao = 0;
std::vector<GLuint> DebugRenderer::s_sphere_buffers;
std::vector<GLuint> DebugRenderer::s_cylinder_buffers;

glm::mat4 DebugRenderer::s_model_mat;
glm::mat4 DebugRenderer::s_view_mat;
glm::mat4 DebugRenderer::s_proj_mat;

float DebugRenderer::s_radius = 1.0f;
int DebugRenderer::s_num_sphere_tris = 0;
int DebugRenderer::s_num_cylinder_tris = 0;
std::unordered_map<std::string, std::vector<DebugRenderer::SphereInfo>> DebugRenderer::s_spheres;
std::unordered_map<std::string, std::vector<DebugRenderer::CylinderInfo>> DebugRenderer::s_cylinders;

glm::fvec3 DebugRenderer::s_base_mat_Ks = glm::fvec3(0.29f, 0.25f, 0.25f);
float DebugRenderer::s_base_mat_shininess = 100.0f;

Light DebugRenderer::s_light;
glm::vec4 DebugRenderer::s_cam_pos;

void DebugRenderer::init() {
    setup_shaders();
    setup_light();
    setup_buffers();
    update_buffers();
}

void DebugRenderer::clean() {
    delete_buffers();
}

void DebugRenderer::add_sphere(const std::string& label, 
                               const glm::vec3& pos,
                               const glm::vec3& color) {

    if(s_spheres.count(label) == 0)
        s_spheres[label] = {};
    else
        s_spheres[label].push_back({ pos, color });
}

void DebugRenderer::erase_spheres(const std::string& label) {
    s_spheres.erase(label);
}

void DebugRenderer::clear_spheres() {
    s_spheres.clear();
}

void DebugRenderer::add_cylinder(const std::string& label, 
                                 const glm::vec3& p0,
                                 const glm::vec3& p1,
                                 const glm::vec3& color) {
    if(s_cylinders.count(label) == 0)
        s_cylinders[label] = { { p0, p1, color } };
    else
        s_cylinders.at(label).push_back({ p0, p1, color });
}

void DebugRenderer::erase_cylinder(const std::string& id) {
    s_cylinders.erase(id);
}

void DebugRenderer::clear_cylinders() {
    s_cylinders.clear();
}

void DebugRenderer::clear_all() {
    clear_spheres();
    clear_cylinders();
}

void DebugRenderer::set_radius(float radius) {
    s_radius = radius;
}

float& DebugRenderer::get_radius() {
    return s_radius;
}

void DebugRenderer::set_matrices(const glm::mat4& model,
                                 const glm::mat4& view,
                                 const glm::mat4& proj,
                                 const glm::vec3& cam_pos) {
    s_model_mat = model;
    s_view_mat  = view;
    s_proj_mat  = proj;
    s_cam_pos   = glm::vec4(cam_pos, 1.0);
}


void DebugRenderer::set_light(const Light& light) {
   s_light = light;
}

Light& DebugRenderer::get_light() {
    return s_light;
}

void DebugRenderer::render() {
    render_spheres();
    render_cylinders();
}

void DebugRenderer::render_spheres() {
    glm::mat4 scale_mat = glm::mat4(1.0f);
    scale_mat = glm::scale(scale_mat, glm::vec3(s_radius));

    s_prog.use();
    glBindVertexArray(s_sphere_vao);

    s_prog.setUniform("Material.Ks" , s_base_mat_Ks);
    s_prog.setUniform("Material.Shininess", s_base_mat_shininess);
    s_prog.setUniform("Light.Position", s_view_mat * s_cam_pos);
    s_prog.setUniform("Light.Ld", s_light.Ld);
    s_prog.setUniform("Light.La", s_light.La);
    s_prog.setUniform("Light.Ls", s_light.Ls);
    s_prog.setUniform("Wire.Enabled", false);

    // Render Fixed Points 
    for(const auto& [label, spheres] : s_spheres) {
        for(const auto& sphere : spheres) {
            s_prog.setUniform("Material.Kd", sphere.color);
            s_prog.setUniform("Material.Ka", sphere.color * 0.65f);

            glm::mat4 trans_mat(1.0f);
            trans_mat = glm::translate(trans_mat, sphere.pos);

            glm::mat4 mv_mat = s_view_mat * s_model_mat * trans_mat * scale_mat;
            glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(mv_mat));

            s_prog.setUniform("NormalMatrix", normal_mat);
            s_prog.setUniform("ModelViewMatrix", mv_mat);
            s_prog.setUniform("MVP", s_proj_mat * mv_mat);
            glDrawElements(GL_TRIANGLES, (GLsizei)s_num_sphere_tris * 3, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
}

void DebugRenderer::render_cylinders() {
    s_prog.use();
    glBindVertexArray(s_cylinder_vao);

    s_prog.setUniform("Material.Ks" , s_base_mat_Ks);
    s_prog.setUniform("Material.Shininess", s_base_mat_shininess);
    s_prog.setUniform("Light.Ls", s_light.Ls);

    s_prog.setUniform("Light.Position", s_view_mat * s_cam_pos);
    s_prog.setUniform("Light.Ls", s_light.Ls);
    s_prog.setUniform("Wire.Enabled", false);

    double cylinder_radius = s_radius * 0.40;

    for(const auto& [label, cylinders] : s_cylinders) {
        for(const auto& cylinder : cylinders) {
            const auto& p0 = cylinder.p0;
            const auto& p1 = cylinder.p1;
            const auto& c = cylinder.color;

            s_prog.setUniform("Material.Kd", c);
            s_prog.setUniform("Material.Ka", c * 0.85f);

            const double length = glm::length(p1 - p0);

            // Scale
            glm::mat4 scale_mat = glm::mat4(1.0f);
            glm::vec3 scale_vec = glm::vec3(cylinder_radius, cylinder_radius, length);
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
            glm::mat4 mv_mat = s_view_mat * s_model_mat * local_mat;
            glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(mv_mat));

            s_prog.setUniform("NormalMatrix", normal_mat);
            s_prog.setUniform("ModelViewMatrix", mv_mat);
            s_prog.setUniform("MVP", s_proj_mat * mv_mat);
            glDrawElements(GL_TRIANGLES, (GLsizei)s_num_cylinder_tris * 3, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
}

void DebugRenderer::setup_shaders() {
    namespace renderer = ca_essentials::renderer;
    namespace paths = globals::paths;

    try {
        s_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.vs").string().c_str());
        s_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.gs").string().c_str());
        s_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.fs").string().c_str());
        s_prog.link();
        s_prog.use();
    } catch (renderer::GLSLProgramException &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    if constexpr(globals::logging::LOG_SHADER_SETUP_MESSAGES) {
        LOGGER.info("");
        LOGGER.info("DebugRenderer Shader Info");
        s_prog.printActiveAttribs();
        LOGGER.info("");
        s_prog.printActiveUniforms();
        LOGGER.info("\n");
    }
    else
        LOGGER.debug("DebugRenderer successfully created");
}

void DebugRenderer::setup_buffers() {
    if(!s_sphere_buffers.empty() || !s_cylinder_buffers.empty())
        delete_buffers();

    s_sphere_buffers  .resize(BufferType::NUM_BUFFERS);
    s_cylinder_buffers.resize(BufferType::NUM_BUFFERS);
    glGenBuffers((GLsizei) s_sphere_buffers.size(),  &s_sphere_buffers[0]);
    glGenBuffers((GLsizei) s_cylinder_buffers.size(), &s_cylinder_buffers[0]);

    // Sphere Vertex Array
    {
        glGenVertexArrays(1, &s_sphere_vao);
        glBindVertexArray(s_sphere_vao);

        // Index
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_sphere_buffers[BufferType::ELEMENT]);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, s_sphere_buffers[BufferType::POSITION]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, s_sphere_buffers[BufferType::NORMAL]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    // Cylinder Vertex Array
    {
        glGenVertexArrays(1, &s_cylinder_vao);
        glBindVertexArray(s_cylinder_vao);

        // Index
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_cylinder_buffers[BufferType::ELEMENT]);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, s_cylinder_buffers[BufferType::POSITION]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, s_cylinder_buffers[BufferType::NORMAL]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }
}

void DebugRenderer::update_buffers() {
    namespace render = globals::render;
    namespace core = ca_essentials::core;

    {
        SphereMesh sphere_mesh = build_sphere_mesh(1.0f, render::cylinder_num_radial_slices,
                                                   render::cylinder_num_vertical_slices);
        s_num_sphere_tris = (int) sphere_mesh.T.rows();

        std::vector<float> vecV;
        matrix_to_vector(sphere_mesh.V.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(sphere_mesh.T.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(sphere_mesh.N.cast<float>(), vecN);

        // Position Buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_sphere_buffers[BufferType::POSITION]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecV.size(), vecV.data(), GL_STATIC_DRAW);

        // Index Buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_sphere_buffers[BufferType::ELEMENT]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * vecT.size(), vecT.data(), GL_STATIC_DRAW);

        // Normal Buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_sphere_buffers[BufferType::NORMAL]);
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

        s_num_cylinder_tris = (int) Tf.rows();

        std::vector<float> vecV;
        matrix_to_vector(Vf.cast<float>(), vecV);

        std::vector<GLuint> vecT;
        matrix_to_vector(Tf.cast<GLuint>(), vecT);

        std::vector<float> vecN;
        matrix_to_vector(Nf.cast<float>(), vecN);

        // Position Buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_cylinder_buffers[BufferType::POSITION]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecV.size(), vecV.data(), GL_STATIC_DRAW);

        // Index Buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_cylinder_buffers[BufferType::ELEMENT]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * vecT.size(), vecT.data(), GL_STATIC_DRAW);

        // Normal Buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_cylinder_buffers[BufferType::NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vecN.size(), vecN.data(), GL_STATIC_DRAW);
    }
}

void DebugRenderer::delete_buffers() {
    if(!s_sphere_buffers.empty()) {
        glDeleteBuffers((GLsizei) s_sphere_buffers.size(), s_sphere_buffers.data());
        s_sphere_buffers.clear();
    }

    if(!s_cylinder_buffers.empty()) {
        glDeleteBuffers((GLsizei) s_cylinder_buffers.size(), s_cylinder_buffers.data());
        s_cylinder_buffers.clear();
    }

    if(s_sphere_vao != 0) {
        glDeleteVertexArrays(1, &s_sphere_vao);
        s_sphere_vao = 0;
    }

    if(s_cylinder_vao != 0) {
        glDeleteVertexArrays(1, &s_cylinder_vao);
        s_cylinder_vao = 0;
    }
}

void DebugRenderer::setup_light() {
    s_light.Ld = glm::fvec3(0.80f, 0.80f, 0.80f);
    s_light.Ls = glm::fvec3(0.00f, 0.00f, 0.00f);
    s_light.La = glm::fvec3(0.79f, 0.79f, 0.79f);
}

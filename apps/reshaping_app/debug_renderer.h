#pragma once

#include <ca_essentials/renderer/glslprogram.h>
#include <ca_essentials/renderer/material.h>
#include <ca_essentials/renderer/light.h>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <unordered_map>

#include <vector>

class DebugRenderer {
public:
    static void init();
    static void clean();

    static void add_sphere(const std::string& label, 
                           const glm::vec3& pos,
                           const glm::vec3& color);
    static void erase_spheres(const std::string& label);
    static void clear_spheres();

    static void add_cylinder(const std::string& label, 
                             const glm::vec3& p0,
                             const glm::vec3& p1,
                             const glm::vec3& color);
    static void erase_cylinder(const std::string& id);
    static void clear_cylinders();

    static void clear_all();

    static void set_radius(float radius);
    static float& get_radius();

    static void set_matrices(const glm::mat4& model,
                             const glm::mat4& view,
                             const glm::mat4& proj,
                             const glm::vec3& cam_pos);
    
    static void set_light(const Light& light);
    static Light& get_light();

    static void render();

private:
    static void setup_shaders();
    static void setup_buffers();
    static void update_buffers();
    static void delete_buffers();

    static void setup_light();

    static void render_spheres();
    static void render_cylinders();

private:
    struct SphereInfo {
        glm::vec3 pos;
        glm::vec3 color;
    };

    struct CylinderInfo {
        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 color;
    };

private:
    static ca_essentials::renderer::GLSLProgram s_prog;
    static GLuint s_sphere_vao;
    static GLuint s_cylinder_vao;
    static std::vector<GLuint> s_sphere_buffers;
    static std::vector<GLuint> s_cylinder_buffers;

    // Matrices
    static glm::mat4 s_model_mat;
    static glm::mat4 s_view_mat;
    static glm::mat4 s_proj_mat;

    static float s_radius;
    static int s_num_sphere_tris;
    static std::unordered_map<std::string, std::vector<SphereInfo>> s_spheres;

    static int s_num_cylinder_tris;
    static std::unordered_map<std::string, std::vector<CylinderInfo>> s_cylinders;

    static glm::fvec3 s_base_mat_Ks;
    static float s_base_mat_shininess;

    static Light s_light;
    static glm::vec4 s_cam_pos;
};

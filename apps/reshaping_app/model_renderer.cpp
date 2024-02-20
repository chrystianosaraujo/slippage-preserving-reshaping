#include "model_renderer.h"
#include "globals.h"
#include "matrix_to_vector.h"
#include "color_scheme_data.h"

#include <ca_essentials/core/logger.h>
#include <ca_essentials/core/compute_flat_mesh.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <igl/per_vertex_normals.h>

namespace {
enum BufferType {
    POSITION = 0,
    NORMAL = 1,
    ELEMENT = 2,

    LAST_BUFFER = ELEMENT,
    NUM_BUFFERS = LAST_BUFFER + 1
};
}

ModelRenderer::ModelRenderer() {
    init();
}

ModelRenderer::ModelRenderer(const Eigen::MatrixXd& V, 
                             const Eigen::MatrixXi& T,
                             const Eigen::MatrixXd& N) {
    namespace core = ca_essentials::core;

    init();

    Eigen::MatrixXd Vf;
    Eigen::MatrixXi Tf;
    Eigen::MatrixXd Nf;
    core::compute_flat_mesh(V, T, Vf, Tf, Nf);

    set_mesh(Vf, Tf, Nf);
    //set_mesh(V, T, N);
}

ModelRenderer::ModelRenderer(const std::vector<float>& V,
                             const std::vector<GLuint>& T,
                             const std::vector<float>& N) {
    init();
    set_mesh(V, T, N);
}

ModelRenderer::~ModelRenderer() {
    delete_buffers();
}

void ModelRenderer::set_mesh(const Eigen::MatrixXd& V, 
                             const Eigen::MatrixXi& T,
                             const Eigen::MatrixXd& N) {
    namespace core = ca_essentials::core;

    Eigen::MatrixXd Vf;
    Eigen::MatrixXi Tf;
    Eigen::MatrixXd Nf;
    core::compute_flat_mesh(V, T, Vf, Tf, Nf);

    std::vector<float> vecV;
    matrix_to_vector(Vf.cast<float>(), vecV);

    std::vector<GLuint> vecT;
    matrix_to_vector(Tf.cast<GLuint>(), vecT);

    std::vector<float> vecN;
    matrix_to_vector(Nf.cast<float>(), vecN);

    set_mesh(vecV, vecT, vecN);
}

void ModelRenderer::set_mesh(const std::vector<float>&  V,
                             const std::vector<GLuint>& T,
                             const std::vector<float>&  N) {
    update_buffers_data(V, T, N);
}

void ModelRenderer::set_matrices(const glm::mat4& model,
                                 const glm::mat4& view,
                                 const glm::mat4& proj,
                                 const glm::mat4& viewport,
                                 const glm::vec3& cam_pos,
                                 const glm::vec3& center) {
    m_model_mat = glm::mat4(1.0);

    m_view_mat  = view;
    m_proj_mat  = proj;
    m_viewport_mat  = viewport;
    m_modelview_mat = m_view_mat * m_model_mat;
    m_normal_mat    = glm::inverseTranspose(glm::mat3(m_modelview_mat));

    m_cam_pos = glm::vec4(cam_pos, 1.0);
    m_center = center;
}

void ModelRenderer::render() {
    if(m_vao == 0)
        return;

    if(!is_surface_rendering_enabled() && !is_wireframe_enabled())
        return;

    m_prog.setUniform("ViewportMatrix" , m_viewport_mat);
    m_prog.setUniform("ModelViewMatrix", m_modelview_mat);
    m_prog.setUniform("NormalMatrix"   , m_normal_mat);
    m_prog.setUniform("MVP"            , m_proj_mat * m_modelview_mat);

    m_prog.setUniform("Wire.Color", m_wireframe_color);
    m_prog.setUniform("Wire.EdgeWidth", m_wireframe_edge_width);
    m_prog.setUniform("Wire.BlendWidth", m_wireframe_blend_width);
    m_prog.setUniform("Wire.Enabled", m_wireframe_enabled);
    m_prog.setUniform("Surface.Enabled", m_surface_enabled);
    m_prog.setUniform("Material.Kd", m_material.Kd);
    m_prog.setUniform("Material.Ka", m_material.Kd);
    m_prog.setUniform("Material.Ks", m_material.Ks);
    m_prog.setUniform("Material.Shininess", m_material.shininess);
    m_prog.setUniform("Light.Ld", m_light.Ld);
    m_prog.setUniform("Light.La", m_light.La);
    m_prog.setUniform("Light.Ls", m_light.Ls);

    glm::vec4 light_pos = m_cam_pos;

    m_prog.setUniform("Light.Position", m_view_mat * light_pos);
    //m_prog.setUniform("Light.Position", m_view_mat * m_cam_pos);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_num_tris * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ModelRenderer::enable_wireframe(bool val) {
    m_wireframe_enabled = val;
}

bool ModelRenderer::is_wireframe_enabled() const {
    return m_wireframe_enabled;
}

void ModelRenderer::set_wireframe_edge_width(float width) {
    m_wireframe_edge_width = width;
}

float ModelRenderer::get_wireframe_edge_width() const {
    return m_wireframe_edge_width;
}

void ModelRenderer::set_wireframe_blend_width(float width) {
    m_wireframe_blend_width = width;
}

float ModelRenderer::get_wireframe_blend_width() const {
    return m_wireframe_blend_width;
}

void ModelRenderer::set_wireframe_color(const glm::vec4& color) {
    m_wireframe_color = color;
}

const glm::vec4& ModelRenderer::get_wireframe_color() const {
    return m_wireframe_color;
}

void ModelRenderer::enable_surface_rendering(bool val) {
    m_surface_enabled = val;
}

bool ModelRenderer::is_surface_rendering_enabled() const {
    return m_surface_enabled;
}

void ModelRenderer::set_material(const Material& mat) {
    m_material = mat;
}

const Material& ModelRenderer::get_material() const {
    return m_material;
}

void ModelRenderer::set_light(const Light& light) {
    m_light = light;
}

const Light& ModelRenderer::get_light() const {
    return m_light;
}

void ModelRenderer::init() {
    setup_shaders();
    setup_buffers();
    setup_default_light();
    setup_default_material();
}

void ModelRenderer::setup_shaders() {
    namespace paths = globals::paths;
    using GLSLProgramException = ca_essentials::renderer::GLSLProgramException;

    try {
        m_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.vs").string().c_str());
        m_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.gs").string().c_str());
        m_prog.compileShader((paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.fs").string().c_str());
        m_prog.link();
        m_prog.use();
    }
    catch(GLSLProgramException& e) {
        LOGGER.error("Error while setting up ModelRenderer::shaders\n    {}", e.what());
        exit(EXIT_FAILURE);
    }

    if constexpr(globals::logging::LOG_SHADER_SETUP_MESSAGES) {
        LOGGER.info("");
        LOGGER.info("ModelRenderer Shader Info");
        m_prog.printActiveAttribs();
        LOGGER.info("");
        m_prog.printActiveUniforms();
        LOGGER.info("\n");
    }
    else
        LOGGER.debug("ModelRenderer successfully created");
}

void ModelRenderer::setup_buffers() {
    if(!m_buffers.empty())
        delete_buffers();

    m_buffers.resize(BufferType::NUM_BUFFERS);
    glGenBuffers((GLsizei) m_buffers.size(), &m_buffers.at(0));

    // Vertex Array
    {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // Index
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[BufferType::ELEMENT]);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, m_buffers[BufferType::POSITION]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, m_buffers[BufferType::NORMAL]);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }
}

void ModelRenderer::update_buffers_data(const std::vector<float>& V,
                                        const std::vector<GLuint>& T,
                                        const std::vector<float>& N) {
    m_num_verts = (GLuint) V.size() / 3;
    m_num_tris  = (GLuint) T.size() / 3;

    // Position Buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[BufferType::POSITION]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * V.size(), V.data(), GL_STATIC_DRAW);
    //Eigen::MatrixXf Vv(m_num_verts, 3);
    //for(int i = 0; i < m_num_verts; ++i) {
    //    Vv(i, 0) = V[i * 3 + 0];
    //    Vv(i, 1) = V[i * 3 + 1];
    //    Vv(i, 2) = V[i * 3 + 2];
    //}
    //glBindBuffer(GL_ARRAY_BUFFER, m_buffers[BufferType::POSITION]);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Vv.size(), Vv.data(), GL_STATIC_DRAW);

    // Index Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[BufferType::ELEMENT]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * T.size(), T.data(), GL_STATIC_DRAW);

    // Normal Buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[BufferType::NORMAL]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * N.size(), N.data(), GL_STATIC_DRAW);
}

void ModelRenderer::delete_buffers() {
    if(!m_buffers.empty()) {
        glDeleteBuffers((GLsizei) m_buffers.size(), m_buffers.data());
        m_buffers.clear();
    }

    if(m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void ModelRenderer::setup_default_light() {
    m_light = get_colorscheme_light(ColorScheme::BLUE);
}

void ModelRenderer::setup_default_material() {
    m_material = get_colorscheme_material(ColorScheme::BLUE);
}

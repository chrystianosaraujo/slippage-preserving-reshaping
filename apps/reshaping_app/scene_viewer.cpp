#include "scene_viewer.h"

#include "selection_handler.h"
#include "crop_frame.h"
#include "debug_renderer.h"

#include <ca_essentials/core/logger.h>
#include <ca_essentials/core/crop_image.h>
#include <ca_essentials/core/geom_utils.h>
#include <ca_essentials/core/bounding_box.h>
#include <ca_essentials/ui/imgui_utils.h>
#include <ca_essentials/renderer/gl_debug.h>
#include <ca_essentials/renderer/texture_provider.h>

#include <ca_essentials/meshes/load_trimesh.h>
#include <ca_essentials/core/bounding_box.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <igl/per_vertex_normals.h>

#include <iostream>
#include <cstdio>
#include <numeric>

#include <stb_image_write.h>

namespace {

static Eigen::Matrix4f glm_to_eigen_mat(const glm::mat4& in_mat) {
    Eigen::Matrix4f out_mat;
    for(int i = 0; i < 4; ++i)
        for(int j = 0; j < 4; ++j)
            out_mat(j, i) = in_mat[i][j];

    return out_mat;
}

static Eigen::Vector4f glm_to_eigen_vec(const glm::vec4& v) {
    return Eigen::Vector4f(v.x, v.y, v.z, v.w);
}

static glm::fvec3 eigen_to_glm_fvec3(const Eigen::Vector3f& v) {
    return glm::fvec3(v.x(), v.y(), v.z());
}

static glm::vec3 eigen_to_glm_vec3(const Eigen::Vector3d& v) {
    return glm::vec3(v.x(), v.y(), v.z());
}

struct ToolbarButtonInfo {
    std::string stem;
    std::string tip;
};
ToolbarButtonInfo TOOLBAR_INFO[] = {
    {"default", "Default"},
    {"picking", "Vertex Selection"},
    {"anchors","Anchor Vertices"},
    {"handles", "Handle Vertices"},
    {"eraser", "Clear Vertices"},
    {"move", "Move"},
    {"scale", "Scale"},
    {"rotate", "Rotate"},
    {"transform", "Transform"},
    {"scale_cage", "Scale Cage"},
};

}

SceneViewer::SceneViewer(int width, int height) {
    setup_ui_icons();
    setup_toolbar();
    setup_input_event_notifier();
    setup_selection_handler();
    setup_reshaping_editor();
    setup_camera();
    setup_renderers();

    m_rebuild_screen_fbo = true;
    m_rebuild_snapshot_fbo = true;
}

SceneViewer::~SceneViewer() {
    namespace renderer = ca_essentials::renderer;
    renderer::TextureProvider::clean();
}

void SceneViewer::set_model(const reshaping::TriMesh& mesh) {
    m_mesh = &mesh;
    clear_reshaping_operation();

    mesh_updated(true);
    clear_reshaping_editor();
}

void SceneViewer::set_reshaping_operation(const std::unordered_set<int>& fixed_points,
                                          const std::vector<std::pair<int, Eigen::Vector3d>>& displacements) {
    m_reshaping_editor->set_edit(fixed_points, displacements);
}

void SceneViewer::set_straight_chains(const reshaping::StraightChains* chains) {
    m_chains_renderer->set_straight_chains(chains);
}

void SceneViewer::reset_mode() {
    m_mode = Mode::CAMERA_MANIP;
    for(int i = 0; i < m_per_mode_status.size(); ++i)
        m_per_mode_status.at(i) = false;

    m_per_mode_status.at((int) Mode::CAMERA_MANIP) = true;
    m_reshaping_editor->set_mode(ReshapingEditor::Mode::NONE);
}

void SceneViewer::model_geometry_updated() {
    mesh_updated(false);
}

CameraInfo SceneViewer::get_camera_info() const {
    return m_cam->get_camera_info();
}

void SceneViewer::set_camera_info(const CameraInfo& info) {
    m_cam->set_camera_info(info);
    prograte_camera_change();
}

double SceneViewer::get_camera_fov() const {
    return m_cam->get_fov();
}

reshaping::EditOperation SceneViewer::get_current_reshaping_edit() const {
    const auto& fixed_points = m_reshaping_editor->fixed_points();
    const auto& handles = m_reshaping_editor->handles();

    reshaping::EditOperation edit_op;
    for(int vid : fixed_points)
        edit_op.displacements.insert({vid, Eigen::Vector3d::Zero()});

    double diag_len = m_bbox.diagonal().norm();
    for(const auto& handle : handles) {
        Eigen::Vector3d disp = (handle.new_pos - handle.orig_pos) / diag_len;
        edit_op.displacements.insert({handle.vid, disp});
    }

    return edit_op;
}

void SceneViewer::clear_reshaping_editor() {
    m_reshaping_editor->clear();
}

void SceneViewer::clear_editor_fixed_points() {
    m_reshaping_editor->erase_fixed_points();
}

void SceneViewer::clear_editor_handles() {
    m_reshaping_editor->erase_handles();
}

void SceneViewer::reset_handle_displacements() {
    m_reshaping_editor->reset_handle_displacements();
}

void SceneViewer::enable_transparent_mode(bool val) {
    namespace render = globals::render::model; 
    
    if(val == is_transparent_mode_enabled())
        return;

    float wireframe_edge_width  = 0.0f;
    float wireframe_blend_width = 0.0f;
    if(val) {
        wireframe_edge_width = render::transparent_mode_wireframe_edge_width;
        wireframe_blend_width = render::transparent_mode_wireframe_blend_width;

        m_model_renderer->enable_surface_rendering(false);
        m_model_renderer->enable_wireframe(true);
    }
    else {
        wireframe_edge_width  = render::normal_mode_wireframe_edge_width;
        wireframe_blend_width = render::normal_mode_wireframe_blend_width;

        m_model_renderer->enable_surface_rendering(true);
        m_model_renderer->enable_wireframe(false);
    }

    m_transparent_mode_on = val;
    m_model_renderer->set_wireframe_blend_width(wireframe_blend_width);
    m_model_renderer->set_wireframe_edge_width(wireframe_edge_width);
}

bool SceneViewer::is_transparent_mode_enabled() const {
    return m_transparent_mode_on;
}

void SceneViewer::enable_surface_rendering(bool val) {
    m_model_renderer->enable_surface_rendering(val);
}

bool SceneViewer::is_surface_rendering_enabled() const {
    return m_model_renderer->is_surface_rendering_enabled();
}

void SceneViewer::enable_wireframe_rendering(bool val) {
    m_model_renderer->enable_wireframe(val);
}

bool SceneViewer::is_wireframe_rendering_enabled() const {
    return m_model_renderer->is_wireframe_enabled();
}

void SceneViewer::enable_displacement_rendering(bool val) {
    m_reshaping_editor->enable_displacement_rendering(val);
}

bool SceneViewer::is_displacement_rendering_enabled() const {
    return m_reshaping_editor->is_displacement_rendering_enabled();
}

void SceneViewer::enable_edit_operation_rendering(bool val) {
    m_edit_op_rendering_on = val;
}

bool SceneViewer::is_edit_operation_rendering_enabled() const {
    return m_edit_op_rendering_on;
}

void SceneViewer::enable_straight_render(bool val) {
    m_straight_rendering_on = val;
}

bool SceneViewer::is_straight_render_enabled() const {
    return m_straight_rendering_on;
}

void SceneViewer::enable_debug_render(bool val) {
    m_debug_rendering_on = val;
}

bool SceneViewer::is_debug_render_enabled() const {
    return m_debug_rendering_on;
}

void SceneViewer::set_snapshot_size_mult(int mult) {
    if(m_snapshot_size_mult == mult)
        return;

    m_snapshot_size_mult = mult;
    m_rebuild_snapshot_fbo = true;
}

int SceneViewer::get_snapshot_size_mult() const {
    return m_snapshot_size_mult;
}

void SceneViewer::set_snapshot_msaa(int msaa) {
    if(m_snapshot_msaa == msaa)
        return;

    m_snapshot_msaa = msaa;
    m_rebuild_snapshot_fbo = true;
}

int SceneViewer::get_snapshot_msaa() const {
    return m_snapshot_msaa;
}

void SceneViewer::set_screen_msaa(int msaa) {
    if(m_screen_msaa == msaa)
        return;

    m_screen_msaa = msaa;
    m_rebuild_screen_fbo = true;
}

int SceneViewer::get_screen_msaa() const {
    return m_screen_msaa;
}

void SceneViewer::enable_crop_frame_rendering(bool val) {
    m_crop_frame_on = val;
}    

bool SceneViewer::is_crop_frame_rendering_enabled() const {
    return m_crop_frame_on;
}

void SceneViewer::render() {
    DebugRenderer::clear_all();

    update_active_mode();

    render_to_widget();
    if(m_snapshot_enabled)
        render_to_snapshot();
}

void SceneViewer::save_screenshot(const std::string& fn) {
    m_snapshot_fn = fn;
    m_snapshot_enabled = true;

    if(m_rebuild_snapshot_fbo)
        setup_snapshot_fbo();
}

void SceneViewer::setup_camera() {
    glm::ivec2 screen_size = glm::ivec2(m_screen_size.x(),
                                        m_screen_size.y());

    // fov = 20deg -> Make sure the perspective effect is small (almost orthographic)
    m_cam = std::make_unique<Camera>(screen_size, 20.0);
}

void SceneViewer::setup_renderers() {
    namespace edit = globals::render::edit;

    DebugRenderer::init();
    m_model_renderer = std::make_unique<ModelRenderer>();
    m_chains_renderer = std::make_unique<StraightChainsRenderer>();
    m_selection_renderer = std::make_unique<SelectionRenderer>(*m_selection_handler);

    enable_transparent_mode(false);
}

void SceneViewer::setup_ui_icons() {
    namespace paths = globals::paths;
    namespace renderer = ca_essentials::renderer;

    for(int i = 0; i < (int) Mode::NUM_MODES; ++i) {
        const auto& info = TOOLBAR_INFO[i];
        renderer::TextureProvider::add_texture((paths::ASSETS_DIR / (info.stem + ".png")).string(), info.stem);
    }
}

void SceneViewer::setup_toolbar() {
    namespace renderer = ca_essentials::renderer;

    auto get_texture_id = [](const std::string& id) -> GLuint {
        auto res = renderer::TextureProvider::texture_id(id);
        assert(res.first && "Invalid texture id: " + id);

        return res.first;
    };

    m_per_mode_status.resize(SceneViewer::NUM_MODES, 0);
    m_per_mode_status.at((int) m_mode) = 1;

    m_toolbar = std::make_unique<ca_essentials::ui::ImGuiToolbar>();
    auto add_toolbar_button = [this](Mode mode) {
        int mode_idx = (int) mode;
        const std::string& id  = TOOLBAR_INFO[mode_idx].stem;
        const std::string& tip = TOOLBAR_INFO[mode_idx].tip;

        auto res = renderer::TextureProvider::texture_id(id);
        assert(res.first && "Invalid texture id: " + id);

        m_toolbar->add_button(res.second, tip, &this->m_per_mode_status.at(mode_idx));

    };

    add_toolbar_button(Mode::CAMERA_MANIP);
    add_toolbar_button(Mode::VERT_SELECTION);
    add_toolbar_button(Mode::HANDLE_SELECTION);
    add_toolbar_button(Mode::ANCHOR_SELECTION);
    add_toolbar_button(Mode::CLEAR_SELECTION);
    m_toolbar->add_separator();
    add_toolbar_button(Mode::TRANSLATE);
    add_toolbar_button(Mode::ROTATE);
    add_toolbar_button(Mode::SCALE);
    add_toolbar_button(Mode::TRANSFORM);
    add_toolbar_button(Mode::SCALE_CAGE);
}

void SceneViewer::setup_input_event_notifier() {
    InputEventNotifier::get().listen_to([this](const MouseEventInfo& e) {
        this->process_mouse_event(e);
    }, true);
}

void SceneViewer::setup_selection_handler() {
    m_selection_handler = std::make_unique<SelectionHandler>();
    m_selection_handler->set_type(SelectionType::VERTEX);
    m_selection_handler->enable_hover_selection(true);
}

void SceneViewer::setup_reshaping_editor() {
    m_reshaping_editor = std::make_unique<ReshapingEditor>(*m_selection_handler);
}

void SceneViewer::process_events() {
    ImGuiIO& io = ImGui::GetIO();
    
    if(!ImGui::IsItemHovered())
        return;

    ImVec2 widget_pos = ImGui::GetItemRectMin();
    ImVec2 mouse_pos = {
        io.MousePos.x - widget_pos.x,
        m_screen_size.y() - (io.MousePos.y - widget_pos.y),
    };

    m_widget_pos << (int) widget_pos.x, (int) widget_pos.y;

    bool is_gizmo_enabled = m_reshaping_editor->is_gizmo_active();

    // Process mouse wheel
    if(!is_gizmo_enabled) {
        double xoffset = io.MouseWheelH;
        double yoffset = io.MouseWheel;

        if(xoffset != 0 || yoffset != 0) {
            // On some setups, shift flips the scroll direction, so take the max
            // scrolling in any direction
            double max_scroll = xoffset;
            if(std::abs(yoffset) > std::abs(xoffset))
                max_scroll = yoffset;

            // Pass camera commands to the camera
            if(max_scroll != 0.0)
                m_cam->process_zoom(max_scroll);
        }
    }

    // Process buttons press/release/drag
    if(!is_gizmo_enabled) {
        // Process drags
        bool drag_left = ImGui::IsMouseDragging(0);
        bool drag_right = !drag_left && ImGui::IsMouseDragging(1);

        if(drag_left || drag_right) {
            glm::vec2 drag_delta{
                io.MouseDelta.x / m_screen_size.x(),
               -io.MouseDelta.y / m_screen_size.y()
            };

            // exactly one of these will be true
            bool is_rotate = drag_left && !io.KeyShift && !io.KeyCtrl;
            bool is_translate = (drag_left && io.KeyShift && !io.KeyCtrl) || drag_right;
            bool is_drag_zoom = drag_left && io.KeyCtrl;

            if(is_drag_zoom)
                m_cam->process_zoom(drag_delta.y * 8.0);

            if(is_translate)
                m_cam->process_translate(drag_delta * 2.0f);

            if(is_rotate) {
                glm::vec2 curr_pos{
                    mouse_pos.x / m_screen_size.x(),
                    (m_screen_size.y() - mouse_pos.y) / m_screen_size.y()
                };

                curr_pos = (curr_pos * 2.0f) - glm::vec2{ 1.0, 1.0 };
                if(std::abs(curr_pos.x) <= 1.0 && std::abs(curr_pos.y) <= 1.0) {
                    m_cam->process_rotate(curr_pos - 2.0f * drag_delta, curr_pos);
                }
            }
        }
    }
    prograte_camera_change();
    update_footer_messages();
}

void SceneViewer::render_to_widget() {
    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);

    // Checking if the viewport size has changed and notify if so
    Eigen::Vector2i viewport_size;
    ImGui::Begin("SceneViewer");
    {
        viewport_size << (int) ImGui::GetContentRegionAvail().x,
                         (int) ImGui::GetContentRegionAvail().y;
        bool viewport_changed = m_screen_fbo.width()  != viewport_size.x() ||
                                m_screen_fbo.height() != viewport_size.y();
        if(viewport_changed)
            screen_size_changed(viewport_size.x(), viewport_size.y());
    }
    ImGui::End();

    ImGui::Begin("SceneViewer");
    {
        if(m_rebuild_screen_fbo)
            setup_screen_fbo();

        // Render to texture
        m_screen_fbo.bind();
        glViewport(0, 0, m_screen_fbo.width(), m_screen_fbo.height());
        render_scene();
        m_screen_fbo.blit_to_single_sampling_fbo();
        m_screen_fbo.unbind();

        ImVec2 widget_pos = ImGui::GetCursorScreenPos();

        // Add texture to screen
        GLuint tex_id = m_screen_fbo.texture_id();
        ImGui::Image(reinterpret_cast<void*>((intptr_t) tex_id),
                     ImVec2((float) viewport_size.x(), (float) viewport_size.y()),
                     ImVec2{0, 1}, ImVec2{1, 0});

        m_reshaping_editor->render_gizmo();
        if(is_crop_frame_rendering_enabled())
            render_crop_frame();

        process_events();
        InputEventNotifier::get().process_inputs();

        m_toolbar->render_imgui(widget_pos);
        render_footer_message(ImVec2((float) viewport_size.x(), (float) viewport_size.y()));
    }

    ImGui::End();
}

void SceneViewer::render_to_snapshot() {
    if(m_rebuild_snapshot_fbo)
        setup_snapshot_fbo();

    m_snapshot_fbo.bind();
    glViewport(0, 0, m_snapshot_fbo.width(), m_snapshot_fbo.height());
    render_scene();
    m_snapshot_fbo.blit_to_single_sampling_fbo();
    m_snapshot_fbo.unbind();

    save_snapshot();
}

void SceneViewer::render_scene() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_model_renderer->render();
    if(is_edit_operation_rendering_enabled())
        m_reshaping_editor->render_reshaping_edit();

    m_selection_renderer->render();

    if(is_debug_render_enabled())
        DebugRenderer::render();
}

void SceneViewer::render_crop_frame() {
    Eigen::Vector4i frame = get_crop_frame();

    auto pos = ImGui::GetWindowPos();
    
    Eigen::Vector4i rect;
    rect(0) = (int) pos.x;
    rect(1) = (int) pos.y;
    rect(2) = m_screen_size.x();
    rect(3) = m_screen_size.y();

    ImU32 im_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::GetForegroundDrawList()->AddRect(ImVec2((float) rect(0), (float) rect(1)),
                                            ImVec2((float) rect(2), (float) rect(3)),
                                            im_color,
                                            0.0f,
                                            0,
                                            14.0f);
}

void SceneViewer::set_footer_message(const std::string& fn) {
    m_footer_messages.clear();
    m_footer_messages.push_back(fn);
}

void SceneViewer::render_footer_message(const ImVec2& win_size) {
    namespace ui = globals::ui;

    if(m_footer_messages.empty())
        return;

    float line_height = ImGui::GetTextLineHeight();

    float margin_x = 10.0f;
    float margin_y = 0.0f;
    float line_padding_y = 0.0f;

    ImVec2 win_pos = ImVec2(0.0f, 0.0f);
    ImVec2 footer_size = ImVec2(0.0f, line_height * m_footer_messages.size());
    ImVec2 footer_pos = ImVec2(win_pos.x + margin_x, win_pos.y + win_size.y - footer_size.y - margin_y);

    const Eigen::Vector3f& c = ui::footer_message_color;
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(c.x() * 255.0f, c.y() * 255.0f, c.z() * 255.0f, 255.0f));

    ImVec2 curr_pos = footer_pos;
    for(const auto& msg : m_footer_messages) {
        ImGui::SetCursorPos(curr_pos);
        ImGui::Text(msg.c_str());

        curr_pos.y += line_height + line_padding_y;
    }

    ImGui::PopStyleColor();
}

void SceneViewer::update_footer_messages() {
    m_footer_messages.clear();
    if(m_mode == Mode::VERT_SELECTION) {
        const auto& [is_hover, vid] = m_selection_handler->get_hover_selected_entity();

        if(is_hover)
            m_footer_messages.push_back("Vertex: " + std::to_string(vid));
    }
}


void SceneViewer::mesh_updated(bool force_reset) {
    recompute_mesh_data();

    m_selection_handler->set_mesh(*m_mesh);
    m_reshaping_editor->set_mesh(*m_mesh);
    update_renderers_mesh_data();
    if(force_reset)
        reset_camera();
}

void SceneViewer::prograte_camera_change() {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view  = m_cam->get_view_matrix();
    glm::mat4 proj  = m_cam->get_perspective_matrix();
    glm::mat4 viewport_mat = m_cam->get_viewport_matrix();
    glm::vec4 viewport = m_cam->get_viewport();
    glm::vec3 cam_pos = m_cam->get_world_position();

    m_selection_handler->set_camera_info(glm_to_eigen_mat(model),
                                         glm_to_eigen_mat(view),
                                         glm_to_eigen_mat(proj),
                                         glm_to_eigen_vec(viewport));
    auto eigen_c = m_bbox.center();
    glm::vec3 center = glm::vec3(eigen_c.x(), eigen_c.y(), eigen_c.z());

    m_model_renderer->set_matrices(model, view, proj, viewport_mat, cam_pos, center);
    m_reshaping_editor->set_matrices(model, view, proj, cam_pos, center);
    m_selection_renderer->set_matrices(model, view, proj, cam_pos);
    m_chains_renderer->set_matrices(model, view, proj, cam_pos);
    DebugRenderer::set_matrices(model, view, proj, cam_pos);
}

void SceneViewer::recompute_mesh_data() {
    namespace core = ca_essentials::core;

    // Updating bounding box
    m_bbox = core::bounding_box(m_mesh->get_vertices());
}

void SceneViewer::update_renderers_mesh_data() {
    namespace core = ca_essentials::core;
    namespace debug = globals::render::debug;

    const Eigen::MatrixXd& V = m_mesh->get_vertices();
    const Eigen::MatrixXi& T = m_mesh->get_facets();
    Eigen::MatrixXd N;

    float avg_ext = (float) (m_bbox.sizes().sum() / 3.0);

    // Computing normals
    igl::per_vertex_normals(V, T, N); 

    m_model_renderer->set_mesh(V, T, N);
    m_selection_renderer->set_mesh(*m_mesh);

    m_selection_renderer->set_bounding_box(eigen_to_glm_fvec3(m_bbox.min().cast<float>()),
                                           eigen_to_glm_fvec3(m_bbox.max().cast<float>()));

    m_chains_renderer->set_geometry(V);
    m_chains_renderer->set_bounding_box(eigen_to_glm_fvec3(m_bbox.min().cast<float>()),
                                        eigen_to_glm_fvec3(m_bbox.max().cast<float>()));

    DebugRenderer::set_radius(debug::sphere_radius * avg_ext);
}

void SceneViewer::reset_camera() {
    glm::vec3 bbmin = eigen_to_glm_fvec3(m_bbox.min().cast<float>());
    glm::vec3 bbmax = eigen_to_glm_fvec3(m_bbox.max().cast<float>());

    m_cam->set_bounding_box(bbmin, bbmax);
    m_cam->set_length_scale(glm::length(bbmax - bbmin));
    m_cam->reset_camera_to_home_view();
    prograte_camera_change();
}

void SceneViewer::update_active_mode() {
    // The active mode is only properly updated after this function is called.
    // Before that, m_mode and m_per_mode_status are invalid
    bool changed = false;

    int sum = std::accumulate(m_per_mode_status.begin(), m_per_mode_status.end(), 0);
    if(sum == 0) {
        m_per_mode_status.at((int) m_mode) = 1;
        changed = true;
    }
    else if(sum == 2) {
        // Another mode was activated, so disable change the current one's status
        // and change m_mode to the new one
        m_per_mode_status.at((int) m_mode) = 0;

        auto itr = std::find(m_per_mode_status.begin(), m_per_mode_status.end(), 1);
        int idx = (int) std::distance(m_per_mode_status.begin(), itr);
        m_mode = (Mode) idx;
        changed = true;
    }

    if(changed)
        update_reshaping_editor_mode();
}

void SceneViewer::update_reshaping_editor_mode() {
    if(m_mode == Mode::VERT_SELECTION)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::NONE);
    else if(m_mode == Mode::HANDLE_SELECTION)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::HANDLES_SELECTION);
    else if(m_mode == Mode::ANCHOR_SELECTION)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::FIXED_POINTS_SELECTION);
    else if(m_mode == Mode::CLEAR_SELECTION)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::ERASE_SELECTION);
    else if(m_mode == Mode::TRANSLATE)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::TRANSLATE_HANDLES);
    else if(m_mode == Mode::SCALE)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::SCALE_HANDLES);
    else if(m_mode == Mode::ROTATE)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::ROTATE_HANDLES);
    else if(m_mode == Mode::TRANSFORM)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::TRANSFORM_HANDLES);
    else if(m_mode == Mode::SCALE_CAGE)
        m_reshaping_editor->set_mode(ReshapingEditor::Mode::SCALE_CAGE_HANDLES);
    else
        assert(false && "Invalid state!");
}

void SceneViewer::clear_reshaping_operation() {
    m_reshaping_editor->clear();
}

void SceneViewer::screen_size_changed(int width, int height) {
    m_screen_size << width, height;

    m_rebuild_screen_fbo = true;
    m_rebuild_snapshot_fbo = true;

    m_cam->set_win_size(glm::ivec2(width, height));
    prograte_camera_change();
}

void SceneViewer::setup_screen_fbo() {
    using STATUS = ca_essentials::renderer::Framebuffer::STATUS;

    glm::vec2 fbo_size = { ImGui::GetContentRegionAvail().x,
                           ImGui::GetContentRegionAvail().y };

    auto status = m_screen_fbo.setup((int32_t) fbo_size.x, (int32_t) fbo_size.y, m_screen_msaa);
    if(status != STATUS::FRAMEBUFFER_COMPLETE)
        LOGGER.error(" Screen fbo could not be created! FBO is not complete");

    m_rebuild_screen_fbo = false;
}

void SceneViewer::setup_snapshot_fbo() {
    using STATUS = ca_essentials::renderer::Framebuffer::STATUS;

    Eigen::Vector2i fbo_size = get_snapshot_fbo_size();

    auto status = m_snapshot_fbo.setup(fbo_size.x(), fbo_size.y(), m_snapshot_msaa);
    if(status != STATUS::FRAMEBUFFER_COMPLETE)
        LOGGER.error("Snapshot framebuffer could not be created! FBO is not complete");

    m_rebuild_snapshot_fbo = false;
}

void SceneViewer::save_snapshot() {
    namespace core = ca_essentials::core;

    int w = m_snapshot_fbo.width();
    int h = m_snapshot_fbo.height();
    int comps = m_snapshot_fbo.num_comps();

    Eigen::Vector4i frame = get_crop_frame();
    frame *= m_snapshot_size_mult;

    int x0 = frame(0);
    int y0 = frame(1);
    int x1 = frame(2);
    int y1 = frame(3);
    int new_w = x1 - x0;
    int new_h = y1 - y0;

    std::vector<unsigned char> pixels = m_snapshot_fbo.get_color_buffer();
    std::vector<unsigned char> cropped_pixels = core::crop_image(pixels, w, h, comps,
                                                                 x0, y0, x1, y1); 

    stbi_flip_vertically_on_write(true);
    int saved = stbi_write_png(m_snapshot_fn.c_str(), new_w, new_h, comps,
                               cropped_pixels.data(), 0);

    if(saved)
        LOGGER.info("Snapshot successfully saved to {}", m_snapshot_fn);
    else
        LOGGER.error("Error while saving snapshot to {}", m_snapshot_fn);

    // Clearing snapshot info
    m_snapshot_enabled = false;
    m_snapshot_fn.clear();
}

void SceneViewer::process_mouse_event(const MouseEventInfo& info) {
    if(m_mode == Mode::VERT_SELECTION   || m_mode == Mode::HANDLE_SELECTION ||
       m_mode == Mode::ANCHOR_SELECTION || m_mode == Mode::CLEAR_SELECTION) {
        m_selection_handler->on_mouse_event(info);
    }
    else {
        if(m_mode == Mode::TRANSLATE)
            m_selection_handler->clear_selection();
    }
}

Eigen::Vector2i SceneViewer::get_snapshot_fbo_size() const {
    return m_screen_size * m_snapshot_size_mult;
}

Eigen::Vector4i SceneViewer::get_crop_frame() const {
    return compute_crop_frame(m_screen_size);
}

#include "reshaping_editor.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <vector>

namespace {
    ImGuizmo::OPERATION get_gizmo_operation(ReshapingEditor::Mode mode) {
        if(mode == ReshapingEditor::Mode::TRANSLATE_HANDLES)
            return ImGuizmo::TRANSLATE;
        else if(mode == ReshapingEditor::Mode::ROTATE_HANDLES)
            return ImGuizmo::ROTATE;
        else if(mode == ReshapingEditor::Mode::SCALE_HANDLES)
            return ImGuizmo::SCALE;
        else if(mode == ReshapingEditor::Mode::TRANSFORM_HANDLES)
            return ImGuizmo::UNIVERSAL;
        else if(mode == ReshapingEditor::Mode::SCALE_CAGE_HANDLES)
            return ImGuizmo::SCALE;
        else 
            return ImGuizmo::UNIVERSAL;
    }

    // TODO: refactor this function
    bool use_gizmo(float* view, float* proj, float* model, ImGuizmo::OPERATION op) {
        ImGuiIO& io = ImGui::GetIO();

        ImGuizmo::SetDrawlist();
        float win_w = (float) ImGui::GetWindowWidth();
        float win_h = (float) ImGui::GetWindowHeight();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, win_w, win_h);

        bool changed = ImGuizmo::Manipulate(view, proj, op, ImGuizmo::LOCAL, model);
        //ImGuizmo::ViewManipulate(cameraView, camDistance, ImVec2(viewManipulateRight - 128, viewManipulateTop), ImVec2(128, 128), 0x10101010);

        return changed;
    }
}

ReshapingEditor::ReshapingEditor(SelectionHandler& sel_handler)
: m_sel_handler(sel_handler) {
    setup_renderer();
}

void ReshapingEditor::set_mode(Mode mode) {
    m_mode = mode;
    m_gizmo_mode = get_gizmo_operation(m_mode);
}

ReshapingEditor::Mode ReshapingEditor::get_mode() const {
    return m_mode;
}

void ReshapingEditor::set_mesh(const TriMesh& trimesh) {
    m_trimesh = &trimesh;

    double avg_bbox_ext = m_trimesh->get_bbox().sizes().sum() / 3.0;
    m_edit_renderer->set_min_bbox_ext((float) avg_bbox_ext);
}

void ReshapingEditor::set_edit(const std::unordered_set<int>& fixed_points,
                               const std::vector<std::pair<int, Eigen::Vector3d>>& disps) {

    clear();

    m_fixed_points = fixed_points;
    for(const auto& disp : disps) {
        int vid = disp.first;

        const Eigen::Vector3d& p0 = m_trimesh->get_vertices().row(vid);
        const Eigen::Vector3d& p1 = disp.second;

        m_handles.emplace_back(vid, p0, p1);
    }

    update_edit_renderer();
    update_model_matrix();
}

void ReshapingEditor::set_matrices(const glm::mat4& model,
                                   const glm::mat4& view,
                                   const glm::mat4& proj,
                                   const glm::vec3& cam_pos,
                                   const glm::vec3& center) {
    m_model_mat = model;
    m_view_mat = view;
    m_proj_mat = proj;
    m_cam_pos = cam_pos;

    m_edit_renderer->set_matrices(m_model_mat, m_view_mat, m_proj_mat, m_cam_pos, center);
}

bool ReshapingEditor::is_gizmo_active() const {
    return ImGuizmo::IsUsing();
}

void ReshapingEditor::render_reshaping_edit() {
    update_reshaping_op();
    m_edit_renderer->render();
}

void ReshapingEditor::render_gizmo() {
    using MODE = ReshapingEditor::Mode;

    if(get_mode() != MODE::TRANSLATE_HANDLES &&
       get_mode() != MODE::ROTATE_HANDLES &&
       get_mode() != MODE::SCALE_HANDLES &&
       get_mode() != MODE::TRANSFORM_HANDLES &&
       get_mode() != MODE::SCALE_CAGE_HANDLES)
        return;

    bool scale_cage_on = m_mode == Mode::SCALE_CAGE_HANDLES;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    if(use_gizmo(glm::value_ptr(m_view_mat), glm::value_ptr(m_proj_mat),
                 glm::value_ptr(m_gizmo_model_mat), m_gizmo_mode)) {
        update_displaced_points();
        update_edit_renderer();
    }

    if(m_gizmo_on != ImGuizmo::IsUsing()) {
        m_gizmo_on = ImGuizmo::IsUsing();
        gizmo_manipulate_finished();
    }
}

const std::unordered_set<int>& ReshapingEditor::fixed_points() const {
    return m_fixed_points;
}

const std::vector<ReshapingEditor::Handle>& ReshapingEditor::handles() const {
    return m_handles;
}

void ReshapingEditor::clear() {
    m_fixed_points.clear();
    m_handles.clear();

    update_edit_renderer();
    update_model_matrix();
}

void ReshapingEditor::erase_fixed_points() {
    m_fixed_points.clear();
    update_edit_renderer();
    update_model_matrix();
}

void ReshapingEditor::erase_handles() {
    m_handles.clear();
    update_edit_renderer();
    update_model_matrix();
}

void ReshapingEditor::reset_handle_displacements() {
    for(auto& handle : m_handles) {
        handle.new_pos = handle.orig_pos;
        handle.last_new_pos = handle.orig_pos;
    }
    update_edit_renderer();
    update_model_matrix();
}

void ReshapingEditor::enable_displacement_rendering(bool val) {
    m_edit_renderer->enable_displacement_render(val);
}

bool ReshapingEditor::is_displacement_rendering_enabled() const {
    return m_edit_renderer->is_displacement_render_enabled();
}

void ReshapingEditor::setup_renderer() {
    m_edit_renderer  = std::make_unique<EditRenderer>();
    m_edit_renderer->set_points_radius(globals::render::edit::sphere_radius);
    m_edit_renderer->enable_displacement_render(true);
}

void ReshapingEditor::update_reshaping_op() {
    using MODE = ReshapingEditor::Mode;

    if(get_mode() != MODE::FIXED_POINTS_SELECTION &&
       get_mode() != MODE::HANDLES_SELECTION &&
       get_mode() != MODE::ERASE_SELECTION)
        return;

    const auto& selection = m_sel_handler.get_selection();
    if(selection.empty())
        return;

    bool changed = false;
    if(get_mode() == MODE::FIXED_POINTS_SELECTION) {
        m_fixed_points.insert(selection.begin(), selection.end());
        changed = true;
    }
    else if(get_mode() == MODE::HANDLES_SELECTION) {
        for(int vid : selection) {
            const Eigen::Vector3d& orig_v = m_trimesh->get_vertices().row(vid);
            m_handles.emplace_back(vid, orig_v, orig_v);
        }

        changed = true;
    }
    else {
        // mode == ERASE_SELECTION
        int num_fixed_before = (int) m_fixed_points.size();
        int num_handles_before = (int) m_handles.size();

        for(int vid : selection)
            m_fixed_points.erase(vid);

        for(int vid : selection) {
            int local_id = -1;

            for(int i = 0; i < m_handles.size(); ++i)
                if(m_handles.at(i).vid == vid) {
                    local_id = i;
                    break;
                }

            if(local_id >= 0)
                m_handles.erase(m_handles.begin() + local_id);
        }

        changed = num_fixed_before != (int) m_fixed_points.size() ||
                  num_handles_before != (int) m_handles.size();
    }

    if(changed) {
        update_edit_renderer();
        update_model_matrix();
    }

    m_sel_handler.clear_selection();
}

void ReshapingEditor::update_edit_renderer() {
    std::vector<glm::fvec3> fixed_points;
    fixed_points.reserve(m_fixed_points.size());
    for(int vid : m_fixed_points) {
        const Eigen::Vector3d& orig_v = m_trimesh->get_vertices().row(vid);
        fixed_points.emplace_back(orig_v.x(), orig_v.y(), orig_v.z());
    }

    std::vector<std::pair<glm::fvec3, glm::fvec3>> disps;
    disps.reserve(m_handles.size());
    for(const auto& handle : m_handles)
        disps.push_back({{handle.orig_pos.x(), handle.orig_pos.y(), handle.orig_pos.z()},
                         {handle.new_pos.x() , handle.new_pos.y() , handle.new_pos.z()}});


    m_edit_renderer->set_edit(fixed_points, disps);
}

void ReshapingEditor::update_model_matrix() {
    glm::vec3 center = glm::vec3(0.0, 0.0, 0.0);
    for(const auto& handle : m_handles) {
        const Eigen::Vector3d& p = handle.new_pos;
        center += glm::vec3(p.x(), p.y(), p.z());
    }
    center /= m_handles.size();
    
    m_gizmo_model_init_mat = glm::mat4(1.0);
    m_gizmo_model_init_mat = glm::translate(m_gizmo_model_init_mat, center);
    m_gizmo_model_mat = m_gizmo_model_init_mat;
}

void ReshapingEditor::update_displaced_points() {
    glm::mat4 init_inv = glm::inverse(m_gizmo_model_init_mat);
    for(Handle& handle : m_handles) {
        glm::vec4 new_pos = glm::vec4(handle.last_new_pos.x(), handle.last_new_pos.y(), handle.last_new_pos.z(), 1.0);
        new_pos =  m_gizmo_model_mat * init_inv * new_pos;
        handle.new_pos << new_pos.x, new_pos.y, new_pos.z;
    }
}

void ReshapingEditor::gizmo_manipulate_finished() {
    // Setting last_new_pos as current new_pos
    for(Handle& handle : m_handles)
        handle.last_new_pos = handle.new_pos;

    update_model_matrix();
}

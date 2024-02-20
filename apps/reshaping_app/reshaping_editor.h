#pragma once

#include "globals.h"
#include "edit_renderer.h"
#include "selection_handler.h"
#include "ImGuizmo.h"

#include <ca_essentials/meshes/trimesh.h>

#include <unordered_set>
#include <memory>

class ReshapingEditor {
public:
    using TriMesh = ca_essentials::meshes::TriMesh;

public:
    enum class Mode {
        NONE,
        FIXED_POINTS_SELECTION,
        HANDLES_SELECTION,
        ERASE_SELECTION,
        TRANSLATE_HANDLES,
        SCALE_HANDLES,
        ROTATE_HANDLES,
        TRANSFORM_HANDLES,
        SCALE_CAGE_HANDLES
    };

    struct Handle {
        Handle(int id, const Eigen::Vector3d& op, const Eigen::Vector3d& np)
        : vid(id), orig_pos(op), last_new_pos(np), new_pos(np) {}

        int vid = 0;
        Eigen::Vector3d orig_pos;
        Eigen::Vector3d last_new_pos; // used as reference for active Gizmo manipulation
        Eigen::Vector3d new_pos;
    };

public:
    ReshapingEditor(SelectionHandler& sel_handler);

    void set_mode(Mode mode);
    Mode get_mode() const;

    void set_mesh(const TriMesh& trimesh);
    void set_edit(const std::unordered_set<int>& fixed_points,
                  const std::vector<std::pair<int, Eigen::Vector3d>>& disps);

    void set_matrices(const glm::mat4& model,
                      const glm::mat4& view,
                      const glm::mat4& proj,
                      const glm::vec3& cam_pos,
                      const glm::vec3& center);

    bool is_gizmo_active() const;

    void render_reshaping_edit();
    void render_gizmo();

    const std::unordered_set<int>& fixed_points() const;
    const std::vector<Handle>& handles() const;

    void clear();

    void erase_fixed_points();
    void erase_handles();
    void reset_handle_displacements();

    void enable_displacement_rendering(bool val);
    bool is_displacement_rendering_enabled() const;

private:
    void setup_renderer();
    void update_reshaping_op();
    void update_edit_renderer();
    void update_model_matrix();
    void update_displaced_points();
    void gizmo_manipulate_finished();

private:
    Mode m_mode = Mode::NONE;
    std::unordered_set<int> m_fixed_points; 
    std::vector<Handle> m_handles;

    // Selection handler
    SelectionHandler& m_sel_handler;

    // Mesh info
    const TriMesh* m_trimesh = nullptr;

    // Renderer
    std::unique_ptr<EditRenderer> m_edit_renderer;

    // Matrices
    glm::mat4 m_model_mat;
    glm::mat4 m_view_mat;
    glm::mat4 m_proj_mat;
    glm::mat4 m_modelview_mat;
    glm::mat3 m_normal_mat;
    glm::vec3 m_cam_pos;

    // Gizmo
    glm::mat4 m_gizmo_model_mat;
    glm::mat4 m_gizmo_model_init_mat;
    ImGuizmo::OPERATION m_gizmo_mode;
    bool m_gizmo_on = false;
};

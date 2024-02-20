#pragma once

#include <ca_essentials/meshes/trimesh.h>
#include <Eigen/Core>

#include <functional>
#include <unordered_set>

struct MouseEventInfo;

enum class SelectionType {
    VERTEX,
    FACES,
    NONE
};

class SelectionHandler {
public:
    using TriMesh = ca_essentials::meshes::TriMesh;

public:
    SelectionHandler() {};

    void on_mouse_event(const MouseEventInfo& info);

    void set_mesh(const TriMesh& mesh);

    void set_camera_info(const Eigen::Matrix4f& model,
                         const Eigen::Matrix4f& view,
                         const Eigen::Matrix4f& proj,
                         const Eigen::Vector4f& viewport);

    void set_type(SelectionType t);
    SelectionType get_type() const;

    void enable_hover_selection(bool val);
    bool is_hover_selection_enabled() const;

    const std::vector<int>& get_selection() const;
    void clear_selection();

    const std::pair<bool, int>& get_hover_selected_entity() const;

    void undo();

private:
    SelectionHandler(const SelectionHandler&) = delete;
    SelectionHandler(const SelectionHandler&&) = delete;
    void operator=(const SelectionHandler&) = delete;

    void compute_adjacency_list();

    bool is_active() const;
    void new_vertex_selected(int id, bool ctrl_on, bool shift_mod);

    struct PickRes {
        bool hit = false;
        int fid = -1;
        int closest_vid = -1;
        Eigen::Vector3f bc;
        Eigen::Vector3d p; 
    };
    PickRes process_pick(const Eigen::Vector2f& p);

    std::vector<int> compute_path_to_new_point(int new_vid);

private:
    SelectionType m_type = SelectionType::NONE;
    std::vector<int> m_selected_entities;
    std::vector<int> m_prev_selected_entities;

    bool m_hover_selection_on = false;
    std::pair<bool, int> m_hover_selected_entity = {false, -1};

    std::vector<std::vector<int>> m_adj_v2v;
    std::vector<int> m_last_selected_path;

    bool m_mouse_clicked = false;
    Eigen::Vector2f m_last_mouse_click;

    const TriMesh* m_mesh = nullptr;

    Eigen::Matrix4f m_model;
    Eigen::Matrix4f m_view;
    Eigen::Matrix4f m_proj;
    Eigen::Vector4f m_viewport;
};

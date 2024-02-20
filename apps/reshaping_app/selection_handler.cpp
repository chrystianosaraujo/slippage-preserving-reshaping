#include "selection_handler.h"
#include "input_event_notifier.h"

#include <igl/unproject_onto_mesh.h>
#include <igl/adjacency_list.h>
#include <igl/dijkstra.h>

namespace {
}

void SelectionHandler::on_mouse_event(const MouseEventInfo& info)  {
    if(!is_active())
        return;

    // Check if mouse has released without any drag. No drag is considered if either:
    //    - not enough movement
    //    - quickly released
    bool release_and_no_drag = false;
    double diff = 0.0;
    if(m_mouse_clicked && info.released) {
        diff = (m_last_mouse_click - info.pos).squaredNorm();
        release_and_no_drag = info.down_time_last < 0.04f || diff <= 2.0;
    }

    if(!m_mouse_clicked && info.clicked) {
        // Registering new click
        m_mouse_clicked = true;
        m_last_mouse_click = info.pos;
    }
    else if(m_mouse_clicked && (info.released || !info.down)) {
        // Reseting mouse click state in case
        //   1 - mouse released
        //   2 - mouse went out of the window (!down)
        m_mouse_clicked = false;
    }

    bool hover_on = is_hover_selection_enabled() && !m_mouse_clicked && !info.released;
    bool clicked_on = release_and_no_drag;
    if(hover_on || clicked_on) {
        m_hover_selected_entity = {false, -1};

        PickRes res = process_pick(info.pos);
        if(!res.hit)
            return;

        int id = get_type() == SelectionType::VERTEX ? res.closest_vid
                                                     : res.fid;
        if(info.released) {
            new_vertex_selected(id, info.ctrl_mod, info.shift_mod);
        }
        else
            m_hover_selected_entity = { true, id };
    }
}

void SelectionHandler::set_mesh(const ca_essentials::meshes::TriMesh& mesh) {
    m_mesh = &mesh;
    compute_adjacency_list();
    clear_selection();
}

void SelectionHandler::set_camera_info(const Eigen::Matrix4f& model,
                                       const Eigen::Matrix4f& view,
                                       const Eigen::Matrix4f& proj,
                                       const Eigen::Vector4f& viewport) {
    m_model = model;
    m_view = view;
    m_proj = proj;
    m_viewport = viewport;
}

void SelectionHandler::set_type(SelectionType t) {
    m_type = t;
}

SelectionType SelectionHandler::get_type() const {
    return m_type;
}

void SelectionHandler::enable_hover_selection(bool val) {
    m_hover_selection_on = val;
}

bool SelectionHandler::is_hover_selection_enabled() const {
    return m_hover_selection_on;
}

const std::vector<int>& SelectionHandler::get_selection() const {
    return SelectionHandler::m_selected_entities;
}

void SelectionHandler::clear_selection() {
    SelectionHandler::m_selected_entities.clear();
    SelectionHandler::m_hover_selected_entity = {false, -1};
}

const std::pair<bool, int>&
SelectionHandler::get_hover_selected_entity() const {
    return m_hover_selected_entity;
}

void SelectionHandler::undo() {
    m_selected_entities = m_prev_selected_entities;
}

void SelectionHandler::compute_adjacency_list() {
    igl::adjacency_list(m_mesh->get_facets(), m_adj_v2v);
}

bool SelectionHandler::is_active() const {
    return m_mesh && get_type() != SelectionType::NONE;
}

void SelectionHandler::new_vertex_selected(int id, bool ctrl_on, bool shift_on) {
    m_prev_selected_entities = m_selected_entities;

    if(!ctrl_on && !shift_on)
        m_selected_entities.clear();

    auto itr = std::find(m_selected_entities.begin(), m_selected_entities.end(), id);
    if(itr != m_selected_entities.end()) {
        m_selected_entities.erase(itr);
        return;
    }
    else {
        // Computing selected path if control pressed and number of selected vertices = 2
        if(ctrl_on && m_selected_entities.size() > 0) {
            std::vector<int> path = compute_path_to_new_point(id);

            for(int i = 1; i < path.size(); ++i)
                m_selected_entities.push_back(path.at(i));
        }
        else {
            m_last_selected_path.clear();
            m_selected_entities.push_back(id);
        }
    }
}

SelectionHandler::PickRes 
SelectionHandler::process_pick(const Eigen::Vector2f& p) {
    using PickRes = SelectionHandler::PickRes;

    const Eigen::MatrixXd& V = m_mesh->get_vertices();
    const Eigen::MatrixXi& F = m_mesh->get_facets();

    PickRes res;
    res.hit = igl::unproject_onto_mesh(p, m_view, m_proj,
                                       m_viewport, V, F, res.fid, res.bc);
    if(res.hit) {
        const Eigen::Vector3d& v0 = V.row(F(res.fid, 0));
        const Eigen::Vector3d& v1 = V.row(F(res.fid, 1));
        const Eigen::Vector3d& v2 = V.row(F(res.fid, 2));
        res.p = v0 * res.bc.x() + v1 * res.bc.y() + v2 * res.bc.z();

        double closest_dist = DBL_MAX;
        for(int i = 0; i < 3; ++i) {
            const Eigen::Vector3d& v = V.row(F(res.fid, i));
            double dist = (v - res.p).squaredNorm();
            if(dist < closest_dist) {
                closest_dist = dist;
                res.closest_vid = F(res.fid, i);
            }
        }
    }

    return res;
}

std::vector<int> SelectionHandler::compute_path_to_new_point(int new_vid) {
    const auto& sel = get_selection();

    int source = m_selected_entities.back();
    int dest = new_vid;

    Eigen::VectorXd previous;
    Eigen::VectorXd min_dist;
    igl::dijkstra(m_mesh->get_vertices(), m_adj_v2v, source, { dest }, min_dist, previous);

    std::vector<int> path;
    igl::dijkstra(dest, previous, path);

    std::reverse(path.begin(), path.end());

    return path;
}

#include "application.h"

#include "camera_serialization.h"
#include "debug_renderer.h"

#include <mesh_reshaping/reshaping_tool.h>
#include <mesh_reshaping/reshaping_tool_io.h>
#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/precompute_reshaping_data.h>
#include <mesh_reshaping/edit_operation.h>
#include <mesh_reshaping/face_principal_curvatures_io.h>
#include <mesh_reshaping/data_filenames.h>

#include <ca_essentials/core/logger.h>
#include <ca_essentials/ui/imgui_utils.h>
#include <ca_essentials/ui/imgui_font_provider.h>
#include <ca_essentials/meshes/load_trimesh.h>
#include <ca_essentials/meshes/save_trimesh.h>
#include <ca_essentials/meshes/debug_utils.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <string>
#include <cassert>
#include <cstdlib>

namespace {

void ImGui_SectionText(const std::string& txt) {
    ImGui::Text((std::string("- ") + txt).c_str());
}
}

Application::Application(const std::string& output_dir,
                         const std::string& temp_dir)
: m_output_dir(output_dir)
, m_temp_dir(temp_dir) {

}
        
Application::~Application() {

}

bool Application::init(int width, int height, std::string init_fn) {
    setup_window(width, height);
    setup_ui_style();
    setup_viewer(width, height);

    if(!init_fn.empty())
        load_model(init_fn);

    return true;
}

void Application::run() {
    m_window->main_loop([this]() {
        main_loop();

        // Make sure a single frame is drawn on the default FBO
        // so we can see what is being generated when executing
        // in batch mode
        if(this->m_close_after_screenshot)
            if(m_frame_idx == 0)
                save_screenshot();
            else if(m_frame_idx > 1)
                m_window->finish_loop();

        if(!this->m_window->is_screenshot_mode_on())
            this->m_frame_idx++;

        handle_key_events();
    });
}

void Application::run_for_snapshot_only(const std::string& fn,
                                        int screen_mult,
                                        int msaa) {
    m_frame_idx = 0;
    m_screenshot_fn = fn;
    m_close_after_screenshot = true;

    m_viewer->set_screen_msaa(msaa);
    m_viewer->set_snapshot_size_mult(screen_mult);

    run();
}

void Application::load_model(const std::string& model_fn,
                             bool normalize) {
    namespace meshes = ca_essentials::meshes;

    DebugRenderer::clear_all();

    m_mesh_fn = model_fn;

    auto tri_mesh = meshes::load_trimesh(model_fn, normalize);
    if(!tri_mesh) {
        LOGGER.error("Could not load model {}", model_fn);
        return;
    }

    set_model(std::move(tri_mesh));
    
    // Automatically loads the following extra information:
    //   - Edit operations
    //   - Cameras
    //   - Straightness information
    //   - Pre-computed principal curvature values
    load_edit_operations();
    load_cameras();
    load_straightness_info();
    load_curvature_info();

    m_viewer->set_model(*m_mesh);
    m_viewer->set_straight_chains(m_straight_info.get());
}

void Application::load_output_solution(const std::string& fn) {
    m_load_output_fn = fn;
}

void Application::load_camera(const std::string& cam_fn, const std::string& label) {
    namespace fs = std::filesystem;

    if(!fs::exists(cam_fn)) {
        LOGGER.warn("Could not find camera file {}", cam_fn);
        return;
    }

    const auto [succ, cam_info] = deserialize_camera(cam_fn, label);
    if(!succ) {
        LOGGER.error("Error while loading camera \"{}\" from file {}", label, cam_fn);
        return;
    }

    m_viewer->set_camera_info(cam_info);
    LOGGER.info("Camera \"{}\" succesfully loaded from {}", label, cam_fn);
}

void Application::save_screenshot() {
    m_viewer->save_screenshot(m_screenshot_fn);
}

void Application::close() {
    m_window->close();
}

void Application::set_model(std::unique_ptr<reshaping::TriMesh>& mesh) {
    m_mesh = std::move(mesh);
    m_orig_V = m_mesh->get_vertices();
}

void Application::reset_model_geometry() {
    Eigen::MatrixXd V;
    m_mesh->export_vertices(V);
    V = m_orig_V;
    m_mesh->import_vertices(V);

    m_viewer->model_geometry_updated();
}

std::string Application::get_model_name() const {
    namespace fs = std::filesystem;

    return fs::path(m_mesh_fn).stem().string();
}

std::string Application::get_active_edit_operation_label() const {
    const auto* op = get_active_edit_operation();
    if(op)
        return op->label;
    else
        return "";
}

const reshaping::EditOperation* Application::get_active_edit_operation() const {
    if(m_selected_edit_idx >= 0)
        return &m_edit_ops.at(m_selected_edit_idx);
    else
        return nullptr;
}

void Application::save_current_edit_operation(const std::string& label) {
    namespace fs = std::filesystem;

    auto new_edit = m_viewer->get_current_reshaping_edit();

    // If there is an edit operation with the given label, update it.
    // Otherwise, create a new one.
    bool updated = false;
    for(auto& edit : m_edit_ops) {
        if(edit.label == label) {
            edit.displacements = new_edit.displacements;
            updated = true;
            break;
        }
    }

    if(!updated) {
        new_edit.label = label;
        m_edit_ops.push_back(new_edit);
    }

    activate_edit_operation(label);

    // Updates the edit operations file
    fs::path fn = reshaping::get_edit_operation_fn(m_mesh_fn);
    bool succ = reshaping::write_edit_operations_to_json(m_edit_ops, fn.string());
    if(succ) {
        LOGGER.info("Edit operation \"{}\" saved to {}", label, fn);
        load_edit_operations();
    }
    else
        LOGGER.info("Error while saving edit operation \"{}\" to {}", label, fn);
}

void Application::clear_current_edit_operation() {
    m_viewer->clear_reshaping_editor();

    // empty label means deactivate all
    activate_edit_operation("");
}

void Application::load_edit_operations() {
    namespace fs = std::filesystem;

    std::string edit_op_fn = reshaping::get_edit_operation_fn(m_mesh_fn);
    if(!fs::exists(edit_op_fn)) {
        LOGGER.warn("Could not find edit operation file {}", edit_op_fn);
        return;
    }

    std::vector<reshaping::EditOperation> edit_ops;
    bool succ = reshaping::load_edit_operations_from_json(edit_op_fn, edit_ops);
    if(!succ) {
        LOGGER.error("Error while loading edit operations file {}", edit_op_fn);
        return;
    }

    m_edit_ops = edit_ops;
    m_selected_edit_idx = -1;
}

void Application::delete_edit_operation(const std::string& label) {
    std::string reshaping_fn = reshaping::get_edit_operation_fn(m_mesh_fn);

    reshaping::delete_edit_operation_from_json(reshaping_fn, label);
    load_edit_operations();
}

void Application::save_camera_to_file(const std::string& label) {
    namespace fs = std::filesystem;

    fs::path fn = reshaping::get_camera_fn(m_mesh_fn);
    bool succ = serialize_camera(label, m_viewer->get_camera_info(), fn);
    if(succ) {
        LOGGER.info("Camera \"{}\" saved to {}", label, fn);
        load_cameras();
    }
    else
        LOGGER.info("Error while saving camera \"{}\" to {}", label, fn);
}

void Application::load_cameras() {
    namespace fs = std::filesystem;

    m_cameras.clear();
    m_camera_labels.clear();

    std::string camera_fn = reshaping::get_camera_fn(m_mesh_fn);
    if(!fs::exists(camera_fn)) {
        LOGGER.warn("Could not find camera file {}", camera_fn);
        return;
    }

    bool succ = deserialize_cameras(camera_fn, m_cameras, m_camera_labels);
    if(!succ)
        LOGGER.error("Error while loading cameras file {}", camera_fn);
}

void Application::delete_camera(const std::string& label) {
    std::string camera_fn = reshaping::get_camera_fn(m_mesh_fn);
    delete_camera_from_file(camera_fn, label);
    load_cameras();
}

void Application::load_straightness_info() {
    namespace fs = std::filesystem;

    m_straight_info = std::make_unique<reshaping::StraightChains>();

    std::string straight_fn = reshaping::get_straightness_fn(m_mesh_fn);
    if(!fs::exists(straight_fn)) {
        LOGGER.warn("Could not find straightness file {}", straight_fn);
        return;
    }

    if(m_straight_info->load_from_file(straight_fn))
        LOGGER.info("Straightness information loaded from {}", straight_fn);
    else
        LOGGER.error("Error while loading straightness information from {}", straight_fn);
}

void Application::load_curvature_info() {
    namespace fs = std::filesystem;

    std::string fn = reshaping::get_curvature_fn(m_mesh_fn);
    if(!fs::exists(fn)) {
        LOGGER.error("Could not find curvature file at {}", fn);
        return;
    }

    if(reshaping::load_face_principal_curvature_values(fn, m_face_k1, m_face_k2))
        LOGGER.info("Face curvature information read from {}", fn);
    else
        LOGGER.error("Error while loading face curvature information from {}", fn);
}

void Application::main_loop() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static bool docking_initialized = false;

    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
    flags |= ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
             ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove     |
             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace Demo", 0, flags);
    ImGui::PopStyleVar();

    ImGuiIO& io = ImGui::GetIO();
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

    ImGuiID dock_left_id;
    if(!docking_initialized) {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
        ImGui::DockBuilderAddNode(dockspace_id);    // Add empty node

        ImGuiID dock_main_id         = dockspace_id; 
        dock_left_id                 = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left,  0.80f, NULL, &dock_main_id);
        ImGuiID dock_right_id        = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, NULL, &dock_main_id);
        ImGuiID dock_right_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down , 0.40f, NULL, &dock_right_id);

        ImGui::DockBuilderDockWindow("Reshaping", dock_right_id);
        ImGui::DockBuilderDockWindow("Scene", dock_right_bottom_id);
        ImGui::DockBuilderDockWindow("SceneViewer", dock_left_id);
        ImGui::DockBuilderFinish(dockspace_id);

        docking_initialized = true;
    }

    ImGui::DockSpace(dockspace_id);

    draw_ui();
    m_viewer->render();

    ImGui::End();
    ImGui::PopStyleVar();
}

void Application::setup_window(int width, int height) {
    using Window = ca_essentials::ui::Window;
    using WindowParams = ca_essentials::ui::WindowParams;

    WindowParams win_params;
    win_params.width  = width;
    win_params.height = height;
    win_params.title  = "3D Reshaping";
    win_params.msaa_on = true;
    win_params.clear_color = Eigen::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
    win_params.open_maximized = true;

    m_window = std::make_unique<Window>(win_params);
}

void Application::setup_ui_style() {
    namespace ui = globals::ui;
    namespace paths = globals::paths;
    namespace fs = std::filesystem;

    ImGuiIO& io = ImGui::GetIO();

    // Setting up default style
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding.x = 4.0f;
        style.WindowPadding.y = 4.0f;
        style.FrameRounding = 5;
        style.GrabRounding = 5;
        style.IndentSpacing = 14;
        style.WindowBorderSize = 0;
        style.ItemSpacing.y = 7;

        ImGui::StyleColorsLight();
    }

    // Setting up fonts
    {
        auto* font = ca_essentials::ui::ImGuiFontProvider::add_font(io,
                                                                    ui::default_font_id,
                                                                    paths::font_fn.string(),
                                                                    ui::font_size);
    
        if(!font)
            LOGGER.error("Error while loading font from {}", paths::font_fn.string());

        io.FontDefault = font;
    }
}

void Application::setup_viewer(int width, int height) {
    m_viewer = std::make_unique<SceneViewer>(width, height);
    assert(m_viewer);

    m_viewer_panel = std::make_unique<SceneViewerPanel>(*m_viewer);
}

void Application::draw_ui() {
    draw_menu_bar();
    draw_edit_panel();
    m_viewer_panel->draw_ui();
}

void Application::draw_menu_bar() {
    if(ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Mesh", "CTRL+O")) {
                open_load_mesh_dialog();
            }
            if (ImGui::MenuItem("Exit")) {
                close();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Application::draw_edit_panel() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_None; 

    static bool open = true;
    ImGui::Begin("Reshaping", &open, ImGuiWindowFlags_NoCollapse);
    {
        ImGui::SetNextItemOpen(true);
        if(ImGui::TreeNode("3D Reshaping")) {
            ImGui::SliderInt("Max Iters.", &m_max_iters, 1, 500);

            if(ImGui::Button("Run Reshaping"))
                perform_reshaping();

            ImGui::TreePop();
        }

        ImGui::Indent();
        draw_reshaping_ops_section();
        draw_camera_section();
        ImGui::Unindent();
    }
    ImGui::End();
}

void Application::draw_serialization_section() {
    ImGui::SetNextItemOpen(true);
    if(ImGui::TreeNode("Serialization")) {
        imgui_utils::add_spacing(globals::ui::section_opening_margin);
        draw_reshaping_ops_section();
        draw_camera_section();

        ImGui::TreePop();
    }
}

void Application::draw_reshaping_ops_section() {
     auto sec_size  = ImVec2(-FLT_MIN, 5.0f * ImGui::GetTextLineHeightWithSpacing());
     auto line_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing());
     auto btn_size  = ImVec2(line_size.y * 0.8f, line_size.y * 0.98f);

     ImGui_SectionText("Load Edit Operation");
     if(ImGui::BeginListBox("##listbox 1", sec_size)) {
         for(int i = 0; i < m_edit_ops.size(); i++) {
             const bool is_selected = m_selected_edit_idx == i;

             if(ImGui::Selectable(m_edit_ops.at(i).label.c_str(),
                                  is_selected,
                                  ImGuiSelectableFlags_AllowItemOverlap,
                                  line_size)) {

                 bool changed = m_selected_edit_idx != i;
                 m_selected_edit_idx = i;

                 activate_edit_operation(m_edit_ops.at(i).label);
             }

             ImGui::SameLine(ImGui::GetWindowWidth() - 1.3f * btn_size.x);
             if(ImGui::Button((std::string("X##reshaping_op") + std::to_string(i)).c_str(), btn_size))
                 delete_edit_operation(m_edit_ops.at(i).label);

             // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
             if(is_selected)
                 ImGui::SetItemDefaultFocus();
         }
         ImGui::EndListBox();
     }
     ImGui_SectionText("Serialize Edit Operation");
     {
        static char label[128] = "";
        ImGui::InputTextWithHint("##edit_label",
                                  "new edit label",
                                  label,
                                  IM_ARRAYSIZE(label));

        ImGui::SameLine();
        if(ImGui::Button("Save##Edit")) {
            save_current_edit_operation(label);
            label[0] = '\0';
        }

        if(ImGui::Button("Clear Edit"))
            clear_current_edit_operation();
     }
}

void Application::draw_camera_section() {
     auto sec_size  = ImVec2(-FLT_MIN, 5.0f * ImGui::GetTextLineHeightWithSpacing());
     auto line_size = ImVec2(0.0, ImGui::GetTextLineHeightWithSpacing());
     auto btn_size  = ImVec2(line_size.y * 0.8f, line_size.y * 0.98f);

     ImGui_SectionText("Load Camera");
     if(ImGui::BeginListBox("##listbox 2", sec_size)) {
         for(int i = 0; i < m_cameras.size(); i++) {
             const bool is_selected = m_selected_cam_idx == i;

             if(ImGui::Selectable(m_camera_labels.at(i).c_str(),
                                  is_selected,
                                  ImGuiSelectableFlags_AllowItemOverlap,
                                  line_size)) {

                 bool changed = m_selected_cam_idx != i;
                 m_selected_cam_idx = i;

                 activate_camera(m_camera_labels.at(i));
             }

             ImGui::SameLine(ImGui::GetWindowWidth() - 1.3f * btn_size.x);
             if(ImGui::Button((std::string("X##cam") + std::to_string(i)).c_str(), btn_size))
                 delete_camera(m_camera_labels.at(i));

             // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
             if(is_selected)
                 ImGui::SetItemDefaultFocus();
         }
         ImGui::EndListBox();
     }

     ImGui_SectionText("Serialize Camera");
     {
        static char label[128] = "";
        ImGui::InputTextWithHint("##cam_label",
                                 "new camera label",
                                 label,
                                 IM_ARRAYSIZE(label));

        ImGui::SameLine();
        if(ImGui::Button("Save##Camera")) {
            save_camera_to_file(label);
            label[0] = '\0';
        }
     }
}

void Application::handle_key_events() {
    // Automatically saves a temporary edit operation when 's' is pressed
    if(ImGui::IsKeyPressed(ImGuiKey_S)) {
        std::string temp_label = "live";

        save_current_edit_operation(temp_label);
        activate_edit_operation(temp_label);
    }
}

void Application::open_load_mesh_dialog() {
    std::string fn = imgui_utils::open_file_dialog("obj", "OBJ files");
    load_model(fn);
}

void Application::activate_edit_operation(const std::string& label) {
    // Resetting model geometry automatically when a new
    // edit operation is activated
    reset_model_geometry();

    int idx = -1;
    std::unordered_set<int> fixed_verts;
    std::vector<std::pair<int, Eigen::Vector3d>> disps;

    if(!label.empty()) {
        // Finding the index of the edit operation with the given label
        for(int i = 0; i < m_edit_ops.size(); ++i) {
            const auto& op = m_edit_ops.at(i);

            if(op.label == label) {
                idx = i;
                break;
            }
        }

        if(idx < 0) {
            LOGGER.error("Could not activate edit operation \"{}\"", label);
            return;
        }
        else
            LOGGER.debug("Activating edit operation \"{}\"", label);

        const double diag_len = m_mesh->get_bbox().diagonal().norm();

        // Computing fixed and displaced vertices
        const auto& op = m_edit_ops.at(idx);
        for(const auto& [vid, trans] : op.displacements) {
            bool is_fixed = trans.norm() < 1e-6;

            if(is_fixed)
                fixed_verts.insert(vid);
            else {
                const Eigen::Vector3d& orig_p = m_mesh->get_vertices().row(vid);
                disps.emplace_back(vid, orig_p + trans * diag_len);
            }
        }

    }

    m_selected_edit_idx = idx;
    
    // Updating viewer with the new edit operation
    m_viewer->set_reshaping_operation(fixed_verts, disps);
    enable_displacement_render(true);
}

void Application::activate_camera(const std::string& label) {
    // Finding index of the camera with the given label
    int idx = -1;
    for(int i = 0; i < m_camera_labels.size(); ++i) {
        if(m_camera_labels.at(i) == label) {
            idx = i;
            break;
        }
    }

    if(idx < 0) {
        LOGGER.error("Could not activate camera \"{}\"", label);
        return;
    }
    else 
        LOGGER.debug("Activating camera \"{}\"", label);

    m_selected_cam_idx = idx;

    // Updating viewer with the loaded camera
    m_viewer->set_camera_info(m_cameras.at(idx));
}

void Application::enable_transparent_mode(bool val) {
    m_viewer->enable_transparent_mode(val);
}

void Application::enable_handle_error_distribution(bool val) {
    m_handle_error_distrib_on = val;
}

void Application::perform_reshaping() {
    reset_model_geometry();

    if(m_selected_edit_idx < 0) {

        LOGGER.warn("Reshaping was not performed."
                    " There is no active reshaping operation.");
        return;
    }

    // Setting up reshaping parameters
    reshaping::ReshapingParams params;
    params.debug_folder = m_temp_dir.string();
    params.input_name   = get_input_name();
    params.max_iters    = m_max_iters;
    params.handle_error_distrib_enabled = m_handle_error_distrib_on;

    if(globals::io::export_detailed_opt_info)
        params.export_objs_every_n_iter = 1;
    else
        params.export_objs_every_n_iter = 20;

    // Precomputing reshaping data
    auto data = reshaping::precompute_reshaping_data(
        params,
        *m_mesh.get(),
        m_face_k1,
        m_face_k2,
        m_straight_info.get()
    );

    // Defining boundary conditions
     const double diag_len = m_mesh->get_bbox().diagonal().norm();

    const auto& op = m_edit_ops.at(m_selected_edit_idx);
    for(const auto& [vid, disp] : op.displacements) {
        const Eigen::Vector3d& orig_pos  = m_mesh->get_vertices().row(vid);
        const Eigen::Vector3d target_pos = reshaping::displacement_to_abs_position(orig_pos,
                                                                                   disp,
                                                                                   diag_len);
        data->bc.insert({ vid, target_pos });
    }

    LOGGER.debug("Starting Reshaping Tool");
    Eigen::MatrixXd newV = reshaping::reshaping_solve(params, *data);

    Eigen::MatrixXd V;
    m_mesh->export_vertices(V);
    V = newV;
    m_mesh->import_vertices(V);

    save_optimization_inputs(*data, params);
    save_optimization_outputs(*data, params);

    m_viewer->model_geometry_updated();

    m_viewer->reset_mode();
    enable_displacement_render(false);
}

void Application::set_max_iters(int max_iters) {
    m_max_iters = max_iters;
}

void Application::enable_edit_operation_render(bool val) {
    m_viewer->enable_edit_operation_rendering(val);
}

void Application::enable_wireframe(bool val) {
    m_viewer->enable_wireframe_rendering(val);
}

void Application::enable_displacement_render(bool val) {
    m_viewer->enable_displacement_rendering(val);
}

void Application::enable_straight_render(bool val) {
    m_viewer->enable_straight_render(val);
}

void Application::save_optimization_inputs(const reshaping::ReshapingData& opt_data,
                                           const reshaping::ReshapingParams& params) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

    const auto* active_op = get_active_edit_operation();
    assert(active_op && "Invalid Reshaping Operation");

    std::string input_name = get_input_name();

    // Exporting the input normalized mesh
    if(reshaping::save_mesh(m_orig_V,
                             m_mesh->get_facets(),
                             m_output_dir.string(),
                             input_name,
                             "input"))
        LOGGER.info("Normalized input mesh exported at {}", m_output_dir);
    else
        LOGGER.error("Error while saving normalized input mesh to {}", m_output_dir);

    // Exporting the handles and fixed points
    if(reshaping::save_edit_operation_to_obj(*active_op,
                                              m_orig_V,
                                              m_mesh->get_facets(),
                                              m_output_dir.string(),
                                              input_name))
        LOGGER.info("Input handles and fixed points exported at {}", m_output_dir);
    else
        LOGGER.error("Error while exporting edit operation (handles and fixed points) at {}", m_output_dir);
}

void Application::save_optimization_outputs(const reshaping::ReshapingData& opt_data,
                                            const reshaping::ReshapingParams& params) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

    std::string input_name = get_input_name();

    // Saving output mesh
    if(reshaping::save_mesh(*m_mesh.get(),
                            m_output_dir.string(),
                            input_name,
                            "output"))
        LOGGER.info("Reshaping output mesh exported at {}", m_output_dir.string());
    else
        LOGGER.error("Error while exporting reshaping output mesh at {}", m_output_dir.string());

    // Saving optimization info to json
    if(reshaping::save_optimization_info_to_json(*m_mesh.get(),
                                                 opt_data,
                                                 params,
                                                 m_output_dir.string(),
                                                 input_name))
        LOGGER.info("Optimization info exported at {}", m_output_dir.string());
    else
        LOGGER.error("Error while exporting optimization info at {}", m_output_dir.string());

}

std::string Application::get_input_name() const {
    std::string model_name = get_model_name();
    const auto* active_op = get_active_edit_operation();

    return model_name + "_" + active_op->label;
}

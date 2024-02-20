#pragma once

#include "globals.h"
#include "scene_viewer.h"
#include "scene_viewer_panel.h"

#include <mesh_reshaping/types.h>
#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/edit_operation.h>

#include <ca_essentials/ui/window.h>

#include <filesystem>
#include <memory>

// Main gui-based application
class Application {
public:
    Application(const std::string& output_dir,
                const std::string& temp_dir);
    ~Application();

    // Initialize the gui-based application
    bool init(int width, int height, std::string init_fn="");

    // Run loop
    void run();

    // Execute a single frame, save the snapshot, and close
    void run_for_snapshot_only(const std::string& fn, int screen_mult=2, int msaa=8);

    // Loads an input surface given its filename
    void load_model(const std::string& fn, bool normalize=true);

    // Loads camera information provided its filename and label
    void load_camera(const std::string& cam_fn, const std::string& label);

    // Loads a pre-computed output reshaping solution
    void load_output_solution(const std::string& fn);

    // Activate an edit operation provided its label
    void activate_edit_operation(const std::string& label);

    // Activate a camera provided its labels
    void activate_camera(const std::string& label);

    // Rendering options
    void enable_transparent_mode(bool val);
    void enable_displacement_render(bool val);
    void enable_straight_render(bool val);
    void enable_edit_operation_render(bool val);
    void enable_wireframe(bool val);

    // Reshaping options
    void enable_handle_error_distribution(bool val);
    void set_max_iters(int max_iters);
    void perform_reshaping();

private:
    // Windowing management
    void setup_window(int width, int height);
    void setup_ui_style();
    void setup_viewer(int width, int height);
    void close();

    void open_load_mesh_dialog();

    // Sets the model to be displayed
    void set_model(std::unique_ptr<reshaping::TriMesh>& model);

    // Reset model geometry to its original state
    void reset_model_geometry();

    // Returns current model name
    std::string get_model_name() const;

    // Reloads edit operations from the pre-defined file
    void load_edit_operations();

    // Deletes an edit operation provided its label
    void delete_edit_operation(const std::string& label);

    // Returns the label of the active edit operation
    std::string get_active_edit_operation_label() const;

    // Returns the active edit operation. If no operation is active, returns nullptr
    const reshaping::EditOperation* get_active_edit_operation() const;

    // Saves the current edit operation to the pre-defined file using the provided label
    void save_current_edit_operation(const std::string& label);

    // Clears the current edit operation
    void clear_current_edit_operation();

    // Save camera information to file using the given label
    void save_camera_to_file(const std::string& label);
    
    // Reloads camera information from the pre-defined file
    void load_cameras();

    // Deletes a camera provided its label
    void delete_camera(const std::string& label);

    // Extra information
    void load_straightness_info();
    void load_curvature_info();

    // Rendering 
    void main_loop();

    // UI
    void draw_ui();
    void draw_menu_bar();
    void draw_edit_panel();
    void draw_serialization_section();
    void draw_reshaping_ops_section();
    void draw_camera_section();
    void handle_key_events();

    // Render-to-image at the pre-defined screenshot filename (m_screenshot_fn)
    void save_screenshot();

    // Saving input (normalized) mesh and active edit operation to objs
    void save_optimization_inputs(const reshaping::ReshapingData& opt_data,
                                  const reshaping::ReshapingParams& params);

    // Saving output reshaping solution to obj and optimization info to json.
    void save_optimization_outputs(const reshaping::ReshapingData& opt_data,
                                   const reshaping::ReshapingParams& params);

    // Returns the input name as a combination of the [model-name]_[edit-label]
    std::string get_input_name() const;

private:
    // Directory where output files are saved
    std::filesystem::path m_output_dir;

    // Diretory where temporary files are saved
    std::filesystem::path m_temp_dir;

    std::unique_ptr<ca_essentials::ui::Window> m_window;
    std::unique_ptr<SceneViewer> m_viewer;
    std::unique_ptr<SceneViewerPanel> m_viewer_panel;

    // Input mesh filename
    std::string m_mesh_fn;

    // Current mesh
    std::unique_ptr<ca_essentials::meshes::TriMesh> m_mesh;

    // Original mesh vertices
    Eigen::MatrixXd m_orig_V;

    // List of available edit operations
    std::vector<reshaping::EditOperation> m_edit_ops;

    // List of available cameras and labels
    std::vector<CameraInfo> m_cameras;
    std::vector<std::string> m_camera_labels;

    // Loaded straightness information
    std::unique_ptr<reshaping::StraightChains> m_straight_info;

    // Loaded principal curvature values
    Eigen::VectorXd m_face_k1;
    Eigen::VectorXd m_face_k2;

    // Index of the selected edit operation and camera
    int m_selected_edit_idx = -1;
    int m_selected_cam_idx  = -1;

    // Indicates whether the next frame must be rendered to an image
    bool m_screenshot_on = false;

    // Indicates where the application must be closed after the next frame
    bool m_close_after_screenshot = false;

    // Filename where the next screenshot will be saved
    std::string m_screenshot_fn;

    // Frame counter
    int m_frame_idx = 0;

    // Reshaping options
    int m_max_iters = globals::reshaping::default_max_iters;
    bool m_handle_error_distrib_on = false;

    // Filename where the pre-computed output reshaping solution is saved
    std::string m_load_output_fn;
};

#pragma once

#include "globals.h"
#include "model_renderer.h"
#include "straight_chains_renderer.h"
#include "selection_renderer.h"
#include "camera.h"
#include "input_event_notifier.h"
#include "selection_handler.h"
#include "reshaping_editor.h"

#include <mesh_reshaping/types.h>
#include <mesh_reshaping/edit_operation.h>
#include <mesh_reshaping/straight_chains.h>

#include <ca_essentials/ui/imgui_toolbar.h>
#include <ca_essentials/meshes/trimesh.h>
#include <ca_essentials/renderer/glslprogram.h>
#include <ca_essentials/renderer/framebuffer.h>

#include <lagrange/common.h>
#include <lagrange/mesh.h>

#include <memory>

class SceneViewer {
public:
    // All possible interaction modes
    enum Mode {
        CAMERA_MANIP,
        VERT_SELECTION,
        ANCHOR_SELECTION,
        HANDLE_SELECTION,
        CLEAR_SELECTION,
        TRANSLATE,
        SCALE,
        ROTATE,
        TRANSFORM,
        SCALE_CAGE,

        NUM_MODES = SCALE_CAGE + 1
    };

public:
	SceneViewer(int width, int height);
	~SceneViewer();

    // Sets the mesh to be displayed
    void set_model(const reshaping::TriMesh& mesh);

    // Sets the edit operation to be displayed
    void set_reshaping_operation(const std::unordered_set<int>& fixed_points,
                                 const std::vector<std::pair<int, Eigen::Vector3d>>& displaced_points);

    // Sets straightness chains
    void set_straight_chains(const reshaping::StraightChains* chains);

    // Resets interaction mode to CAMERA_MANIP
    void reset_mode();

    // Notifies that the model geometry has been updated
    void model_geometry_updated();

    // Render scene
    void render();

    // Save screenshot to a png file
    void save_screenshot(const std::string& fn);

    // Camera
    CameraInfo get_camera_info() const;
    void set_camera_info(const CameraInfo& info);
    double get_camera_fov() const;

    // Returns the current edit operation
    reshaping::EditOperation get_current_reshaping_edit() const;
    
    // Clears current edit operation
    void clear_reshaping_editor();

    // Clears all fixed points
    void clear_editor_fixed_points();

    // Clears all handles
    void clear_editor_handles();

    // Resets all handle displacements
    void reset_handle_displacements();

    // Enables/disables transparent mode (model in wirefrme only)
    void enable_transparent_mode(bool val);
    bool is_transparent_mode_enabled() const;

    // Enables/disables edit operation rendering
    void enable_edit_operation_rendering(bool val);
    bool is_edit_operation_rendering_enabled() const;

    // Enables/disables model rendering in solid mode 
    void enable_surface_rendering(bool val);
    bool is_surface_rendering_enabled() const;

    // Enables/disables model rendering in wireframe mode
    void enable_wireframe_rendering(bool val);
    bool is_wireframe_rendering_enabled() const;

    // Enables/disables displacement vector rendering
    void enable_displacement_rendering(bool val);
    bool is_displacement_rendering_enabled() const;

    // Enables/disables straight sections render options
    void enable_straight_render(bool val);
    bool is_straight_render_enabled() const;

    // Enables/disables debugging render options
    void enable_debug_render(bool val);
    bool is_debug_render_enabled() const;

    // Snapshot size multiplier
    void set_snapshot_size_mult(int mult);
    int get_snapshot_size_mult() const;

    // Snapshot msaa rendering level
    void set_snapshot_msaa(int msaa);
    int get_snapshot_msaa() const;

    // Screen msaa rendering level
    void set_screen_msaa(int msaa);
    int get_screen_msaa() const;

    // Enables/disables snapshot crop frame rendering
    void enable_crop_frame_rendering(bool val);
    bool is_crop_frame_rendering_enabled() const;

    // Sets footer message
    void set_footer_message(const std::string& fn);

private:
    void setup_camera();
    void setup_renderers();
    void setup_input_event_notifier();
    void setup_selection_handler();
    void setup_ui_icons();
    void setup_toolbar();
    void setup_reshaping_editor();

    void process_events();

    void render_to_widget();
    void render_to_snapshot();
    void render_scene();
    void render_crop_frame();

    void render_footer_message(const ImVec2& win_size);
    void update_footer_messages();

    void mesh_updated(bool reset_camera=false);
    void recompute_mesh_data();

    void prograte_camera_change();
    void reset_camera();

    void update_active_mode();
    void update_renderers_mesh_data();
    void update_reshaping_editor_mode();
    void clear_reshaping_operation();

    void screen_size_changed(int width, int height);
    void setup_screen_fbo();
    void setup_snapshot_fbo();
    void save_snapshot();

    void process_mouse_event(const MouseEventInfo& info);

    Eigen::Vector4i get_crop_frame() const;
    Eigen::Vector2i get_snapshot_fbo_size() const;

private:
    // Mesh to be displayed
    const reshaping::TriMesh* m_mesh = nullptr;

    // Viewport size
    Eigen::Vector2i m_screen_size = Eigen::Vector2i::Zero();

    std::unique_ptr<ModelRenderer> m_model_renderer;
    std::unique_ptr<SelectionRenderer> m_selection_renderer;
    std::unique_ptr<StraightChainsRenderer> m_chains_renderer;

    // UI 
    std::unique_ptr<ReshapingEditor> m_reshaping_editor;
    std::unique_ptr<ca_essentials::ui::ImGuiToolbar> m_toolbar;

    // Camera
    std::unique_ptr<Camera> m_cam;

    // Selection Handler
    std::unique_ptr<SelectionHandler> m_selection_handler;
    
    // Extra model info
    Eigen::AlignedBox3d m_bbox;

    // Interaction mode
    Mode m_mode = Mode::CAMERA_MANIP;
    std::vector<int> m_per_mode_status;

    bool m_transparent_mode_on = false;
    bool m_frame_rendering_on = false;
    bool m_straight_rendering_on = false;
    bool m_debug_rendering_on = true;
    bool m_crop_frame_on = false;
    bool m_edit_op_rendering_on = true;

    ca_essentials::renderer::Framebuffer m_screen_fbo;
    int m_screen_msaa = 32;
    bool m_rebuild_screen_fbo = false;

    // Snashot info
    ca_essentials::renderer::Framebuffer m_snapshot_fbo;
    std::string m_snapshot_fn;
    int m_snapshot_msaa = 32;
    int m_snapshot_size_mult = 1;
    bool m_snapshot_enabled = false;
    bool m_rebuild_snapshot_fbo = false;

    Eigen::Vector2i m_widget_pos = Eigen::Vector2i(0, 0);

    std::vector<std::string> m_footer_messages;
};

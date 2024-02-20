#include "scene_viewer_panel.h"
#include "globals.h"

#include <ca_essentials/ui/imgui_utils.h>
#include <imgui.h>

#define DEBUG_SECTION_ON 0

SceneViewerPanel::SceneViewerPanel(SceneViewer& viewer)
: m_viewer(viewer) {

}

void SceneViewerPanel::draw_ui() {
    bool open = true;
    ImGui::Begin("Scene", &open, ImGuiWindowFlags_NoCollapse);
    {
        draw_view_mode_section();
        draw_model_section();
        draw_straight_section();
#if DEBUG_SECTION_ON
        draw_debug_section();
#endif
        draw_screen_section();
        draw_snapshot_section();
        draw_reshaping_section();
    }
    ImGui::End();
}

void SceneViewerPanel::draw_view_mode_section() {
    // If the following call is not used:
    // A Text+SameLine+Button sequence will have the text a little too high by default!
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Mode:"); ImGui::SameLine();

    int op = m_viewer.is_transparent_mode_enabled();
    ImGui::RadioButton("Normal", &op, 0); ImGui::SameLine();
    ImGui::RadioButton("Transparent", &op, 1);
    
    bool transparent_mode_on = op == 1;
    m_viewer.enable_transparent_mode(transparent_mode_on);
}

void SceneViewerPanel::draw_model_section() {
    bool surface_on = m_viewer.is_surface_rendering_enabled();
    if(ImGui::Checkbox("Surface ON", &surface_on))
        m_viewer.enable_surface_rendering(surface_on);
    
    bool wireframe_on = m_viewer.is_wireframe_rendering_enabled();
    if(ImGui::Checkbox("Wireframe ON", &wireframe_on))
        m_viewer.enable_wireframe_rendering(wireframe_on);
}

void SceneViewerPanel::draw_reshaping_section() {
    if(ImGui::TreeNode("Gesture Editor")) {
        bool disp_render_on = m_viewer.is_displacement_rendering_enabled();
        if(ImGui::Checkbox("Disp. Arrows ON", &disp_render_on))
            m_viewer.enable_displacement_rendering(disp_render_on);

        if(ImGui::Button("Clear Anchors"))
            m_viewer.clear_editor_fixed_points();
        if(ImGui::Button("Clear Handles"))
            m_viewer.clear_editor_handles();
        if(ImGui::Button("Reset Displacements"))
            m_viewer.reset_handle_displacements();
    }
}

void SceneViewerPanel::draw_straight_section() {
    bool straight_render_on = m_viewer.is_straight_render_enabled();
    if(ImGui::Checkbox("Straight Chains ON", &straight_render_on))
        m_viewer.enable_straight_render(straight_render_on);
}

void SceneViewerPanel::draw_debug_section() {
    bool debug_render_on = m_viewer.is_debug_render_enabled();
    if(ImGui::Checkbox("Debug Render ON", &debug_render_on))
        m_viewer.enable_debug_render(debug_render_on);
}

void SceneViewerPanel::draw_screen_section() {
    // Slider with msaa options
    {
        int msaa = m_viewer.get_screen_msaa();
    
        // This is a hack to show only msaa valid options
        enum MSAA { MSAA_0, MSAA_2, MSAA_4, MSAA_8, MSAA_16, MSAA_32, MSAA_COUNT };
    
        int curr_elem = 0;
        if(msaa > 0)
            curr_elem = (int) log2(msaa);
    
        const char* elems_names[MSAA_COUNT] = { "0", "2", "4", "8", "16", "32"};
        const char* elem_name = (curr_elem >= 0 && curr_elem < MSAA_COUNT) ? elems_names[curr_elem] : "Unknown";
        if(ImGui::SliderInt("Screen MSAA", &curr_elem, 0, MSAA_COUNT - 1, elem_name)) {
            msaa = (int) pow(2.0f, (float) curr_elem);
            m_viewer.set_screen_msaa(msaa);
        }
    }
}

void SceneViewerPanel::draw_snapshot_section() {
    ImGui::SetNextItemOpen(true);
    if(ImGui::TreeNode("Snapshot")) {
        bool show_cropping_frame = m_viewer.is_crop_frame_rendering_enabled();
        if(ImGui::Checkbox("Show Cropping Frame", &show_cropping_frame))
            m_viewer.enable_crop_frame_rendering(show_cropping_frame);

        int snapshot_screen_mult = m_viewer.get_snapshot_size_mult();
        if(ImGui::SliderInt("Screen Mult.", &snapshot_screen_mult, 1, 5))
            m_viewer.set_snapshot_size_mult(snapshot_screen_mult);

        // Slider with msaa options
        {
            int snapshot_msaa = m_viewer.get_snapshot_msaa();

            // This is a hack to show only msaa valid options
            enum MSAA { MSAA_0, MSAA_2, MSAA_4, MSAA_8, MSAA_16, MSAA_32, MSAA_COUNT };

            int curr_elem = 0;
            if(snapshot_msaa > 0)
                curr_elem = (int) log2((float) snapshot_msaa);

            const char* elems_names[MSAA_COUNT] = { "0", "2", "4", "8", "16", "32"};
            const char* elem_name = (curr_elem >= 0 && curr_elem < MSAA_COUNT) ? elems_names[curr_elem] : "Unknown";
            if(ImGui::SliderInt("MSAA", &curr_elem, 0, MSAA_COUNT - 1, elem_name)) {
                snapshot_msaa = (int) pow(2.0f, (float) curr_elem);
                m_viewer.set_snapshot_msaa(snapshot_msaa);
            }
        }

        // Save snapshot button
        if(ImGui::Button("Save Snapshot")) {
            std::string fn = imgui_utils::open_save_dialog("png", "PNG");

            if(!fn.empty())
                m_viewer.save_screenshot(fn);
        }
    }
}

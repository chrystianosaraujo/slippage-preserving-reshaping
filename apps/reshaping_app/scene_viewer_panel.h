#pragma once

#include "globals.h"
#include "scene_viewer.h"

class SceneViewerPanel {
public:
    SceneViewerPanel(SceneViewer& viewer);

    void draw_ui();

private:
    void draw_view_mode_section();
    void draw_model_section();
    void draw_reshaping_section();
    void draw_straight_section();
    void draw_debug_section();
    void draw_screen_section();
    void draw_snapshot_section();

private:
    SceneViewer& m_viewer;
};

#pragma once

#include <imgui.h>
#include <glad/glad.h>

#include <vector>
#include <string>

namespace ca_essentials {
namespace ui {

// Floating toolbar implementation similar to the one available in Blender
class ImGuiToolbar {
public:
    struct Style {
        ImVec2 button_size = ImVec2(45.0f, 45.0f);
        ImVec4 button_color;
        ImVec4 button_active_color;
        ImVec4 button_hovered_color;
        ImVec4 bg_color = ImVec4(50, 50, 50, 255);
        ImVec2 toolbar_margin = ImVec2(5.0f, 5.0f);

        float button_padding = 3.0;
        float toolbar_padding = 1.0f;
        float separator_len = 4.0f;
        float items_gap = 1.0f;
        float bg_rounding = 2.0f;
    };

public:
    ImGuiToolbar();
    ImGuiToolbar(const Style& style);

    void add_button(GLuint tex_id, const std::string& tip, int* active);
    void add_separator();

    void render_imgui(const ImVec2& pos);

private:
    struct Button {
        Button(unsigned int tex = 0, const std::string tip = "", int* v=nullptr)
        : tex_id(tex), tip(tip), v(v) {}

        unsigned int tex_id = 0;
        std::string tip;
        int* v=nullptr;
    };

    void render_bg_rectangle(const ImVec2& pos, const ImVec2& rect_size);
    void render_group_items(const ImVec2& pos, int group_i);

    const ImVec2& get_group_rect_size(int g) const;

    void compute_grouped_items_height();
    void compute_group_sizes();

private:
    std::vector<std::vector<Button>> m_grouped_buttons;
    Style m_style;

    std::vector<ImVec2> m_groups_size;
    std::vector<std::vector<float>> m_grouped_items_height;
};

}
}

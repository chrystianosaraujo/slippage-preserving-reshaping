#include <ca_essentials/ui/imgui_toolbar.h>
#include <ca_essentials/ui/imgui_toggle_image_button.h>

namespace ca_essentials {
namespace ui {

ImGuiToolbar::ImGuiToolbar() {
}

ImGuiToolbar::ImGuiToolbar(const Style& style)
: m_style(style) {

}

void ImGuiToolbar::add_button(GLuint tex_id, const std::string& tip, int* active) {
    if(m_grouped_buttons.empty())
        m_grouped_buttons.emplace_back();

    m_grouped_buttons.back().emplace_back(tex_id, tip, active);

    if(!m_grouped_items_height.empty()) {
        m_grouped_items_height.clear();
        m_groups_size.clear();
    }
}

void ImGuiToolbar::add_separator() {
    if(m_grouped_buttons.empty())
        return;

    m_grouped_buttons.emplace_back();

    if(!m_grouped_items_height.empty()) {
        m_grouped_items_height.clear();
        m_groups_size.clear();
    }
}

void ImGuiToolbar::render_imgui(const ImVec2& pos) {
    if(m_grouped_items_height.empty()) {
        compute_grouped_items_height();
        compute_group_sizes();
    }

    ImVec2 win_padding = ImGui::GetStyle().WindowPadding;

    // For some unknown reason yet, ImGui's SetCursor works relative to the current window when
    // drawing using the DrawList (rect drawing) and absolute position when adding buttons...
    //
    // WindowPadding only affects the way Buttons are drawn
    ImVec2 rec_pos     = ImVec2(pos.x + m_style.toolbar_margin.x, pos.y + m_style.toolbar_margin.y);
    ImVec2 buttons_pos = ImVec2(m_style.toolbar_margin.x + win_padding.x,
                                m_style.toolbar_margin.y + win_padding.y);

    for(size_t g = 0; g < m_grouped_buttons.size(); ++g) {
        const auto& group = m_grouped_buttons.at(g);
        const ImVec2& rec_size = get_group_rect_size((int) g);

        render_bg_rectangle(rec_pos, rec_size);
        render_group_items(buttons_pos, (int) g);

        rec_pos.y += rec_size.y + 
                     m_style.separator_len +
                     m_style.items_gap;

        buttons_pos.y += rec_size.y + 
                     m_style.separator_len +
                     m_style.items_gap;
    }
}

void ImGuiToolbar::render_bg_rectangle(const ImVec2& pos, const ImVec2& rect_size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImVec4& bg_color = m_style.bg_color;
    
    ImVec2 p0 = pos;
    ImVec2 p1 = ImVec2(p0.x + rect_size.x, p0.y + rect_size.y);
    ImU32 col_a = ImGui::GetColorU32(IM_COL32(bg_color.x, bg_color.y,
                                              bg_color.z, bg_color.w));
    draw_list->AddRectFilled(p0, p1, col_a, m_style.bg_rounding);
}

void ImGuiToolbar::render_group_items(const ImVec2& pos, int group_i) {
    namespace ui = ca_essentials::ui;
    const auto& group = m_grouped_buttons.at(group_i);

    ImVec2 curr_pos = ImVec2(pos.x + m_style.toolbar_padding, pos.y + m_style.toolbar_padding);
    for(size_t i = 0; i < group.size(); ++i) {
        const auto& button = group.at(i);

        ImGui::SetCursorPos(curr_pos);
        void* tex_id_ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(button.tex_id));

        ui::ImGuiToggleImageButton(reinterpret_cast<void*>(tex_id_ptr),
                                   m_style.button_size, (bool*) button.v, ImVec2(0, 0), ImVec2(1, 1), (int) m_style.button_padding); 

        curr_pos.y += m_grouped_items_height.at(group_i).at(i);
    }
}

const ImVec2& ImGuiToolbar::get_group_rect_size(int g) const {
    return m_groups_size.at(g);
}

void ImGuiToolbar::compute_grouped_items_height() {
    size_t num_groups = m_grouped_buttons.size();
    m_grouped_items_height.resize(num_groups);
    for(size_t g = 0; g < num_groups; ++g) {
        const auto& group = m_grouped_buttons.at(g);
        size_t num_buttons = group.size();

        m_grouped_items_height.at(g).resize(num_buttons);
        for(size_t i = 0; i < num_buttons; ++i) {
            bool is_last = i == (num_buttons - 1);

            float button_padding = !is_last ? m_style.button_padding * 2.0f : 0.0f;
            float gap = !is_last ? m_style.items_gap : 0.0f;
            m_grouped_items_height.at(g).at(i) = m_style.button_size.y   +
                                                 button_padding          +
                                                 gap;
        }
    }
}

void ImGuiToolbar::compute_group_sizes() {
    size_t num_groups = m_grouped_buttons.size();
    m_groups_size.resize(num_groups);

    for(size_t g = 0; g < num_groups; ++g) {
        const auto& group = m_grouped_buttons.at(g);
        size_t num_buttons = group.size();

        auto& g_size = m_groups_size.at(g);
        float width =  m_style.button_size.x +
                       (m_style.toolbar_padding * 2.0f) +
                       (m_style.button_padding  * 2.0f);
        float height = (m_style.toolbar_padding * 2.0f) +
                       (m_style.button_padding  * 2.0f);

        g_size = ImVec2(width, height);
        for(size_t i = 0; i < num_buttons; ++i)
            g_size.y += m_grouped_items_height.at(g).at(i);
    }
}

}
}

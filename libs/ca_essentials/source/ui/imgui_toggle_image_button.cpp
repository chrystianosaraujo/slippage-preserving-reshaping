#include <ca_essentials/ui/imgui_toggle_image_button.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace ca_essentials {
namespace ui {

bool ImGuiToggleImageButton(ImTextureID texture_id,
                            const ImVec2& size, bool* v,
                            const ImVec2& uv0, const ImVec2& uv1,
                            int frame_padding, const ImVec4& bg_col,
                            const ImVec4& tint_col) {
    ImGuiContext& g = *ImGui::GetCurrentContext();

    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    // Default to using texture ID as ID. User can still push string/integer prefixes.
    ImGui::PushID((void*)(intptr_t) texture_id);
    const ImGuiID id = window->GetID("#image");
    ImGui::PopID();

    if (frame_padding >= 0)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2((float)frame_padding, (float)frame_padding));

    bool pressed = false;
    {
        const ImVec2 padding = g.Style.FramePadding;
        const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered, held;
        pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        if(pressed) {
            *v = !(*v);
        }

        // Render
        const ImU32 col = ImGui::GetColorU32(((held && hovered) || *v) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImGui::RenderNavHighlight(bb, id);
        ImGui::RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));
        if (bg_col.w > 0.0f)
            window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, ImGui::GetColorU32(bg_col));
        window->DrawList->AddImage(texture_id, bb.Min + padding, bb.Max - padding, uv0, uv1, ImGui::GetColorU32(tint_col));

    }
    if (frame_padding >= 0)
        ImGui::PopStyleVar();

    return pressed;
}

}
}

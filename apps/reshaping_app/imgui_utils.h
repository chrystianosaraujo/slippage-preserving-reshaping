#pragma once

#include <imgui_internal.h>
//#include <IconsFontAwesome5.h>

#if 0
#include <nfd.h>
#endif

#include <string>

namespace imgui_utils {

    inline constexpr float GUI_INDENTATION_SIZE = 20.0f;

    inline void SetupImGuiStyle(bool bStyleDark_, float alpha_) {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    }

    inline void begin_disabled() {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    inline void end_disabled() {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

    inline void push_indentation() {
        ImGui::Indent(GUI_INDENTATION_SIZE);
    }

    inline void pop_indentation() {
        ImGui::Indent(-GUI_INDENTATION_SIZE);
    }

    inline void add_underline(ImColor col_) {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        min.y = max.y;
        ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
    }


#if 0
    // hyperlink urls
    // Code from: https://gist.github.com/dougbinks/ImGuiUtils.h
    // TODO: fix code style
    inline void text_url(const char* name_, const char* URL_, bool SameLineBefore_, bool SameLineAfter_)
    {
        if (SameLineBefore_) { ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.93f, 1.0f));
        ImGui::Text(name_);
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
                platform_utils::open_file(URL_);

            add_underline(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            ImGui::SetTooltip(ICON_FA_LINK "  Open\n%s", URL_);
        }
        else
        {
            add_underline(ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        }
        if (SameLineAfter_) { ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); }
    }
#endif

#if 0
    inline std::string open_folder_dialog() {
        nfdchar_t* path = nullptr;
        nfdresult_t result = NFD_PickFolder(nullptr, &path);
        if (result != NFD_OKAY) return "";

        auto path_string = std::string(path);
        free(path);

        return path_string;
    }

    inline std::string open_save_dialog(const std::string& ext)
    {
        namespace fs = lagrange::fs;

        nfdchar_t* path = nullptr;
        nfdresult_t result = NFD_SaveDialog(ext.c_str(), nullptr, &path);
        if (result == NFD_OKAY) {
            std::string s = fs::get_string_ending_with(path, ("." + ext).c_str());
            free(path);
            return s;
        }

        return "";
    }
#endif
}
#pragma once

#include <imgui_internal.h>

#include <nfd.h>

#include <filesystem>
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

inline void add_spacing(float h) {
    ImGui::Dummy(ImVec2(0.0f, h));
}

inline void add_spacing(float w, float h) {
    ImGui::Dummy(ImVec2(w, h));
}

inline std::string open_folder_dialog() {
    std::string out_str;

    nfdchar_t* path = nullptr;
    nfdresult_t result = NFD_PickFolder(&path, nullptr);
    if(result == NFD_OKAY) {
        out_str = path;
        NFD_FreePath(path);
    }

    return out_str;
}

inline std::string open_file_dialog(const std::string& ext, const std::string& desc) {
    std::string out_str;

    nfdfilteritem_t filter_item[1] = {
        {desc.c_str(), ext.c_str()}
    };

    nfdchar_t* path = nullptr;
    nfdresult_t result = NFD_OpenDialog(&path, filter_item, 1, nullptr);
    if(result == NFD_OKAY) {
        out_str = path;

        NFD_FreePath(path);
    }

    return out_str;
}

inline std::string open_save_dialog(const std::string& ext, const std::string& desc) {
    namespace fs = std::filesystem;

    nfdfilteritem_t filter_item[1] = {
        {desc.c_str(), ext.c_str()}
    };

    std::string out_str;

    nfdchar_t* path = nullptr;
    nfdresult_t result = NFD_SaveDialog(&path, filter_item, 1, nullptr, nullptr);
    if(result == NFD_OKAY) {
        fs::path out_path(path);
        out_path.replace_extension(ext);
        out_str = out_path.string();

        NFD_FreePath(path);
    }

    return out_str;
}

}
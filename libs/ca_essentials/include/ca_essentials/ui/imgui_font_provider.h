#pragma once

#include <imgui.h>

#include <string>
#include <unordered_map>

namespace ca_essentials {
namespace ui {

class ImGuiFontProvider {
public:
    static ImFont* add_font(const ImGuiIO& io, const std::string& id,
                                  const std::string& font_fn, int font_size) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(font_fn.c_str(), (float) font_size);
        ImGuiFontProvider::s_fonts.emplace(id, font);
        return font;
    }

    // ImGui requires a non-const pointer
    static ImFont* get_font(const std::string& id) {
        auto& itr = ImGuiFontProvider::s_fonts.find(id);
        if(itr != ImGuiFontProvider::s_fonts.end())
            return itr->second;
        else
            return nullptr;
    }

    static void clean() {
        for(auto& [id, font] : ImGuiFontProvider::s_fonts)
            delete font;
    }

private:
    inline static std::unordered_map<std::string, ImFont*> s_fonts;
};

}
}

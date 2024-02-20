#include <ca_essentials/core/color.h>

#include <sstream>
#include <array>
#include <cstdlib>

namespace {

static std::vector<ca_essentials::core::Color> s_default_colors;

void initialize_colors() {
    s_default_colors.reserve(55);
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#90ee90"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#72bafd"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#fdb572"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#d77373"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#9fcaa5"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#8a7247"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#a78956"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#7cd884"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#add8e6"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#ca9191"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#4f93b0"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#f8fa6d"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#b46363"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#bda984"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#50945b"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#70c2dd"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#fb9650"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#dcdcdc"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#0048BA"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#B0BF1A"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#7CB9E8"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#C0E8D5"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#B284BE"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#72A0C1"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#EDEAE0"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#F0F8FF"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#C46210"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#EFDECD"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#E52B50"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#9F2B68"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#F19CBB"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#AB274F"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#D3212D"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#3B7A57"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#FFBF00"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#FF7E00"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#9966CC"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#3DDC84"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#CD9575"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#665D1E"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#915C83"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#841B2D"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#FAEBD7"));
    s_default_colors.push_back(ca_essentials::core::color_from_hex("#008000"));
}

}

namespace ca_essentials {
namespace core {

std::string color_to_hex(const Color& c, bool include_alpha) {
    std::array<int, 4> comps = {
        (int) (c.r * 255.0),
        (int) (c.g * 255.0),
        (int) (c.b * 255.0),
        (int) (c.a * 255.0),
    };

    std::stringstream ss;
    ss << "#";
    int num_comps = include_alpha ? 4 : 3;
    for(int i = 0; i < num_comps; ++i) {
        int c = comps[i];

        std::stringstream hex_c;
        hex_c << std::hex << c;

        // Guaranteeing it is represented by a two numbers always
        std::string hex_c_str = hex_c.str();
        if(hex_c_str.size() == 1)
            hex_c_str = "0" + hex_c_str;

        ss << hex_c_str;
    }
    return ss.str();
}

std::string color_to_hex(const Eigen::Vector4f& c, bool include_alpha) {
    Color color(c.x(), c.y(), c.z(), c.w());
    return color_to_hex(color);
}

std::string color_to_hex(const Eigen::Vector3f& c) {
    Color color(c.x(), c.y(), c.z(), 1.0);
    return color_to_hex(color, false);
}

Color color_from_hex(const std::string& str) {
    std::string sub;

    Color c;
    sub = str.substr(1, 2);
    c.r = strtol(sub.c_str(), nullptr, 16) / 255.0f;

    sub = str.substr(3, 2);
    c.g = strtol(sub.c_str(), nullptr, 16) / 255.0f;

    sub = str.substr(5, 2);
    c.b = strtol(sub.c_str(), nullptr, 16) / 255.0f;

    // input str includes alpha information
    if (str.size() == 9) {
        sub = str.substr(7, 2);
        c.a = strtol(sub.c_str(), nullptr, 16) / 255.0f;
    }
    else
        c.a = 1.0f;

    return c;
}

const std::vector<Color>& get_default_colors() {
    if(s_default_colors.empty())
        initialize_colors();

    return s_default_colors;
}

};
};

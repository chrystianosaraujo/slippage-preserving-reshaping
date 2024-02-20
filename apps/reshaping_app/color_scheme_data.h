#pragma once

#include <ca_essentials/renderer/material.h>
#include <ca_essentials/renderer/light.h>

#include <string>

enum ColorScheme {
    BLUE   = 0,
    CREAMY = 1,
    GREEN  = 2,
    PINK   = 3,

    LAST    = PINK,
    DEFAULT = BLUE,
    COUNT   = LAST + 1,
    UNKNOWN = COUNT + 1
};

std::string color_to_label(ColorScheme color);
ColorScheme label_to_color(const std::string& color_str);

const Material& get_colorscheme_material(ColorScheme color);
const Light& get_colorscheme_light(ColorScheme color);

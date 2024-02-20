#include "color_scheme_data.h"

#include <vector>

namespace {

std::vector<Material> s_default_materials;
std::vector<Light>    s_default_lights;

void init_blue_color_data() {
    Material& mat = s_default_materials[ColorScheme::BLUE];
    mat.Kd = glm::fvec3(126.0f/255.0f, 161.0f/255.0f, 215.0f/255.0);
    mat.Ks = glm::fvec3(0.00f, 0.00f, 0.00f);
    mat.Ka = glm::fvec3(88.0f/255.0f, 111.0f/255.0f, 125.0f/255.0f);

    Light& light = s_default_lights[ColorScheme::BLUE];
    light.Ld = glm::fvec3(171.0/255.0f, 171.0f/255.0f, 171.0f/255.0);
    light.Ls = glm::fvec3(0.0f, 0.0f, 0.0f);
    light.La = glm::fvec3(88.0f/255.0f, 111.0f/255.0f, 125.0f/255.0f);
}

void init_green_color_data() {
    Material& mat = s_default_materials[ColorScheme::GREEN];
    mat.Kd = glm::fvec3(135.0f/255.0f, 169.0f/255.0f, 163.0f/255.0);
    mat.Ks = glm::fvec3(0.0f, 0.0f, 0.0f);
    mat.Ka = glm::fvec3(230.0f/255.0f, 128.0f/255.0f, 77.0f/255.0f);

    Light& light = s_default_lights[ColorScheme::GREEN];
    light.Ld = glm::fvec3(171.0/255.0f, 171.0f/255.0f, 171.0f/255.0);
    light.Ls = glm::fvec3(0.0f, 0.0f, 0.00f);
    light.La = glm::fvec3(116.0f/255.0f, 144.0f/255.0f, 161.0f/255.0f);
}

void init_creamy_color_data() {
    Material& mat = s_default_materials[ColorScheme::CREAMY];
    mat.Kd = glm::fvec3(199.0f/ 255.0f, 158.0f/255.0f, 140.0f/255.0f);
    mat.Ks = glm::fvec3(0.0f, 0.0f, 0.0f);
    mat.Ka = glm::fvec3(230.0f/ 255.0f, 128.0f/255.0f, 77.0/255.0f);

    Light& light = s_default_lights[ColorScheme::CREAMY];
    light.Ld = glm::fvec3(171.0/255.0, 171.0/255.0, 171.0/255.0);
    light.Ls = glm::fvec3(0.0, 0.0, 0.0);
    light.La = glm::fvec3(114.0/255.0 ,139.0/255.0, 156.0/255.0);
}

void init_pink_color_data() {
    Material& mat = s_default_materials[ColorScheme::PINK];

    Light& light = s_default_lights[ColorScheme::PINK];
}

void init_default_materials_and_lights() {
    s_default_materials.resize(ColorScheme::COUNT);
    s_default_lights.resize(ColorScheme::COUNT);

    init_blue_color_data();
    init_green_color_data();
    init_creamy_color_data();
    init_pink_color_data();
}

}

const Material& get_colorscheme_material(ColorScheme color) {
    if(s_default_materials.empty())
        init_default_materials_and_lights();

    if(color == ColorScheme::UNKNOWN)
        return s_default_materials.at((int) ColorScheme::DEFAULT);
    else
        return s_default_materials.at((int) color);
}

const Light& get_colorscheme_light(ColorScheme color) {
    if(s_default_lights.empty())
        init_default_materials_and_lights();

    if(color == ColorScheme::UNKNOWN)
        return s_default_lights.at((int) ColorScheme::DEFAULT);
    else
        return s_default_lights.at((int) color);
}

std::string color_to_label(ColorScheme color) {
    switch(color) {
        case ColorScheme::BLUE:  return "blue";
        case ColorScheme::GREEN: return "green";
        case ColorScheme::CREAMY:return "creamy";
        case ColorScheme::PINK:  return "pink";
        default: return "unknown";
    }
}

ColorScheme label_to_color(const std::string& color_str) {
    if(color_str == "blue")   return ColorScheme::BLUE;
    if(color_str == "green")  return ColorScheme::GREEN;
    if(color_str == "creamy") return ColorScheme::CREAMY;
    if(color_str == "pink")   return ColorScheme::PINK;
    return ColorScheme::UNKNOWN;
}

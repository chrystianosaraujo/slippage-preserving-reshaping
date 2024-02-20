#pragma once

#include <string>
#include <vector>

#include <Eigen/Core>

namespace ca_essentials {
namespace core {

struct Color {
    Color(float r = 0.0, float g = 0.0, float b = 0.0, float a = 1.0)
        : r(r), g(g), b(b), a(a) {};

    Color(const Eigen::Vector4f& c)
        : r(c.x()), g(c.y()), b(c.z()), a(c.w()) {};

    Color(const Eigen::Vector3f& c)
        : r(c.x()), g(c.y()), b(c.z()), a(1.0f) {};

    // TODO: remove this and use eigen internally directly
    Eigen::Vector4f to_eigen_vec() const {
        return Eigen::Vector4f(r, g, b, a);
    }

    float r;
    float g;
    float b;
    float a;
};

// Converts color object to hex string
std::string color_to_hex(const Color& c, bool include_alpha = true);
std::string color_to_hex(const Eigen::Vector4f& c, bool include_alpha = true);
std::string color_to_hex(const Eigen::Vector3f& c);

// Converts from hex string to color object
Color color_from_hex(const std::string& str);

const std::vector<Color>& get_default_colors();
};

};

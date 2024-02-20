#pragma once

#include <cmath>

namespace ca_essentials{
namespace core {

inline double to_degrees(double rad) {
    return rad * 180.0 / M_PI;
}

inline double to_radians(double deg) {
    return deg * M_PI / 180.0;
}

}
}

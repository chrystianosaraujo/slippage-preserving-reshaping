#pragma once

#include "globals.h"

#include <Eigen/Core>

inline Eigen::Vector4i
compute_crop_frame(const Eigen::Vector2i& win_size) {
    auto win_center = win_size / 2;

    int crop_h = win_size.y() - globals::io::crop_top_margin * 2;
    int crop_w = (int) (crop_h * globals::io::crop_width);

    int x0 = win_center.x() - (int)(crop_w / 2.0f);
    int y0 = globals::io::crop_top_margin;

    int x1 = win_center.x() + (int)(crop_w / 2.0f);
    int y1 = win_size.y() - globals::io::crop_top_margin;

    return {x0, y0, x1, y1};
}
           

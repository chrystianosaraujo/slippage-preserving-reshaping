#pragma once

#include <vector>

namespace ca_essentials {
namespace core {

// Crops the given image to the given dimensions - from (x0, y0) to (x1, y1).
template <typename T>
std::vector<T>
crop_image(const std::vector<T>& image,
           int width, int height, int comps,
           int x0, int y0,
           int x1, int y1) {

    auto to_flatten_index = [width, height, comps](int x, int y) -> int {
        return y * width * comps + (x * comps);
    };

    int new_w = x1 - x0;
    int new_h = y1 - y0;

    std::vector<T> new_image(new_w * new_h * comps);
    int cropped_flattened_idx = 0;
    for(int y = 0; y < new_h; ++y) {
        for(int x = 0; x < new_w; ++x) {
            int orig_x = x0 + x;
            int orig_y = y0 + y;

            int orig_flattened_idx = to_flatten_index(orig_x, orig_y);
            for(int c = 0; c < comps; ++c)
                new_image[cropped_flattened_idx + c] = image[orig_flattened_idx + c];

            cropped_flattened_idx += comps;
        }
    }

    return new_image;
}

}
}

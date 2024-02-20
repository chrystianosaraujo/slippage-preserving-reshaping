#pragma once

#include <chrono>
#include <string>
#include <cassert>
#include <unordered_map>
#include <iostream>

namespace ca_essentials {
namespace core {

class Timer {
    using TimePoint = std::chrono::high_resolution_clock::time_point;

public:
    void start(const std::string& start_point_id) {
        m_start_points[start_point_id] = now();
    }

    double elapsed(const std::string& start_point_id) const {
        namespace chrono = std::chrono;

        auto curr_time = now();
        const auto itr = m_start_points.find(start_point_id);

        assert(itr != m_start_points.end() &&
               "Unexpected start point id \"" + start_point_id.c_str() + "\"");

        return (double) chrono::duration_cast<chrono::milliseconds>(curr_time - itr->second).count();
    }

private:
    TimePoint now() const {
        return std::chrono::high_resolution_clock::now();
    }

private:
    std::unordered_map<std::string, TimePoint> m_start_points;
};

}
}

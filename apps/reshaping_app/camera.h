/*
Author: Chrystiano Araujo (araujoc@cs.ubc.ca)

This file is an adaptation of the Polyscope camera class,
which is distributed under the following license:

MIT License

Copyright (c) 2017-2019 Nicholas Sharp and the Polyscope contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include "camera_parameters.h"

#include <glm/glm.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <functional>
#include <array>
#include <string>

struct CameraInfo {
    // flattened
    std::array<double, 16> view_mat = {0.0};

    double fov;
    double near_clip_ratio;
    double far_clip_ratio;
    std::string projection_mode;
};

class Camera {
public:
    enum class NavigateStyle {
        Turntable = 0, Free, Planar, Arcball
    };

    enum class UpDir {
        XUp = 0, YUp, ZUp, NegXUp, NegYUp, NegZUp
    };

    enum class ProjectionMode {
        Perspective = 0, Orthographic, UNKNOWN
    };

public:
    Camera(const glm::ivec2 win_size,
           const double default_fov=45.0,
           const std::function<void()>& request_redraw_cb = {});
    ~Camera();

    void set_win_size(const glm::ivec2& win_size);
    const glm::ivec2& get_win_size() const;

    void set_length_scale(double len_scale);
    void set_bounding_box(const glm::fvec3& bbmin, const glm::fvec3& bbmax);
    void set_fov(double fov);
    double get_fov() const;

    void process_translate(glm::vec2 delta);
    void process_rotate(glm::vec2 start_p, glm::vec2 end_p);
    void process_zoom(double amount);

    // The "home" view looks at the center of the scene's bounding box.
    glm::mat4 compute_home_view();
    void reset_camera_to_home_view();

    void set_up_dir(UpDir new_up_dir);
    UpDir get_up_dir() const;
    glm::vec3 get_up_vec() const;

    glm::mat4 get_view_matrix() const;
    glm::mat4 get_perspective_matrix() const;
    glm::vec3 get_world_position() const;
    glm::mat4 get_viewport_matrix() const;
    glm::vec4 get_viewport() const;

    CameraInfo get_camera_info() const;
    void set_camera_info(const CameraInfo& info);

private:
    void get_camera_frame(glm::vec3& lookDir, glm::vec3& upDir, glm::vec3& rightDir);
    glm::fvec3 center() const;

    void request_redraw() const;

private:
    NavigateStyle m_style = NavigateStyle::Free;
    UpDir m_up_dir = UpDir::YUp;
    ProjectionMode m_projection_mode = ProjectionMode::Perspective;

    glm::ivec2 m_win_size = glm::ivec2(1280, 720);
    int m_init_window_posX = 20;
    int m_init_window_posY = 20;

    double m_length_scale = 1.0;
    double m_move_scale = 1.0;
    double m_default_near_clip_ratio = 0.005;
    double m_default_far_clip_ratio  = 20.0;
    double m_near_clip_ratio = m_default_near_clip_ratio;
    double m_far_clip_ratio  = m_default_far_clip_ratio;

    glm::mat4x4 m_view_mat;

    double m_default_fov = 45.0;
    const double m_min_fov = 5.0;
    const double m_max_fov = 160.0;
    double m_fov = m_default_fov;

    glm::fvec3 m_bbmin = glm::fvec3(0.0);
    glm::fvec3 m_bbmax = glm::fvec3(0.0);

    std::function<void()> m_request_redraw_cb;
};
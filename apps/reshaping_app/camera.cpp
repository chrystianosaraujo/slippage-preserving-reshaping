#include "camera.h"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace {

std::string projection_mode_to_string(Camera::ProjectionMode mode) {
    using ProjectionMode = Camera::ProjectionMode;

    if(mode == ProjectionMode::Perspective)
        return "Perspective";
    else if(mode == ProjectionMode::Orthographic)
        return "Orthographic";
    else
        return "[UNKOWN]"; // ERROR
}
Camera::ProjectionMode string_to_projection_mode(const std::string& str) {
    using ProjectionMode = Camera::ProjectionMode;

    if(str == projection_mode_to_string(ProjectionMode::Perspective))
        return ProjectionMode::Perspective;
    else if(str == projection_mode_to_string(ProjectionMode::Orthographic))
        return ProjectionMode::Orthographic;
    else
        return ProjectionMode::UNKNOWN;
}

}

Camera::Camera(const glm::ivec2 win_size,
               const double default_fov,
               const std::function<void()>& request_redraw_cb)
: m_win_size(win_size)
, m_default_fov(default_fov)
, m_fov(default_fov)
, m_request_redraw_cb(request_redraw_cb) {

}

Camera::~Camera() {

}

void Camera::set_win_size(const glm::ivec2& win_size) {
    m_win_size = win_size;
}

const glm::ivec2& Camera::get_win_size() const {
    return m_win_size;
}

void Camera::set_length_scale(double len_scale) {
    m_length_scale = len_scale;
}

void Camera::set_bounding_box(const glm::fvec3& bbmin, const glm::fvec3& bbmax) {
    m_bbmin = bbmin;
    m_bbmax = bbmax;
}

void Camera::set_fov(double fov) {
    m_fov = fov;
}

double Camera::get_fov() const {
    return m_fov;
}

void Camera::process_translate(glm::vec2 delta) {
  if (glm::length(delta) == 0) {
    return;
  }
  // Process a translation
  double movement_scale = m_length_scale * 0.6 * m_move_scale;
  glm::mat4x4 cam_space_T = glm::translate(glm::mat4x4(1.0),
                                           (float) movement_scale * glm::vec3(delta.x, delta.y, 0.0));
  m_view_mat = cam_space_T * m_view_mat;

  request_redraw();
}

void Camera::process_rotate(glm::vec2 start_p, glm::vec2 end_p) {
  if (start_p == end_p)
    return;

  // Get frame
  glm::vec3 frame_look_dir, frame_up_dir, frame_right_dir;
  get_camera_frame(frame_look_dir, frame_up_dir, frame_right_dir);

  switch (m_style) {
  case NavigateStyle::Turntable: {
    glm::vec2 drag_delta = end_p - start_p;
    float del_theta = (float)(2.0 * drag_delta.x * m_move_scale);
    float del_phi   = (float)(2.0 * drag_delta.y * m_move_scale);

    // Translate to center
    m_view_mat = glm::translate(m_view_mat, center());

    // Rotation about the horizontal axis
    glm::mat4x4 phi_cam_R = glm::rotate(glm::mat4x4(1.0), -del_phi, frame_right_dir);
    m_view_mat = m_view_mat * phi_cam_R;

    // Rotation about the vertical axis
    glm::vec3 turntable_up;
    switch (m_up_dir) {
    case UpDir::XUp:
      turntable_up = glm::vec3(1., 0., 0.);
      break;
    case UpDir::YUp:
      turntable_up = glm::vec3(0., 1., 0.);
      break;
    case UpDir::ZUp:
      turntable_up = glm::vec3(0., 0., 1.);
      break;
    case UpDir::NegXUp:
      turntable_up = glm::vec3(-1., 0., 0.);
      break;
    case UpDir::NegYUp:
      turntable_up = glm::vec3(0., -1., 0.);
      break;
    case UpDir::NegZUp:
      turntable_up = glm::vec3(0., 0., -1.);
      break;
    }
    glm::mat4x4 theta_cam_R = glm::rotate(glm::mat4x4(1.0), del_theta, turntable_up);
    m_view_mat = m_view_mat * theta_cam_R;

    // Undo centering
    m_view_mat = glm::translate(m_view_mat, -center());
    break;
  }
  case NavigateStyle::Free: {
    glm::vec2 drag_delta = end_p - start_p;
    float del_theta = (float)(2.0 * drag_delta.x * m_move_scale);
    float del_phi   = (float)(2.0 * drag_delta.y * m_move_scale);

    // Translate to center
    m_view_mat = glm::translate(m_view_mat, center());

    // Rotation about the vertical axis
    glm::mat4x4 theta_cam_r = glm::rotate(glm::mat4x4(1.0), del_theta, frame_up_dir);
    m_view_mat = m_view_mat * theta_cam_r;

    // Rotation about the horizontal axis
    glm::mat4x4 phi_cam_r = glm::rotate(glm::mat4x4(1.0), -del_phi, frame_right_dir);
    m_view_mat = m_view_mat * phi_cam_r;

    // Undo centering
    m_view_mat = glm::translate(m_view_mat, -center());
    break;
  }
  case NavigateStyle::Planar: {
    // Do nothing
    break;
  }
  case NavigateStyle::Arcball: {
    // Map inputs to unit sphere
    auto to_sphere = [](glm::vec2 v) {
      double x = glm::clamp(v.x, -1.0f, 1.0f);
      double y = glm::clamp(v.y, -1.0f, 1.0f);
      double mag = x * x + y * y;
      if (mag <= 1.0) {
        return glm::vec3{x, y, -std::sqrt(1.0 - mag)};
      } else {
        return glm::normalize(glm::vec3{x, y, 0.0});
      }
    };
    glm::vec3 sphere_start = to_sphere(start_p);
    glm::vec3 sphere_end   = to_sphere(end_p);

    glm::vec3 rot_axis = -cross(sphere_start, sphere_end);
    double rot_mag = std::acos(glm::clamp(dot(sphere_start, sphere_end), -1.0f, 1.0f) * m_move_scale);

    glm::mat4 camera_rotate = glm::rotate(glm::mat4x4(1.0), (float) rot_mag, glm::vec3(rot_axis.x, rot_axis.y, rot_axis.z));

    // Get current camera rotation
    glm::mat4x4 R;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        R[i][j] = m_view_mat[i][j];
      }
    }
    R[3][3] = 1.0;

    glm::mat4 update = glm::inverse(R) * camera_rotate * R;

    m_view_mat = m_view_mat * update;
    break;
  }
  }

  request_redraw();
}

void Camera::process_zoom(double amount) {
  if (amount == 0.0) return;

  // Translate the camera forwards and backwards
  switch (m_projection_mode) {
  case ProjectionMode::Perspective: {
    float movementScale = (float)(m_length_scale * 0.1 * m_move_scale);
    glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), glm::vec3(0., 0., movementScale * amount));
    m_view_mat = camSpaceT * m_view_mat;
    break;
  }
  case ProjectionMode::Orthographic: {
    double fovScale = std::min(m_fov - m_min_fov, m_max_fov - m_fov) / (m_max_fov - m_min_fov);
    m_fov += -fovScale * amount;
    m_fov = glm::clamp(m_fov, m_min_fov, m_max_fov);
    break;
  }
  }

  request_redraw();
}

glm::mat4 Camera::compute_home_view() {
    glm::mat4x4 R(1.0);
    glm::vec3 base_up;

    switch (m_up_dir) {
    case UpDir::XUp:
    case UpDir::NegXUp:
      base_up = glm::vec3(1., 0., 0.);
      R = glm::rotate(glm::mat4x4(1.0), static_cast<float>(M_PI / 2), glm::vec3(0., 0., 1.));

      if (m_up_dir == UpDir::NegXUp) {
        base_up *= -1;
        R = glm::rotate(R, static_cast<float>(M_PI), glm::vec3(0., 0., 1.));
      }
      break;

    case UpDir::YUp:
    case UpDir::NegYUp:
      base_up = glm::vec3(0., 1., 0.);
      // this is our camera's default
      if (m_up_dir == UpDir::NegYUp) {
        base_up *= -1;
        R = glm::rotate(R, static_cast<float>(M_PI), glm::vec3(0., 0., 1.));
      }
      break;

    case UpDir::ZUp:
    case UpDir::NegZUp:
      base_up = glm::vec3(0., 0., 1.);
      R = glm::rotate(glm::mat4x4(1.0), static_cast<float>(M_PI / 2), glm::vec3(-1., 0., 0.));
      R = glm::rotate(glm::mat4x4(1.0), static_cast<float>(M_PI), glm::vec3(0., 1., 0.)) *
          R; // follow common convention for "front"
      if (m_up_dir == UpDir::NegZUp) {
        base_up *= -1;
        R = glm::rotate(R, static_cast<float>(M_PI), glm::vec3(0., 1., 0.));
      }
      break;
    }
    
    // Rotate around the up axis, since our camera looks down -Z
    // (except in planar mode, where we look down +Z so default axes are as-expected)
    if (m_style != NavigateStyle::Planar) {
      R = glm::rotate(R, static_cast<float>(M_PI), base_up);
    }
    
    glm::mat4x4 T_obj = glm::translate(glm::mat4x4(1.0), -center());
    glm::mat4x4 T_cam = glm::translate(glm::mat4x4(1.0),
                                       glm::vec3(0.0, 0.0, -3.5 * m_length_scale));
    
    return T_cam * R * T_obj;
}

void Camera::reset_camera_to_home_view() {
  m_view_mat = compute_home_view();

  m_fov = m_default_fov;
  m_near_clip_ratio = m_default_near_clip_ratio;
  m_far_clip_ratio  = m_default_far_clip_ratio;

  request_redraw();
}

void Camera::set_up_dir(UpDir new_up_dir) {
  m_up_dir = new_up_dir;
  reset_camera_to_home_view();
}

Camera::UpDir Camera::get_up_dir() const {
    return m_up_dir;
}

glm::vec3 Camera::get_up_vec() const {
  switch (m_up_dir) {
  case UpDir::NegXUp:
    return glm::vec3{-1., 0., 0.};
  case UpDir::XUp:
    return glm::vec3{1., 0., 0.};
  case UpDir::NegYUp:
    return glm::vec3{0., -1., 0.};
  case UpDir::YUp:
    return glm::vec3{0., 1., 0.};
  case UpDir::NegZUp:
    return glm::vec3{0., 0., -1.};
  case UpDir::ZUp:
    return glm::vec3{0., 0., 1.};
  }

  // unused fallthrough
  return glm::vec3{0., 0., 0.};
}

glm::mat4 Camera::get_view_matrix() const {
    return m_view_mat;
}

glm::mat4 Camera::get_perspective_matrix() const {
    double far_clip  = m_far_clip_ratio  * m_length_scale;
    double near_clip = m_near_clip_ratio * m_length_scale;
    double fov_rad   = glm::radians(m_fov);
    double aspect_ratio = (double) m_win_size.x / m_win_size.y;
    
    switch (m_projection_mode) {
    case ProjectionMode::Perspective: {
        return glm::perspective(fov_rad, aspect_ratio, near_clip, far_clip);
        break;
    }
    case ProjectionMode::Orthographic: {
        double vert = tan(fov_rad / 2.) * m_length_scale * 2.;
        double horiz = vert * aspect_ratio;
        return glm::ortho(-horiz, horiz, -vert, vert, near_clip, far_clip);
        break;
    }
    }
    return glm::mat4(1.0f); // unreachable
}

glm::vec3 Camera::get_world_position() const {
    // This will work no matter how the view matrix is constructed...
    glm::mat4 inv_view_Mat = inverse(get_view_matrix());
    return glm::vec3{inv_view_Mat[3][0], inv_view_Mat[3][1], inv_view_Mat[3][2]};
}

glm::mat4 Camera::get_viewport_matrix() const {
    float w2 = (float) m_win_size.x / 2.0f;
    float h2 = (float) m_win_size.y / 2.0f;

    return glm::mat4(glm::vec4(w2    , 0.0f    , 0.0f, 0.0f),
                     glm::vec4(0.0f  , h2      , 0.0f, 0.0f),
                     glm::vec4(0.0f  , 0.0f    , 1.0f, 0.0f),
                     glm::vec4(w2 + 0, h2 + 0.f, 0.0f, 1.0f));
}

glm::vec4 Camera::get_viewport() const {
    return glm::vec4(0.0, 0.0, m_win_size.x, m_win_size.y);
}

CameraInfo Camera::get_camera_info() const {
    CameraInfo info;

    info.fov = m_fov;
    info.near_clip_ratio = m_near_clip_ratio;
    info.far_clip_ratio = m_far_clip_ratio;
    info.projection_mode = projection_mode_to_string(m_projection_mode);

    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            info.view_mat[4 * i + j] = m_view_mat[j][i];

    return info;
}

void Camera::set_camera_info(const CameraInfo& info) {
    glm::mat4 view_mat;
    int it = 0;
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            view_mat[j][i] = (float) info.view_mat[it++];

    m_view_mat = view_mat;
    m_fov = info.fov;
    m_near_clip_ratio = info.near_clip_ratio;
    m_far_clip_ratio = info.far_clip_ratio;
    m_projection_mode = string_to_projection_mode(info.projection_mode);
}

void Camera::get_camera_frame(glm::vec3& lookDir,
                              glm::vec3& upDir,
                              glm::vec3& rightDir) {
    glm::mat3x3 R;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        R[i][j] = m_view_mat[i][j];
      }
    }
    glm::mat3x3 Rt = glm::transpose(R);
    
    lookDir  = Rt * glm::vec3(0.0, 0.0, -1.0);
    upDir    = Rt * glm::vec3(0.0, 1.0, 0.0);
    rightDir = Rt * glm::vec3(1.0, 0.0, 0.0);
}

glm::fvec3 Camera::center() const {
    return 0.5f * (m_bbmin + m_bbmax); 
}

void Camera::request_redraw() const {
    if(m_request_redraw_cb)
        m_request_redraw_cb();
}
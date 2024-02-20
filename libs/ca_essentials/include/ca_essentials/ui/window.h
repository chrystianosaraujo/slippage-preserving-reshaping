#pragma once

#include <iostream>
#include <cstdio>
#include <functional>
#include <Eigen/Core>

#include <glad/glad.h>
#include <ca_essentials/renderer/framebuffer.h>

struct GLFWwindow;
struct ImGuiContext;

namespace ca_essentials {
namespace ui {

// Wrapper class for a GLFW window
struct WindowParams {
    WindowParams();

    int width = 1920;
    int height = 1080;
    std::string title;
    bool vsync_on = true;
    bool graphics_debug_on = false;
    bool msaa_on = true;
    bool open_maximized = false;
    Eigen::Vector4f clear_color = Eigen::Vector4f::Ones();
};

class Window {
public:
    Window(const WindowParams& params);
    ~Window();

    void main_loop(const std::function<void()> main_loop);
    void finish_loop() {
        m_finish_loop = true;
    }
    void close();

    Eigen::Vector2i window_size() const;
    Eigen::Vector2i default_framebuffer_size() const;

    void save_screenshot(const std::string& fn,
                         int size_mult=2, int msaa=0,
                         const Eigen::Vector4i crop_frame=Eigen::Vector4i(0, 0));
    bool is_screenshot_mode_on() const;

    int snapshot_max_msaa() const;

    void set_resize_cb(const std::function<void(int w, int h)>& cb);

private:
    void setup_window();
	void destroy_window();

    void pre_render();
    void post_render();

    void prepare_snapshot_fbo();
    void save_screenshot();

    void window_resize_callback(int width, int height);

private:
    class GLFWCallbackHandler {
    public:
        GLFWCallbackHandler() = delete;
        GLFWCallbackHandler(const GLFWCallbackHandler&) = delete;
        GLFWCallbackHandler(GLFWCallbackHandler&&) = delete;
        ~GLFWCallbackHandler() = delete;

        static void resize_cb(GLFWwindow* glfw_win, int width, int height);
        static void register_window_object(Window* win_obj);

    private:
        static Window* m_win_obj;
    };

private:
    WindowParams m_params;
    GLFWwindow* m_window = nullptr;
    ImGuiContext* m_imgui_context = nullptr;

    bool m_finish_loop = false;

    ca_essentials::renderer::Framebuffer m_fbo;
    int m_snapshot_size_mult = 1;
    int m_snapshot_msaa = 0;

    // Snapshot
    bool m_lazy_snapshot_enabled = false;
    bool m_snapshot_enabled = false;
    bool m_force_fbo_update = false;
    Eigen::Vector4i m_snapshot_crop_frame; 
    std::string m_snapshot_fn;

    std::function<void(int w, int h)> m_resize_cb;
};

}
}

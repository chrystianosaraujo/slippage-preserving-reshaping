#include <ca_essentials/ui/window.h>
#include <ca_essentials/ui/imgui_utils.h>
#include <ca_essentials/renderer/gl_debug.h>
#include <ca_essentials/core/logger.h>
#include <ca_essentials/core/crop_image.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace ca_essentials {
namespace ui {
Window* Window::GLFWCallbackHandler::m_win_obj = nullptr;
}
}

namespace {

void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

}

namespace ca_essentials {
namespace ui {

WindowParams::WindowParams() {

}

Window::Window(const WindowParams& params)
: m_params(params) {
    GLFWCallbackHandler::register_window_object(this);

    setup_window();
}

Window::~Window() {
    destroy_window();
}

void Window::main_loop(const std::function<void()> main_loop) {
    m_finish_loop = false;
    m_snapshot_enabled = false;

    while(!glfwWindowShouldClose(m_window) && !m_finish_loop) {
        pre_render();
        main_loop();
        post_render();
    }
}

void Window::close() {
    glfwSetWindowShouldClose(m_window, true);
}

Eigen::Vector2i Window::window_size() const {
    Eigen::Vector2i size;
    glfwGetWindowSize(m_window, &size.x(), &size.y());

    return size;
}

Eigen::Vector2i Window::default_framebuffer_size() const {
    Eigen::Vector2i size;
    glfwGetFramebufferSize(m_window, &size.x(), &size.y());

    return size;
}

void Window::save_screenshot(const std::string& fn,
                             int screen_mult, int msaa,
                             const Eigen::Vector4i crop_frame) {
    bool rebuild_fbo = false;
    if(screen_mult != m_snapshot_size_mult || msaa != m_fbo.msaa())
        rebuild_fbo = true;

    m_snapshot_fn = fn;
    m_snapshot_msaa = msaa;
    m_snapshot_size_mult = screen_mult;
    m_lazy_snapshot_enabled = true;
    if((crop_frame.x() + crop_frame.y()) == 0)
        m_snapshot_crop_frame = Eigen::Vector4i(0, 0,
                                                m_fbo.width(), m_fbo.height());
    else
        m_snapshot_crop_frame = crop_frame * m_snapshot_size_mult;

    if(rebuild_fbo || m_force_fbo_update)
        prepare_snapshot_fbo();
}

bool Window::is_screenshot_mode_on() const {
    return m_snapshot_enabled;
}

int Window::snapshot_max_msaa() const {
    GLint max_msaa;
    glGetIntegerv(GL_MAX_SAMPLES, &max_msaa);

    return max_msaa;
}

void Window::set_resize_cb(const std::function<void(int w, int h)>& cb) {
    m_resize_cb = cb;
}

void Window::setup_window() {
    namespace renderer = ca_essentials::renderer;

    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit())
        throw std::runtime_error("Error while initializing GLFW");

    // Decide GL+GLSL versions
    // GL 4.3 + GLSL 430
    const char* glsl_version = "#version 430";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, m_params.graphics_debug_on);
    glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);

    // Enabling MSAA
    if(m_params.msaa_on)
        glfwWindowHint(GLFW_SAMPLES, 32);

    // Create window with graphics context
    m_window = glfwCreateWindow(m_params.width, m_params.height, m_params.title.c_str(), NULL, NULL);
    if(!m_window)
        throw std::runtime_error("Error while creating GLFW window");

    // This is a temporary hack to make the window open in maximum size. The 
    // hit glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE) is not currently working
    if(m_params.open_maximized)
        glfwMaximizeWindow(m_window);

    glfwMakeContextCurrent(m_window);
    if(m_params.vsync_on)
        glfwSwapInterval(1); // Enable vsync

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize OpenGL context");

    glfwSetWindowSizeCallback(m_window, GLFWCallbackHandler::resize_cb);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    m_imgui_context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
    io.ConfigFlags |= ImGuiDockNodeFlags_PassthruCentralNode;

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui::SetCurrentContext(m_imgui_context);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    if(m_params.msaa_on)
        glEnable(GL_MULTISAMPLE); // Enable MSAA

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if(m_params.graphics_debug_on && (flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        //glDebugMessageCallback(renderer::gl_debug_output, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

#define PRINT_MSAA_INFO
#ifdef PRINT_MSAA_INFO
    GLint bufs, samples;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &bufs);
    glGetIntegerv(GL_SAMPLES, &samples);
    LOGGER.debug("MSAA: buffers = {} samples = {}", bufs, samples);
#endif
}

void Window::destroy_window() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::pre_render() {
    if(m_lazy_snapshot_enabled) {
        m_lazy_snapshot_enabled = false;
        m_snapshot_enabled = true;
    }

    assert(m_window);
    glfwMakeContextCurrent(m_window);
    assert(m_window);

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    Eigen::Vector2i fbo_size;
    if(m_snapshot_enabled) {
        m_fbo.bind();
        glViewport(0, 0, m_fbo.width(), m_fbo.height());
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        const auto win_size = window_size();
        glViewport(0, 0, win_size.x(), win_size.y());
    }

    const Eigen::Vector4f& c = m_params.clear_color;
    glClearColor(c.x(), c.y(), c.z(), c.w());
}

void Window::post_render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if(m_snapshot_enabled)
        save_screenshot();
    
    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
    
    glfwSwapBuffers(m_window);
}

void Window::prepare_snapshot_fbo() {
    using STATUS = ca_essentials::renderer::Framebuffer::STATUS;

    Eigen::Vector2i fbo_size = default_framebuffer_size();
    fbo_size *= m_snapshot_size_mult;

    LOGGER.debug("Snapshot framebuffer being created...");
    LOGGER.debug("    Size: {} {}", fbo_size.x(), fbo_size.y());
    LOGGER.debug("    Screen mult: {}", m_snapshot_size_mult);
    LOGGER.debug("    MSAA: {}", m_snapshot_msaa);

    auto status = m_fbo.setup(fbo_size.x(), fbo_size.y(), m_snapshot_msaa);
    if(status != STATUS::FRAMEBUFFER_COMPLETE)
        LOGGER.error("    Could not be created! FBO is not complete");
    else {
        LOGGER.debug("    Sucessfully created!");
    }

    m_force_fbo_update = false;
}

void Window::save_screenshot() {
    namespace core = ca_essentials::core;

    int w = m_fbo.width();
    int h = m_fbo.height();
    int comps = m_fbo.num_comps();

    int x0 = m_snapshot_crop_frame(0);
    int y0 = m_snapshot_crop_frame(1);
    int x1 = m_snapshot_crop_frame(2);
    int y1 = m_snapshot_crop_frame(3);
    int new_w = x1 - x0;
    int new_h = y1 - y0;

    std::vector<unsigned char> pixels = m_fbo.get_color_buffer();
    std::vector<unsigned char> cropped_pixels = core::crop_image(pixels, w, h, comps,
                                                                 x0, y0, x1, y1); 

    stbi_flip_vertically_on_write(true);
    int saved = stbi_write_png(m_snapshot_fn.c_str(), new_w, new_h, comps, cropped_pixels.data(), 0);

    if(saved)
        LOGGER.info("Snapshot saved to {}", m_snapshot_fn);
    else
        LOGGER.error("Error while saving snapshot to {}", m_snapshot_fn);

    // Clearing snapshot info
    m_snapshot_enabled = false;
    m_snapshot_fn.clear();
}

void Window::window_resize_callback(int width, int height) {
    if(m_resize_cb)
        m_resize_cb(width, height);

    m_force_fbo_update = true;
}

void Window::GLFWCallbackHandler::resize_cb(GLFWwindow* glfw_win, int width, int height) {
    if(m_win_obj)
        m_win_obj->window_resize_callback(width, height);
}

void Window::GLFWCallbackHandler::register_window_object(Window* win_obj) {
    m_win_obj = win_obj;
}

}
}

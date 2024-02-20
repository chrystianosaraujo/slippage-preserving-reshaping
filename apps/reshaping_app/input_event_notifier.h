#pragma once

#include <Eigen/core>
#include <functional>

enum class MouseButton {
    LEFT,
    RIGHT
};

struct MouseEventInfo {
    MouseButton btn;
    bool clicked = false;
    bool released = false;
    bool dragged = false;
    bool down = false;
    bool wheel_changed = false;
    bool shift_mod = false;
    bool ctrl_mod = false;

    // duration mouse has been clicked
    float down_time = 0.0f;

    // duration mouse was clicked for the last event
    float down_time_last = 0.0f;

    float wheel = 0.0f;
    Eigen::Vector2f drag = Eigen::Vector2f::Zero();
    Eigen::Vector2f pos = Eigen::Vector2f::Zero();
};

class InputEventNotifier {
public:
    using MouseEventCB = std::function<void(const MouseEventInfo& e)>;

public:
    static InputEventNotifier& get();

    static void listen_to(const MouseEventCB cb, bool include_mouse_move);

    static void process_inputs();

private:
    InputEventNotifier() {};

    InputEventNotifier(const InputEventNotifier&) = delete;
    InputEventNotifier(const InputEventNotifier&&) = delete;
    void operator=(const InputEventNotifier&) = delete;

    void process_mouse_inputs();
    void notify_mouse_listeners(const MouseEventInfo& info);

private:
    static InputEventNotifier* s_instance;

    std::vector<MouseEventCB> m_mouse_cbs;
    std::vector<bool> m_include_mouse_move;
};

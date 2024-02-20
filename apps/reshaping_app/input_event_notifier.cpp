#include "input_event_notifier.h"

#include <imgui.h>

InputEventNotifier* InputEventNotifier::s_instance = nullptr;

InputEventNotifier& InputEventNotifier::get() {
    if(!InputEventNotifier::s_instance)
        s_instance = new InputEventNotifier();

    return *s_instance;
}

void InputEventNotifier::listen_to(const MouseEventCB cb, bool include_mouse_move) {
    InputEventNotifier::get().m_mouse_cbs.push_back(cb);
    InputEventNotifier::get().m_include_mouse_move.push_back(include_mouse_move);
}

void InputEventNotifier::process_inputs() {
    InputEventNotifier::get().process_mouse_inputs();
}

void InputEventNotifier::process_mouse_inputs() {
    if(!ImGui::IsItemHovered())
        return;

    ImVec2 widget_pos = ImGui::GetItemRectMin();
    ImVec2 widget_size = ImGui::GetItemRectSize();

    ImGuiIO& io = ImGui::GetIO();

    MouseEventInfo info;
    info.pos = {io.MousePos.x - widget_pos.x,
                widget_size.y - (io.MousePos.y - widget_pos.y)};

    info.ctrl_mod = io.KeyCtrl;
    info.shift_mod = io.KeyShift;
    info.down      = io.MouseDown[0];
    info.down_time = io.MouseDownDuration[0];
    info.down_time_last = io.MouseDownDurationPrev[0];

    // Mouse wheel
    {
        float x_offset = io.MouseWheelH;
        float y_offset = io.MouseWheel;

        if(x_offset >= 1e-5f || y_offset >= 1e-5f) {
            if(std::abs(x_offset) > std::abs(y_offset))
                info.wheel = x_offset;
            else
                info.wheel = y_offset;

            info.wheel_changed = true;
        }
    }

    // Process mouse drag
    {
        bool drag_left = ImGui::IsMouseDragging(0);
        bool drag_right = !drag_left && ImGui::IsMouseDragging(1);

        if(drag_left || drag_right) {
            info.dragged = true;
            info.drag = {io.MouseDelta.x / widget_size.x,
                        -io.MouseDelta.y / widget_size.y};

            info.btn = drag_left ? MouseButton::LEFT : MouseButton::RIGHT;
        }
    }

    // Process mouse clicked
    {
        bool clicked_left = ImGui::IsMouseClicked(0);
        bool clicked_right = !clicked_left && ImGui::IsMouseClicked(1);
        if(clicked_left || clicked_right) {
            info.clicked = true;
            info.btn = clicked_left ? MouseButton::LEFT : MouseButton::RIGHT;
        }
    }

    // Process mouse released
    {
        bool released_left = ImGui::IsMouseReleased(0);
        bool released_right = !released_left && ImGui::IsMouseReleased(1);
        if(released_left || released_right) {
            info.released = true;
            info.btn = released_left ? MouseButton::LEFT : MouseButton::RIGHT;
        }
    }

    notify_mouse_listeners(info);
}

void InputEventNotifier::notify_mouse_listeners(const MouseEventInfo& info) {
    for(auto i = 0; i < m_mouse_cbs.size(); ++i) {
        bool any_input = info.clicked || info.released ||
                         info.dragged || info.wheel_changed;

        bool notify_anyways = m_include_mouse_move.at(i);

        if(any_input || notify_anyways)
            m_mouse_cbs.at(i)(info);
    }
}

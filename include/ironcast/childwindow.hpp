#pragma once

#include "ironcast/widget.hpp"

namespace ironcast {
    // An ImGui child window at this widget's Yoga box. Children are ordinary Yoga children,
    // laid out inside its padding; the layout is an origin, so their positions are relative to
    // the child window, which is what SetCursorPos inside BeginChild expects.
    // Drawn above the parent window's other items and clipped to the parent window.
    // Scrolls when given a size (width/height, flex, max_*) smaller than its children.
    // The Yoga border is managed here (border line + scrollbars); use padding for spacing.
    class ChildWindow : public Widget {
        ImGuiChildFlags childFlags_ = ImGuiChildFlags_None;
        ImGuiWindowFlags windowFlags_ = ImGuiWindowFlags_None;

    public:
        ChildWindow(const std::string& title = "", Widget* parent = nullptr);

        ChildWindow& set_child_flags(ImGuiChildFlags flags);
        [[nodiscard]] ImGuiChildFlags child_flags() const;

        ChildWindow& set_window_flags(ImGuiWindowFlags flags);
        [[nodiscard]] ImGuiWindowFlags window_flags() const;

        bool show() override;
    };
}

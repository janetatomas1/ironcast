#pragma once

#include "ironcast/widget.hpp"

namespace ironcast {
    // An ImGui popup at this widget's Yoga box. It is absolutely positioned, so it takes no space
    // in its parent, and drawn above everything. Clicking outside it or pressing Escape closes it.
    // Children are ordinary Yoga children, laid out inside its padding; the layout is an origin,
    // so their positions are relative to the popup. The Yoga border is managed here (title bar,
    // if any, + border line); use padding for spacing.
    // The popup is identified by title_id(), so renaming it closes it.
    class Popup : public Widget {
        // Yoga owns the box: moving or resizing would be undone the next frame.
        ImGuiWindowFlags flags_ = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        // Wanted state; show() closes the ImGui popup to match it, and clears it when ImGui
        // closes the popup itself.
        bool open_ = false;
        // Set by open() until show() calls OpenPopup. Kept apart from open_: ImGui closes a
        // popup (click outside) after show() ran, so the next frame open_ is still true while
        // the popup is closed, which would otherwise look like a request to open it.
        bool openRequested_ = false;

    protected:
        // The ImGui Begin call for this kind of popup; returns whether it is showing.
        virtual bool begin(ImGuiWindowFlags flags);

    public:
        Popup(const std::string& title = "", Widget* parent = nullptr);

        Popup& set_window_flags(ImGuiWindowFlags flags);
        [[nodiscard]] ImGuiWindowFlags flags() const;

        // Take effect on the next show().
        void open();
        void close();
        [[nodiscard]] bool is_open() const;

        bool show() override;
    };
}

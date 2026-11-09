#pragma once

#include <string>

#include <function2/function2.hpp>

#include "ironcast/sdlgpu/renderwidget.hpp"

namespace ironcast::sdlgpu {
    // A button showing texture(), drawn at this widget's Yoga box (frame padding included).
    // It has no intrinsic size. title is only its ImGui id; it is not shown.
    class ImageButton : public RenderWidget {
        fu2::unique_function<void()> callback_;

    public:
        ImageButton(const std::string& title, Widget* parent = nullptr);
        ImageButton(const std::string& title, fu2::unique_function<void()> callback, Widget* parent = nullptr);

        void set_callback(fu2::unique_function<void()> callback);
        bool show() final;
    };
}

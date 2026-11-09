#pragma once

#include "ironcast/gpubackend.hpp"

namespace ironcast {
    // Backend without a graphics API: a plain SDL window and ImGui context per window.
    // Frames are built (layout, widgets, events) but nothing is drawn or presented.
    class NullBackend: public GPUBackend {
    public:
        NullBackend(const WindowConfig &config = {});
        void init() override;
        void destroy() override;
        void update(Widget *rootWidget) override;
    };
}

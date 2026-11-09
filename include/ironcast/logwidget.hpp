#pragma once

#include <cstdint>
#include <vector>

#include "ironcast/button.hpp"
#include "ironcast/log.hpp"
#include "ironcast/widget.hpp"

namespace ironcast {
    // Shows the messages kept by log_memory() (LogConfig::memoryCapacity), from the start of the app: a toolbar
    // with a minimum level, a text filter, Clear and Copy, above a scrolling list coloured by level. It follows
    // new messages while scrolled to the bottom. The list says so when the buffer is off.
    // This widget has no intrinsic size: give it one (width/height, flex_grow).
    class LogWidget : public Widget {
        // Level combo and text filter, drawn with ImGui; fills the toolbar left of the buttons.
        struct Filter : Widget {
            LogWidget& log;

            Filter(LogWidget& log, Widget* parent);
            bool show() override;
            ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) override;
        };

        // The scrolling list; it has no intrinsic size.
        struct Rows : Widget {
            LogWidget& log;

            Rows(LogWidget& log, Widget* parent);
            bool show() override;
        };

        std::vector<LogEntry> entries_;
        // Indices into entries_ passing the level and the filter.
        std::vector<int> visible_;
        std::uint64_t version_ = UINT64_MAX;
        spdlog::level::level_enum minLevel_ = spdlog::level::trace;
        ImGuiTextFilter filter_;
        bool refilter_ = true;

        Widget toolbar_ = Widget("", this);
        Filter filterBar_ = Filter(*this, &toolbar_);
        Button clear_;
        Button copy_;
        Rows rows_ = Rows(*this, this);

        // Copies the buffer when it changed and rebuilds visible_ when anything did.
        void refresh(MemorySink& memory);
        // Puts the shown lines on the clipboard.
        void copy_visible() const;

    public:
        LogWidget(Widget* parent = nullptr);

        // Messages below it are hidden.
        [[nodiscard]] spdlog::level::level_enum min_level() const;
        LogWidget& set_min_level(spdlog::level::level_enum level);
    };
}

#include <string>

#include "ironcast/clipboard.hpp"
#include "ironcast/logwidget.hpp"

namespace ironcast {
    namespace {
        ImVec4 level_color(spdlog::level::level_enum level) {
            switch (level) {
                case spdlog::level::trace:
                case spdlog::level::debug: return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                case spdlog::level::warn: return {1.0f, 0.78f, 0.25f, 1.0f};
                case spdlog::level::err:
                case spdlog::level::critical: return {1.0f, 0.35f, 0.35f, 1.0f};
                default: return ImGui::GetStyleColorVec4(ImGuiCol_Text);
            }
        }
    }

    LogWidget::Filter::Filter(LogWidget& log, Widget* parent): Widget("", parent), log(log) {
        layout().enable_measure();
    }

    bool LogWidget::Filter::show() {
        static constexpr const char* levels[] = {"trace", "debug", "info", "warn", "error", "critical"};
        const float comboWidth = ImGui::GetFontSize() * 7.0f;
        int level = static_cast<int>(log.minLevel_);
        bool changed = false;

        ImGui::PushID(static_cast<int>(id()));
        ImGui::SetNextItemWidth(comboWidth);

        if (ImGui::Combo("##level", &level, levels, IM_ARRAYSIZE(levels))) {
            log.set_min_level(static_cast<spdlog::level::level_enum>(level));
            changed = true;
        }
        ImGui::SameLine();

        if (log.filter_.Draw("##filter", std::max(width() - comboWidth - ImGui::GetStyle().ItemSpacing.x, 1.0f))) {
            log.refilter_ = true;
            changed = true;
        }
        ImGui::PopID();
        return changed;
    }

    ImVec2 LogWidget::Filter::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        return measure_field(0.0f, width, widthMode, height, heightMode);
    }

    LogWidget::Rows::Rows(LogWidget& log, Widget* parent): Widget("", parent), log(log) {}

    bool LogWidget::Rows::show() {
        const auto memory = log_memory();
        // id(), not the title: renaming must not reset the scroll position.
        ImGui::BeginChild(static_cast<ImGuiID>(id()), size(), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        if (memory == nullptr) {
            ImGui::TextDisabled("Log buffer is off (LogConfig::memoryCapacity = 0)");
            ImGui::EndChild();
            return false;
        }
        log.refresh(*memory);

        // Read before adding rows: following only while the user is at the bottom.
        const bool follow = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(log.visible_.size()));

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                const LogEntry& entry = log.entries_[log.visible_[row]];
                ImGui::PushStyleColor(ImGuiCol_Text, level_color(entry.level));
                ImGui::TextUnformatted(entry.line.c_str(), entry.line.c_str() + entry.line.size());
                ImGui::PopStyleColor();
            }
        }
        clipper.End();

        if (follow) {
            ImGui::SetScrollHereY(1.0f);
        }
        // Unlike most Begin/End pairs, EndChild is required even when BeginChild returns false.
        ImGui::EndChild();
        return false;
    }

    LogWidget::LogWidget(Widget* parent)
        : Widget("", parent)
        , clear_("Clear", [] {
            if (auto memory = log_memory()) {
                memory->clear();
            }
        }, &toolbar_)
        , copy_("Copy", [this] { copy_visible(); }, &toolbar_) {
        column(4.0f);
        toolbar_.row(8.0f);
        filterBar_.flex_grow(1);
        rows_.flex_grow(1);
        copy_.margin(percent(1), YGEdgeRight);
        copy_.width(percent(4.0f));
        clear_.width(percent(4.0f));
    }

    spdlog::level::level_enum LogWidget::min_level() const {
        return minLevel_;
    }

    LogWidget& LogWidget::set_min_level(spdlog::level::level_enum level) {
        minLevel_ = level;
        refilter_ = true;
        return *this;
    }

    void LogWidget::refresh(MemorySink& memory) {
        if (const auto version = memory.version(); version != version_) {
            memory.copy(entries_);
            version_ = version;
            refilter_ = true;
        }

        if (!refilter_) {
            return;
        }
        visible_.clear();

        for (int i = 0; i < static_cast<int>(entries_.size()); i++) {
            const LogEntry& entry = entries_[i];

            if (entry.level >= minLevel_ && filter_.PassFilter(entry.line.c_str())) {
                visible_.push_back(i);
            }
        }
        refilter_ = false;
    }

    void LogWidget::copy_visible() const {
        std::string text;

        for (const int i: visible_) {
            text += entries_[i].line;
            text += '\n';
        }
        clipboard::set_text(text);
    }
}

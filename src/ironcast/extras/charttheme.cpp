#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>

#include <imgui_internal.h>
#include "ironcast/assert.hpp"

#include "ironcast/extras/charttheme.hpp"

#include "ironcast/ironcast.hpp"
#include "ironcast/window.hpp"

namespace ironcast {
    namespace {
        struct ChartContexts {
            ImPlotContext* plot = nullptr;
            ImPlot3DContext* plot3d = nullptr;
            uint64_t appliedVersion = 0;
        };

        struct ChartState {
            ChartTheme theme;
            uint64_t version = 1;
            // One entry per window, keyed by its ImGui context.
            std::unordered_map<ImGuiContext*, ChartContexts> contexts;
        };

        ChartState& state() {
            static ChartState instance;
            return instance;
        }

        // Called while ImGui shuts the window's context down.
        void destroy_chart_contexts(ImGuiContext* context, ImGuiContextHook*) {
            auto& contexts = state().contexts;
            const auto it = contexts.find(context);
            if (it == contexts.end()) {
                return;
            }
            ImPlot3D::DestroyContext(it->second.plot3d);
            ImPlot::DestroyContext(it->second.plot);
            contexts.erase(it);
        }

        void apply_chart_theme(const ChartTheme& theme, uint64_t version) {
            ImPlot::GetStyle() = theme.plot;
            ImPlot3D::GetStyle() = theme.plot3d;

            if (theme.palette.empty()) {
                return;
            }
            // ImPlot can't replace or remove a colormap, so each theme version registers its own name.
            const std::string name = std::format("ironcast.{}", version);
            const int size = static_cast<int>(theme.palette.size());
            ImPlot::GetStyle().Colormap = ImPlot::AddColormap(name.c_str(), theme.palette.data(), size);
            ImPlot3D::GetStyle().Colormap = ImPlot3D::AddColormap(name.c_str(), theme.palette.data(), size);
        }
    }

    void set_chart_theme(Ironcast& app, const ChartTheme& theme) {
        IRONCAST_DEBUG_ASSERT(theme.palette.size() != 1, "a chart palette needs at least two colours");

        // Deferred: charts drawn earlier in the frame would keep the old style.
        app.add_action([&app, theme]() {
            state().theme = theme;
            ++state().version;

            // PlotDefaultSize changes the charts' measured size.
            for (size_t i = 0; i < app.window_count(); ++i) {
                app.get_window_at(i)->root_widget()->layout().mark_dirty_tree();
            }
            app.request_redraw();
        });
    }

    const ChartTheme& chart_theme() {
        return state().theme;
    }

    namespace detail {
        void make_chart_contexts_current() {
            ImGuiContext* context = ImGui::GetCurrentContext();
            IRONCAST_DEBUG_ASSERT(context != nullptr, "charts drawn with no imgui context");

            ChartState& chart = state();
            auto [it, created] = chart.contexts.try_emplace(context);
            ChartContexts& contexts = it->second;

            if (created) {
                contexts.plot = ImPlot::CreateContext();
                contexts.plot3d = ImPlot3D::CreateContext();

                ImGuiContextHook hook;
                hook.Type = ImGuiContextHookType_Shutdown;
                hook.Callback = destroy_chart_contexts;
                ImGui::AddContextHook(context, &hook);
            }
            ImPlot::SetCurrentContext(contexts.plot);
            ImPlot3D::SetCurrentContext(contexts.plot3d);

            if (contexts.appliedVersion != chart.version) {
                apply_chart_theme(chart.theme, chart.version);
                contexts.appliedVersion = chart.version;
            }
        }
    }
}

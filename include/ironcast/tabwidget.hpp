#pragma once

#include <memory>
#include <string>

#include "ironcast/assert.hpp"

#include "ironcast/container.hpp"

namespace ironcast {
    // Pages with a tabbar above them, one tab per page. The strip is the first Yoga
    // child, the pages follow; only the selected page is displayed, the others get
    // display(YGDisplayNone), so Yoga gives them no space.
    // Container is protected: pages are added through add_tab(), which parents them to the widget.
    class TabWidget : public Widget, protected Container<Widget> {
        struct TabBar: Widget {
            ImGuiTabBarFlags flags_ = ImGuiTabBarFlags_None;
            bool stretch_ = true;
            TabWidget *tabWidget;
            size_t selected_ = 0;
            // The tab ImGui showed last frame, to tell its own switches from set_selected(). SIZE_MAX until
            // ImGui shows selected_, so the first page is pushed too.
            size_t shown_ = SIZE_MAX;
            std::string groupID;

            TabBar(TabWidget* parent);
            bool show() override;
            ImVec2 measure(
                float width,
                YGMeasureMode widthMode,
                float height,
                YGMeasureMode heightMode
            ) override;
        };

        TabBar tabBar = TabBar(this);

        // Shows page index and hides the previously selected one.
        void display_page(size_t index);
        // Queues the page to be parented and appended on the next dispatch.
        void attach_page(std::unique_ptr<Widget> page);

    public:
        using Container::count;
        using Container::at;
        using Container::items;

        TabWidget(Widget* parent = nullptr);

        // Adds a page; its title() is the tab label. The page is built now, but joins the tabs
        // on the next dispatch (count() doesn't include it until then). The returned reference
        // stays valid until the page is removed or the TabWidget is destroyed.
        template <WidgetConcept W = Widget, class... Args>
        W& add_tab(Args&&... args) {
            auto page = std::make_unique<W>(std::forward<Args>(args)...);
            auto& ref = *page;
            attach_page(std::move(page));
            return ref;
        }

        // Same as above for a page built by the caller; takes ownership of it.
        template <WidgetConcept W>
        W& add_tab(std::unique_ptr<W> page) {
            IRONCAST_DEBUG_ASSERT(page != nullptr, "TabWidget::add_tab called with null");
            auto& ref = *page;
            attach_page(std::move(page));
            return ref;
        }

        // Removes and destroys the page at index on the next dispatch. The index is resolved
        // then, so queued adds and removes apply in call order.
        void remove_tab(size_t index);

        [[nodiscard]] size_t selected() const;
        TabWidget& set_selected(size_t index);

        TabWidget& set_flags(ImGuiTabBarFlags flags);
        [[nodiscard]] ImGuiTabBarFlags flags() const;

        // When true (the default), the tabs share the bar's width equally; with too many tabs the
        // share drops below the labels' width and ImGui shortens them with "...". When false, tabs
        // have their natural width.
        TabWidget& set_stretch(bool stretch);
        [[nodiscard]] bool stretch() const;

        bool show() override;
    };
}

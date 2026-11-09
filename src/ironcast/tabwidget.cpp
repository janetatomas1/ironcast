#include <algorithm>

#include <imgui_internal.h>
#include "ironcast/assert.hpp"

#include "ironcast/tabwidget.hpp"
#include "ironcast/ironcast.hpp"

using namespace ironcast::literals;

namespace ironcast {
    TabWidget::TabBar::TabBar(TabWidget* parent): Widget("", parent), tabWidget(parent) {
        layout().enable_measure();
    }

    bool TabWidget::TabBar::show() {
        bool changed = false;
        if (ImGui::BeginTabBar(title_id().c_str(), flags_)){
            // With stretch_, the tabs share the bar's width equally. With too many tabs the share
            // drops below the labels' width and ImGui shortens them with "...".
            const ImGuiStyle& style = ImGui::GetStyle();
            const float count = static_cast<float>(tabWidget->count());
            float available = width() - style.ItemInnerSpacing.x * (count - 1.0f);
            if (flags_ & ImGuiTabBarFlags_TabListPopupButton) {
                available -= ImGui::GetFontSize() + style.FramePadding.y;
            }
            const float tabWidth = std::max(available / count, 1.0f);

            auto tabs = tabWidget->items();
            size_t shownNow = shown_;
            size_t index = 0;
            for (const auto& tab : tabs) {
                // ImGui is told about selected_ until it shows it, so set_selected() needs nothing else
                // to switch the tab. Not after: it would queue a switch back over ImGui's own switches.
                const ImGuiTabItemFlags itemFlags = index == selected_ && shown_ != selected_
                    ? ImGuiTabItemFlags_SetSelected
                    : ImGuiTabItemFlags_None;

                if (stretch_) {
                    ImGui::SetNextItemWidth(tabWidth);
                }
                if (ImGui::BeginTabItem(tab.title_id().c_str(), nullptr, itemFlags)) {
                    shownNow = index;
                    ImGui::EndTabItem();
                }

                // Deferred: the page boxes for this frame are already laid out, and the switch
                // must land before the next frame passes SetSelected for the old tab.
                if (ImGui::IsItemClicked() && index != selected_) {
                    tabWidget->post([index](auto& self) {
                        self.display_page(index);
                    });
                    changed = true;
                }
                index++;
            }

            // ImGui switched without a click on the tab (tab list popup, keyboard navigation). Only trusted while
            // ImGui and selected_ agreed last frame: after set_selected() ImGui shows the old tab for one more frame.
            if (shownNow != selected_ && shown_ == selected_) {
                tabWidget->post([shownNow](auto& self) {
                    self.display_page(shownNow);
                });
                changed = true;
            }
            shown_ = shownNow;

            ImGui::EndTabBar();
        }

        return changed;
    }

    ImVec2 TabWidget::TabBar::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Intrinsic width is the tabs side by side, as BeginTabBar lays them out without
        // shrinking; tab items are frame height tall.
        float tabsWidth = 0.0f;
        size_t count = 0;
        for (const auto& tab : tabWidget->items()) {
            tabsWidth += ImGui::TabItemCalcSize(tab.title_id().c_str(), false).x;
            ++count;
        }

        if (count > 1) {
            tabsWidth += ImGui::GetStyle().ItemInnerSpacing.x * static_cast<float>(count - 1);
        }

        // The list button sits left of the tabs, at the width TabBarTabListPopupButton() takes.
        // Scroll buttons and resize-down only apply when the tabs don't fit, so they don't count.
        if (flags_ & ImGuiTabBarFlags_TabListPopupButton) {
            tabsWidth += ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y;
        }

        const ImVec2 size{
            fit_measure(tabsWidth, width, widthMode),
            fit_measure(ImGui::GetFrameHeight(), height, heightMode)
        };

        return size;
    }

    TabWidget::TabWidget(Widget* parent): Widget("", parent) {}

    size_t TabWidget::selected() const {
        return tabBar.selected_;
    }

    void TabWidget::display_page(size_t index) {
        if (index == tabBar.selected_) {
            return;
        }

        items()[tabBar.selected_].display(YGDisplayNone);
        tabBar.selected_ = index;
        items()[tabBar.selected_].display(YGDisplayFlex);
    }

    void TabWidget::attach_page(std::unique_ptr<Widget> page) {
        post([page = std::move(page)](auto& self) mutable {
            auto& ref = self.adopt(std::move(page));
            ref.set_parent(&self);
            // The first page starts selected; later ones stay hidden until chosen.
            if (self.count() > 1) {
                ref.display(YGDisplayNone);
            }
            // A new tab widens the strip.
            self.tabBar.layout().mark_dirty();
            // Pages added before the window starts are initialized with it.
            if (self.in_window_tree()) {
                ref.init();
            }
        });
    }

    void TabWidget::remove_tab(size_t index) {
        post([index](auto& self) {
            IRONCAST_DEBUG_ASSERT(index < self.count(), "TabWidget::remove_tab index out of range", index, self.count());

            size_t& selected = self.tabBar.selected_;
            const bool wasSelected = index == selected;
            self.remove(index);

            // Keep the same page selected; if it was the removed one, its successor (or the new
            // last page) takes over and has to be shown.
            if (index < selected) {
                --selected;
            } else if (wasSelected && self.count() > 0) {
                selected = std::min(selected, self.count() - 1);
                self.at(selected).display(YGDisplayFlex);
            } else if (self.count() == 0) {
                selected = 0;
            }

            // Indices past the removed page shifted. ImGui still shows the same page unless it was the removed one;
            // then the successor has to be pushed, or ImGui falls back to its most recently selected tab.
            self.tabBar.shown_ = wasSelected ? SIZE_MAX : selected;
            self.tabBar.layout().mark_dirty();
        });
    }

    TabWidget& TabWidget::set_selected(size_t index) {
        IRONCAST_DEBUG_ASSERT(index < count(), "TabWidget::set_selected index out of range", index, count());
        display_page(index);
        return *this;
    }

    TabWidget& TabWidget::set_flags(ImGuiTabBarFlags flags) {
        tabBar.flags_ = flags;
        tabBar.layout().mark_dirty();
        return *this;
    }

    ImGuiTabBarFlags TabWidget::flags() const {
        return tabBar.flags_;
    }

    TabWidget& TabWidget::set_stretch(bool stretch) {
        tabBar.stretch_ = stretch;
        return *this;
    }

    bool TabWidget::stretch() const {
        return tabBar.stretch_;
    }

    bool TabWidget::show() {
        tabBar.render();

        if (count() > 0) {
            items()[tabBar.selected_].render();
        }

        return false;
    }
}

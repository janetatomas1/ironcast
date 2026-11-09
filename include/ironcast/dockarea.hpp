#pragma once

#include <memory>

#include "ironcast/assert.hpp"

#include "ironcast/container.hpp"
#include "ironcast/pane.hpp"

namespace ironcast {
    // An ImGui dock space covering this widget's Yoga box, holding Panes that users can drag,
    // tab together, split and float. The first time it shows, panes are placed by their
    // dock(): split off at that side, or tabbed in the middle. Panes added later join the
    // middle as tabs. The layout isn't saved between runs.
    // This widget has no intrinsic size: give it one (width/height, flex_grow).
    // Container is protected: panes are added through add_pane(), which parents them to the area.
    class DockArea : public Widget, protected Container<Pane> {
        ImGuiDockNodeFlags flags_ = ImGuiDockNodeFlags_None;
        bool built_ = false;

        // Queues the pane to be parented and appended on the next dispatch.
        void attach_pane(std::unique_ptr<Pane> pane);
        // Lays out the starting dock nodes and docks the panes into them.
        void build();

    public:
        using Container::count;
        using Container::at;
        using Container::items;

        DockArea(Widget* parent = nullptr);

        // Adds a pane. It is built now, but joins the area on the next dispatch (count() doesn't
        // include it until then). The returned reference stays valid until the pane is removed
        // or the area is destroyed.
        template <std::derived_from<Pane> P = Pane, class... Args>
        P& add_pane(Args&&... args) {
            auto pane = std::make_unique<P>(std::forward<Args>(args)...);
            auto& ref = *pane;
            attach_pane(std::move(pane));
            return ref;
        }

        // Same as above for a pane built by the caller; takes ownership of it.
        template <std::derived_from<Pane> P>
        P& add_pane(std::unique_ptr<P> pane) {
            IRONCAST_DEBUG_ASSERT(pane != nullptr, "DockArea::add_pane called with null");
            auto& ref = *pane;
            attach_pane(std::move(pane));
            return ref;
        }

        // Removes and destroys the pane at index on the next dispatch. The index is resolved
        // then, so queued adds and removes apply in call order.
        void remove_pane(size_t index);

        // ImGuiDockNodeFlags for the dock space, e.g. ImGuiDockNodeFlags_NoUndocking to keep
        // panes from floating.
        DockArea& set_flags(ImGuiDockNodeFlags flags);
        [[nodiscard]] ImGuiDockNodeFlags flags() const;

        bool show() override;
    };
}

#include <algorithm>

#include <imgui_internal.h>
#include "ironcast/assert.hpp"

#include "ironcast/selectable.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    Selectable::Selectable(
        std::function<void(std::vector<size_t>)> on_change,
        Widget* parent
    ): Widget("", parent),
    onChange_(std::move(on_change)) {
        layout().enable_measure();
    }

    Selectable& Selectable::add_item(const std::string& label) {
        post([label](auto& self) {
            self.labels_.push_back(label);
            self.selected_.push_back(false);
            self.layout().mark_dirty();
        });
        return *this;
    }

    Selectable& Selectable::remove_item(size_t index) {
        post([index](auto& self) {
            IRONCAST_DEBUG_ASSERT(index < self.labels_.size(), "Selectable::remove_item index out of range", index, self.labels_.size());
            self.labels_.erase(self.labels_.begin() + static_cast<std::ptrdiff_t>(index));
            self.selected_.erase(self.selected_.begin() + static_cast<std::ptrdiff_t>(index));
            self.layout().mark_dirty();
        });
        return *this;
    }

    size_t Selectable::count() const {
        return labels_.size();
    }

    const std::string& Selectable::label(size_t index) const {
        IRONCAST_DEBUG_ASSERT(index < labels_.size(), "Selectable::label index out of range", index, labels_.size());
        return labels_[index];
    }

    bool Selectable::is_selected(size_t index) const {
        IRONCAST_DEBUG_ASSERT(index < selected_.size(), "Selectable::is_selected index out of range", index, selected_.size());
        return selected_[index];
    }

    Selectable& Selectable::set_selected(size_t index, bool selected) {
        IRONCAST_DEBUG_ASSERT(index < selected_.size(), "Selectable::set_selected index out of range", index, selected_.size());
        selected_[index] = selected;
        return *this;
    }

    std::vector<size_t> Selectable::selected() const {
        std::vector<size_t> indices;
        for (size_t i = 0; i < selected_.size(); ++i) {
            if (selected_[i]) {
                indices.push_back(i);
            }
        }

        return indices;
    }

    Selectable& Selectable::set_flags(ImGuiMultiSelectFlags flags) {
        flags_ = flags;
        return *this;
    }

    ImGuiMultiSelectFlags Selectable::flags() const {
        return flags_;
    }

    Selectable& Selectable::set_on_change(std::function<void(std::vector<size_t>)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }

    bool Selectable::show() {
        const std::vector<bool> before = selected_;

        // Rows get unique ids under this widget's id, and the multi-select scope is keyed on it.
        ImGui::PushID(title_id().c_str());

        // Requests address rows by index (SetNextItemSelectionUserData), applied straight to selected_.
        ImGuiSelectionExternalStorage storage;
        storage.UserData = this;
        storage.AdapterSetItemSelected = [](ImGuiSelectionExternalStorage* self, int index, bool selected) {
            static_cast<Selectable*>(self->UserData)->selected_[index] = selected;
        };

        const int selectedCount = static_cast<int>(std::ranges::count(selected_, true));
        ImGuiMultiSelectIO* io = ImGui::BeginMultiSelect(flags_, selectedCount, static_cast<int>(count()));
        storage.ApplyRequests(io);

        const float rowHeight = ImGui::GetFontSize();
        for (size_t i = 0; i < count(); ++i) {
            ImGui::SetCursorPos(ImVec2{x(), y() + rowHeight * static_cast<float>(i)});
            ImGui::PushID(static_cast<int>(i));
            ImGui::SetNextItemSelectionUserData(static_cast<ImGuiSelectionUserData>(i));
            // An explicit width, or ImGui spans the row to the window's right edge.
            ImGui::Selectable(
                labels_[i].c_str(),
                selected_[i],
                ImGuiSelectableFlags_NoPadWithHalfSpacing,
                ImVec2{width(), rowHeight}
            );
            ImGui::PopID();
        }

        io = ImGui::EndMultiSelect();
        storage.ApplyRequests(io);
        ImGui::PopID();

        const bool changed = selected_ != before;
        if (changed && onChange_) {
            post([indices = selected()](auto& self) {
                if (self.onChange_) {
                    self.onChange_(indices);
                }
            });
        }

        return changed;
    }

    ImVec2 Selectable::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // The widest visible label; one text line per row, no gaps.
        float labelsWidth = 0.0f;
        for (const auto& label : labels_) {
            labelsWidth = std::max(labelsWidth, ImGui::CalcTextSize(label.c_str(), nullptr, true).x);
        }

        return ImVec2{
            fit_measure(labelsWidth, width, widthMode),
            fit_measure(ImGui::GetFontSize() * static_cast<float>(count()), height, heightMode)
        };
    }
}

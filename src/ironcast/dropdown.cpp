
#include "ironcast/assert.hpp"

#include "ironcast/dropdown.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    Dropdown::Dropdown(const std::string& title, Widget* parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    const char* Dropdown::preview() const {
        return has_selection() ? options_[selected_].c_str() : placeholder_.c_str();
    }

    Dropdown& Dropdown::add_option(const std::string& label) {
        options_.push_back(label);
        return *this;
    }

    Dropdown& Dropdown::insert_option(size_t index, const std::string& label) {
        IRONCAST_DEBUG_ASSERT(index <= options_.size(), "insert_option index out of range", index, options_.size());

        options_.insert(options_.begin() + index, label);

        if (selected_ >= 0 && static_cast<size_t>(selected_) >= index) {
            selected_ += 1;
        }

        return *this;
    }

    void Dropdown::remove_option(size_t index) {
        IRONCAST_DEBUG_ASSERT(index < options_.size(), "remove_option index out of range", index, options_.size());

        options_.erase(options_.begin() + index);

        if (selected_ == static_cast<int>(index)) {
            selected_ = -1;
        } else if (selected_ > static_cast<int>(index)) {
            selected_ -= 1;
        }

        layout().mark_dirty();
    }

    void Dropdown::clear_options() {
        options_.clear();
        selected_ = -1;
        layout().mark_dirty();
    }

    int Dropdown::selected() const {
        return selected_;
    }

    void Dropdown::set_selected(int index) {
        IRONCAST_DEBUG_ASSERT(index == -1 || (index >= 0 && static_cast<size_t>(index) < options_.size()),
                     "Dropdown::set_selected index out of range", index, options_.size());
        selected_ = index;
        layout().mark_dirty();
    }

    const std::string& Dropdown::selected_label() const {
        IRONCAST_DEBUG_ASSERT(has_selection(), "selected_label called with no selection");
        return options_[selected_];
    }

    bool Dropdown::has_selection() const {
        return selected_ >= 0 && static_cast<size_t>(selected_) < options_.size();
    }

    size_t Dropdown::count() const {
        return options_.size();
    }

    const std::string& Dropdown::at(size_t index) const {
        IRONCAST_DEBUG_ASSERT(index < options_.size(), "option_at index out of range", index, options_.size());
        return options_[index];
    }

    Dropdown& Dropdown::set_placeholder(const std::string& text) {
        placeholder_ = text;
        layout().mark_dirty();
        return *this;
    }

    const std::string& Dropdown::placeholder() const {
        return placeholder_;
    }

    DropdownFlags Dropdown::flags() const {
        return flags_;
    }

    Dropdown& Dropdown::set_flags(DropdownFlags flags) {
        flags_ = flags;
        layout().mark_dirty();
        return *this;
    }

    Dropdown& Dropdown::set_on_change(std::function<void(int)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }

    bool Dropdown::show() {
        bool changed = false;

        // NoPreview and WidthFitPreview size the combo themselves and ignore the item width.
        set_next_field_width(label_extent());

        // Popped before the items, so only the combo frame takes the height, not the popup.
        const bool pushed = push_frame_height();
        const bool open = ImGui::BeginCombo(title_id().c_str(), preview(), flags_);
        pop_frame_height(pushed);

        if (open) {
            for (size_t i = 0; i < options_.size(); ++i) {
                bool isSelected = (static_cast<int>(i) == selected_);

                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(options_[i].c_str(), isSelected)) {
                    if (selected_ != static_cast<int>(i)) {
                        selected_ = static_cast<int>(i);
                        changed = true;
                    }
                }
                ImGui::PopID();

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (changed) {
            // WidthFitPreview measures the selected label; picked up on the next layout.
            layout().mark_dirty();

            if (onChange_) {
                post([selected = selected_](auto& self) {
                    if (self.onChange_) {
                        self.onChange_(selected);
                    }
                });
            }
        }

        return changed;
    }

    ImVec2 Dropdown::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Mirrors ImGui::BeginCombo's frame width; the label follows after ItemInnerSpacing.
        const float arrow = (flags_ & ImGuiComboFlags_NoArrowButton) ? 0.0f : frame_height(height, heightMode);
        float frame;

        if (flags_ & ImGuiComboFlags_NoPreview) {
            frame = arrow;
        } else if (flags_ & ImGuiComboFlags_WidthFitPreview) {
            const float text = ImGui::CalcTextSize(preview(), nullptr, true).x;
            frame = arrow + text + ImGui::GetStyle().FramePadding.x * 2.0f;
        } else {
            frame = default_field_width();
        }

        return ImVec2{
            fit_measure(frame + label_extent(), width, widthMode),
            fit_measure(ImGui::GetFrameHeight(), height, heightMode)
        };
    }
}

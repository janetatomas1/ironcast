#include "ironcast/modal.hpp"

namespace ironcast {
    Modal::Modal(const std::string& title, Widget* parent): Popup(title, parent) {}

    bool Modal::begin(ImGuiWindowFlags flags) {
        return ImGui::BeginPopupModal(title_id().c_str(), nullptr, flags);
    }
}

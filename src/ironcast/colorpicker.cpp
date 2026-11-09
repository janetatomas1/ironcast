#include "ironcast/colorpicker.hpp"

namespace ironcast::detail {
    ImGuiColorEditFlags color_edit_options() {
        return ImGui::GetIO().ConfigColorEditFlags;
    }
}

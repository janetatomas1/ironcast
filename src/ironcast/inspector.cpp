#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>
#include <typeinfo>
#include <utility>

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#endif

#include <imgui.h>

#include "ironcast/inspector.hpp"
#include "ironcast/widget.hpp"

namespace ironcast {
    namespace {
        std::string type_name(const Widget& widget) {
            const char* name = typeid(widget).name();
#if __has_include(<cxxabi.h>)
            int status = 0;
            if (char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status)) {
                std::string result = demangled;
                std::free(demangled);
                return result;
            }
#endif
            return name;
        }

        std::string to_string(LayoutValue value) {
            switch (value.unit) {
                case YGUnitPoint:
                    return std::format("{}", value.value);
                case YGUnitPercent:
                    return std::format("{}%", value.value);
                default:
                    return YGUnitToString(value.unit);
            }
        }

        // CSS order: top right bottom left.
        template <class F>
        std::string edges(F get) {
            return std::format("{} {} {} {}", get(YGEdgeTop), get(YGEdgeRight), get(YGEdgeBottom), get(YGEdgeLeft));
        }

        constexpr std::array<std::pair<const char*, YGEdge>, 5> edgeNames{{
            {"all", YGEdgeAll},
            {"top", YGEdgeTop},
            {"right", YGEdgeRight},
            {"bottom", YGEdgeBottom},
            {"left", YGEdgeLeft},
        }};

        // Starts a row with its name; the caller's control goes in the value column, full width.
        void label(const char* name, bool indent = false) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (indent) {
                ImGui::Indent();
            }
            ImGui::TextDisabled("%s", name);
            if (indent) {
                ImGui::Unindent();
            }
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
        }

        void row(const char* name, const std::string& value) {
            label(name);
            ImGui::TextUnformatted(value.c_str());
        }

        // Unset (NaN) shows as 0; the value is only written back once it is dragged.
        bool edit_float(const char* name, float& value, float min = -FLT_MAX) {
            label(name);
            if (std::isnan(value)) {
                value = 0.0f;
            }
            ImGui::PushID(name);
            const bool changed = ImGui::DragFloat("##value", &value, 0.1f, min, FLT_MAX);
            ImGui::PopID();
            return changed;
        }

        const char* unit_name(YGUnit unit) {
            switch (unit) {
                case YGUnitUndefined: return "unset";
                case YGUnitPoint: return "pt";
                case YGUnitPercent: return "%";
                default: return YGUnitToString(unit);
            }
        }

        // Number plus unit. Picking a unit for an unset value starts it at 0.
        bool edit_value(const char* name, LayoutValue& value, bool allowAuto, bool indent = false) {
            label(name, indent);
            ImGui::PushID(name);
            bool changed = false;

            const ImGuiStyle& style = ImGui::GetStyle();
            const float unitWidth = ImGui::CalcTextSize("unset").x + ImGui::GetFrameHeight() + style.FramePadding.x * 2.0f;
            if (value.unit == YGUnitPoint || value.unit == YGUnitPercent) {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - unitWidth - style.ItemInnerSpacing.x);
                changed = ImGui::DragFloat("##value", &value.value, 0.5f);
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
            }

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##unit", unit_name(value.unit))) {
                for (const YGUnit unit : {YGUnitPoint, YGUnitPercent, YGUnitAuto}) {
                    if (unit == YGUnitAuto && !allowAuto) {
                        continue;
                    }
                    if (ImGui::Selectable(unit_name(unit), unit == value.unit) && unit != value.unit) {
                        value.unit = unit;
                        if (std::isnan(value.value)) {
                            value.value = 0.0f;
                        }
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::PopID();
            return changed;
        }

        // Lists every value of a Yoga enum: its ToString() returns "unknown" past the last one.
        template <class E>
        bool edit_enum(const char* name, E& value, const char* (*to_string)(E)) {
            label(name);
            ImGui::PushID(name);
            bool changed = false;
            if (ImGui::BeginCombo("##value", to_string(value))) {
                for (int i = 0; std::strcmp(to_string(static_cast<E>(i)), "unknown") != 0; ++i) {
                    const E option = static_cast<E>(i);
                    if (ImGui::Selectable(to_string(option), option == value)) {
                        value = option;
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            return changed;
        }

        ImVec2 inset(ImVec2 point, float x, float y) {
            return {point.x + x, point.y + y};
        }

        // Margin, border, padding and content boxes, colored like browser devtools. The border box
        // is the widget's Yoga box; the outline alone is drawn for a widget that is only hovered.
        void draw_box(Widget& widget, bool full) {
            ImDrawList* list = ImGui::GetForegroundDrawList();
            const ImVec2 min = widget.screen_position();
            const ImVec2 max = inset(min, widget.width(), widget.height());

            if (!full) {
                list->AddRect(min, max, IM_COL32(120, 170, 255, 255));
                return;
            }

            const auto border = [&](YGEdge edge) { return widget.layout_border(edge); };
            const auto padding = [&](YGEdge edge) { return widget.layout_padding(edge) + border(edge); };

            const ImVec2 marginMin = inset(min, -widget.layout_margin(YGEdgeLeft), -widget.layout_margin(YGEdgeTop));
            const ImVec2 marginMax = inset(max, widget.layout_margin(YGEdgeRight), widget.layout_margin(YGEdgeBottom));
            const ImVec2 paddingMin = inset(min, border(YGEdgeLeft), border(YGEdgeTop));
            const ImVec2 paddingMax = inset(max, -border(YGEdgeRight), -border(YGEdgeBottom));
            const ImVec2 contentMin = inset(min, padding(YGEdgeLeft), padding(YGEdgeTop));
            const ImVec2 contentMax = inset(max, -padding(YGEdgeRight), -padding(YGEdgeBottom));

            list->AddRectFilled(contentMin, contentMax, IM_COL32(110, 160, 220, 70));
            list->AddRect(marginMin, marginMax, IM_COL32(250, 165, 80, 255));
            list->AddRect(min, max, IM_COL32(250, 220, 110, 255));
            list->AddRect(paddingMin, paddingMax, IM_COL32(150, 200, 120, 255));
            list->AddRect(contentMin, contentMax, IM_COL32(110, 160, 220, 255));
        }
    }

    void Inspector::tree(Widget& widget, bool root) {
        Layout& layout = widget.layout();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (root) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        if (layout.child_count() == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if (widget.id() == selected_) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        // Hidden widgets (display none) are greyed out.
        const bool visible = layout.visible();
        if (!visible) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }
        const bool open = ImGui::TreeNodeEx(
            std::to_string(widget.id()).c_str(),
            flags,
            "%s: %s",
            type_name(widget).c_str(),
            widget.title().c_str()
        );
        if (!visible) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selected_ = widget.id();
        }
        if (ImGui::IsItemHovered()) {
            hovered_ = widget.id();
        }

        if (open) {
            for (size_t i = 0; i < layout.child_count(); ++i) {
                if (Widget* child = layout.child(i)->owner()) {
                    tree(*child, false);
                }
            }
            ImGui::TreePop();
        }
    }

    void Inspector::properties(Widget& widget) {
        ImGui::TextUnformatted(type_name(widget).c_str());
        ImGui::TextDisabled("id %llu", static_cast<unsigned long long>(widget.id()));

        if (!ImGui::BeginTable("properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
            return;
        }
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

        Layout& layout = widget.layout();

        // Applied on Enter: a new title changes the widget's ImGui id, and with it its ImGui state.
        std::array<char, 256> title{};
        widget.title().copy(title.data(), title.size() - 1);
        label("title");
        if (ImGui::InputText("##title", title.data(), title.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            widget.set_title(title.data());
        }

        bool enabled = widget.enabled();
        label("enabled");
        if (ImGui::Checkbox("##enabled", &enabled)) {
            widget.set_enabled(enabled);
        }

        // Computed by Yoga, so read-only.
        row("visible", layout.visible() ? "yes" : "no");
        row("position", std::format("{}, {}", widget.x(), widget.y()));
        row("size", std::format("{} x {}", widget.width(), widget.height()));
        row("screen", std::format("{}, {}", widget.screen_position().x, widget.screen_position().y));

        // Each box: the computed edges, then what is set on each edge, editable.
        row("margin", edges([&](YGEdge edge) { return widget.layout_margin(edge); }));
        ImGui::PushID("margin");
        for (const auto& [name, edge] : edgeNames) {
            LayoutValue value = layout.style_margin(edge);
            if (edit_value(name, value, true, true)) {
                widget.margin(value, edge);
            }
        }
        ImGui::PopID();

        row("border", edges([&](YGEdge edge) { return widget.layout_border(edge); }));
        ImGui::PushID("border");
        for (const auto& [name, edge] : edgeNames) {
            float value = layout.style_border(edge);
            label(name, true);
            if (std::isnan(value)) {
                value = 0.0f;
            }
            ImGui::PushID(name);
            if (ImGui::DragFloat("##value", &value, 0.1f, 0.0f, FLT_MAX)) {
                widget.border(value, edge);
            }
            ImGui::PopID();
        }
        ImGui::PopID();

        row("padding", edges([&](YGEdge edge) { return widget.layout_padding(edge); }));
        ImGui::PushID("padding");
        for (const auto& [name, edge] : edgeNames) {
            LayoutValue value = layout.style_padding(edge);
            if (edit_value(name, value, false, true)) {
                widget.padding(value, edge);
            }
        }
        ImGui::PopID();

        LayoutValue value = widget.min_width();
        if (edit_value("min width", value, false)) {
            widget.min_width(value);
        }
        value = widget.min_height();
        if (edit_value("min height", value, false)) {
            widget.min_height(value);
        }
        value = widget.max_width();
        if (edit_value("max width", value, false)) {
            widget.max_width(value);
        }
        value = widget.max_height();
        if (edit_value("max height", value, false)) {
            widget.max_height(value);
        }

        if (YGFlexDirection direction = widget.flex_direction(); edit_enum("flex direction", direction, YGFlexDirectionToString)) {
            widget.flex_direction(direction);
        }
        if (float grow = widget.flex_grow(); edit_float("flex grow", grow, 0.0f)) {
            widget.flex_grow(grow);
        }
        if (float shrink = widget.flex_shrink(); edit_float("flex shrink", shrink, 0.0f)) {
            widget.flex_shrink(shrink);
        }
        value = widget.flex_basis();
        if (edit_value("flex basis", value, true)) {
            widget.flex_basis(value);
        }
        if (YGWrap wrap = widget.flex_wrap(); edit_enum("flex wrap", wrap, YGWrapToString)) {
            widget.flex_wrap(wrap);
        }
        if (YGJustify justify = widget.justify_content(); edit_enum("justify content", justify, YGJustifyToString)) {
            widget.justify_content(justify);
        }
        if (YGAlign align = widget.align_items(); edit_enum("align items", align, YGAlignToString)) {
            widget.align_items(align);
        }
        if (YGAlign align = widget.align_self(); edit_enum("align self", align, YGAlignToString)) {
            widget.align_self(align);
        }
        if (YGAlign align = widget.align_content(); edit_enum("align content", align, YGAlignToString)) {
            widget.align_content(align);
        }
        if (float gap = widget.gap(); edit_float("gap", gap, 0.0f)) {
            widget.gap(gap);
        }
        if (YGDisplay display = widget.display(); edit_enum("display", display, YGDisplayToString)) {
            widget.display(display);
        }
        if (YGOverflow overflow = widget.overflow(); edit_enum("overflow", overflow, YGOverflowToString)) {
            widget.overflow(overflow);
        }
        if (YGPositionType type = widget.position_type(); edit_enum("position type", type, YGPositionTypeToString)) {
            widget.position_type(type);
        }
        if (float ratio = widget.aspect_ratio(); edit_float("aspect ratio", ratio, 0.0f)) {
            widget.aspect_ratio(ratio);
        }

        ImGui::EndTable();
    }

    bool Inspector::update(Widget& root) {
        bool open = true;
        hovered_ = 0;

        ImGui::SetNextWindowSize({380.0f, 520.0f}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Inspector", &open)) {
            const ImVec2 treeSize{0.0f, ImGui::GetContentRegionAvail().y * 0.5f};
            if (ImGui::BeginChild("tree", treeSize, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY)) {
                tree(root, true);
            }
            ImGui::EndChild();

            if (Widget* selected = Widget::find(selected_)) {
                properties(*selected);
            } else {
                ImGui::TextDisabled("Select a widget in the tree.");
            }
        }
        ImGui::End();

        if (Widget* selected = Widget::find(selected_)) {
            draw_box(*selected, true);
        }
        if (hovered_ != selected_) {
            if (Widget* hovered = Widget::find(hovered_)) {
                draw_box(*hovered, false);
            }
        }

        return open;
    }
}

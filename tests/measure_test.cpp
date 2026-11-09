// Checks each leaf widget's measure() against what ImGui actually draws, without a window.
// Yoga sizes the widget through measure(); render() then draws it, and GetItemRectSize() is
// the size ImGui laid out. Group widgets report the whole group's rect after EndGroup.

#include <gtest/gtest.h>

#include <functional>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>

#include <ironcast/button.hpp>
#include <ironcast/checkbox.hpp>
#include <ironcast/coloredit.hpp>
#include <ironcast/colorpicker.hpp>
#include <ironcast/drag.hpp>
#include <ironcast/dropdown.hpp>
#include <ironcast/input.hpp>
#include <ironcast/progressbar.hpp>
#include <ironcast/separator.hpp>
#include <ironcast/radiobutton.hpp>
#include <ironcast/radiogroup.hpp>
#include <ironcast/slider.hpp>
#include <ironcast/text.hpp>
#include <ironcast/textinput.hpp>
#include <ironcast/table.hpp>
#include <ironcast/menubar.hpp>
#include <ironcast/nullbackend.hpp>
#include <ironcast/window.hpp>
#include <ironcast/treenode.hpp>
#include <ironcast/treeview.hpp>
#include <ironcast/widget.hpp>
#include <ironcast/windowconfig.hpp>

using namespace ironcast;

namespace {
    constexpr float displayWidth = 1280.0f;
    constexpr float displayHeight = 720.0f;
    // Yoga rounds boxes to whole pixels; ImGui doesn't.
    constexpr float tolerance = 1.0f;

    class Measure : public ::testing::Test {
    protected:
        // Leaves keep their intrinsic width instead of stretching to the column.
        Widget root;

        void SetUp() override {
            ImGui::CreateContext();

            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.DisplaySize = ImVec2{displayWidth, displayHeight};
            io.DeltaTime = 1.0f / 60.0f;
            // Like ImGui's null renderer: the atlas builds on demand, and texture requests are
            // acknowledged after each frame without uploading anything.
            io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;

            root.column(4.0f).align_items(YGAlignFlexStart);
        }

        void TearDown() override {
            ImGui::DestroyContext();
        }

        // One frame as opengl::Backend::update runs it: layout inside the frame, then draw.
        void frame(const std::function<void()>& draw) {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2{0.0f, 0.0f});
            ImGui::SetNextWindowSize(ImVec2{displayWidth, displayHeight});
            ImGui::Begin("measure", nullptr, WindowConfig{}.windowFlags);

            root.layout().calculate_layout(displayWidth, displayHeight);
            draw();

            ImGui::End();
            ImGui::Render();

            if (auto textures = ImGui::GetDrawData()->Textures; textures != nullptr) {
                for (ImTextureData* texture : *textures) {
                    if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates) {
                        texture->SetStatus(ImTextureStatus_OK);
                    } else if (texture->Status == ImTextureStatus_WantDestroy) {
                        texture->SetStatus(ImTextureStatus_Destroyed);
                    }
                }
            }
        }

        // Yoga's box and ImGui's drawn rect for one widget.
        struct Sizes {
            ImVec2 layout;
            ImVec2 drawn;
        };

        Sizes sizes(Widget& widget) {
            Sizes result{};
            frame([&] {
                widget.render();
                result.layout = widget.size();
                result.drawn = ImGui::GetItemRectSize();
            });
            return result;
        }

        void expect_matches(Widget& widget) {
            const Sizes s = sizes(widget);
            EXPECT_GT(s.layout.x, 0.0f) << widget.title();
            EXPECT_GT(s.layout.y, 0.0f) << widget.title();
            EXPECT_NEAR(s.layout.x, s.drawn.x, tolerance) << widget.title() << " width";
            EXPECT_NEAR(s.layout.y, s.drawn.y, tolerance) << widget.title() << " height";
        }

        // Explicit box on a frame widget: ImGui must draw exactly that box.
        void expect_box(Widget& widget, float width, float height) {
            widget.width(width).height(height);
            const Sizes s = sizes(widget);
            EXPECT_NEAR(s.layout.x, width, tolerance) << widget.title() << " layout width";
            EXPECT_NEAR(s.layout.y, height, tolerance) << widget.title() << " layout height";
            EXPECT_NEAR(s.drawn.x, width, tolerance) << widget.title() << " drawn width";
            EXPECT_NEAR(s.drawn.y, height, tolerance) << widget.title() << " drawn height";
        }
    };
}

// Intrinsic sizes

TEST_F(Measure, Button) {
    Button button("Click me", &root);
    expect_matches(button);
}

TEST_F(Measure, ButtonHiddenLabelSuffix) {
    Button button("Save##toolbar", &root);
    expect_matches(button);
}

TEST_F(Measure, Checkbox) {
    Checkbox checkbox("Enable feature", nullptr, &root);
    expect_matches(checkbox);
}

TEST_F(Measure, RadioButton) {
    int value = 0;
    RadioButton radio("Option", &value, 0, &root);
    expect_matches(radio);
}

TEST_F(Measure, Text) {
    Text text("Hello, world!", &root);
    expect_matches(text);
}

TEST_F(Measure, TextWrapsAtWidth) {
    Text text("The quick brown fox jumps over the lazy dog, again and again and again.", &root);
    text.width(120.0f);
    const Sizes s = sizes(text);
    EXPECT_GT(s.layout.y, ImGui::GetFontSize() * 1.5f) << "expected several lines";
    EXPECT_NEAR(s.layout.y, s.drawn.y, tolerance);
}

TEST_F(Measure, Slider) {
    SliderFloat<> slider("Volume", &root);
    expect_matches(slider);
}

TEST_F(Measure, SliderMultiComponent) {
    SliderInt<3> slider("Color", &root);
    expect_matches(slider);
}

TEST_F(Measure, Drag) {
    DragFloat<2> drag("Position", &root);
    expect_matches(drag);
}

TEST_F(Measure, InputWithStepButtons) {
    InputInt<> input("Count", &root);
    expect_matches(input);
}

TEST_F(Measure, InputWithoutStepButtons) {
    InputFloat<> input("Weight", &root);
    expect_matches(input);
}

TEST_F(Measure, InputMultiComponent) {
    InputFloat<3> input("Offset", &root);
    expect_matches(input);
}

TEST_F(Measure, TextInput) {
    TextInput input("Name", &root);
    expect_matches(input);
}

TEST_F(Measure, TextInputMultiline) {
    TextInput input("Notes", &root);
    input.set_multiline(true);
    expect_matches(input);
}

TEST_F(Measure, Dropdown) {
    Dropdown dropdown("Color", &root);
    dropdown.add_option("Red").add_option("Green");
    expect_matches(dropdown);
}

TEST_F(Measure, DropdownWidthFitPreview) {
    Dropdown dropdown("Color", &root);
    dropdown.add_option("Red").add_option("Dark green");
    dropdown.set_selected(1);
    dropdown.set_flags(ImGuiComboFlags_WidthFitPreview);
    expect_matches(dropdown);
}

TEST_F(Measure, DropdownNoPreview) {
    Dropdown dropdown("Color", &root);
    dropdown.set_flags(ImGuiComboFlags_NoPreview);
    expect_matches(dropdown);
}

TEST_F(Measure, ColorEdit) {
    ColorEdit<> edit("Tint", &root);
    expect_matches(edit);
}

TEST_F(Measure, ColorEditNoInputs) {
    ColorEdit<3> edit("Swatch", &root);
    edit.set_flags(ImGuiColorEditFlags_NoInputs);
    expect_matches(edit);
}

TEST_F(Measure, ColorPicker) {
    ColorPicker<> picker("Accent", &root);
    expect_matches(picker);
}

TEST_F(Measure, ColorPickerRgb) {
    ColorPicker<3> picker("Background", &root);
    expect_matches(picker);
}

TEST_F(Measure, ColorPickerWithReference) {
    ColorPicker<> picker("Accent", &root);
    picker.set_reference({1.0f, 0.0f, 0.0f, 1.0f});
    expect_matches(picker);
}

TEST_F(Measure, ColorPickerAlphaBar) {
    ColorPicker<> picker("Accent", &root);
    picker.set_flags(ImGuiColorEditFlags_AlphaBar);
    expect_matches(picker);
}

TEST_F(Measure, ColorPickerNoSidePreview) {
    ColorPicker<> picker("Accent", &root);
    picker.set_flags(ImGuiColorEditFlags_NoSidePreview);
    expect_matches(picker);
}

TEST_F(Measure, ProgressBar) {
    ProgressBar bar(&root);
    bar.set_fraction(0.5f);
    expect_matches(bar);
}

// Separators stretch across the column (align_self), so the root's width.
TEST_F(Measure, Separator) {
    Separator separator("", &root);
    const Sizes s = sizes(separator);
    EXPECT_NEAR(s.layout.x, displayWidth, tolerance);
    EXPECT_NEAR(s.drawn.x, displayWidth, tolerance);
    EXPECT_NEAR(s.layout.y, s.drawn.y, tolerance);
}

TEST_F(Measure, SeparatorText) {
    Separator separator("Section", &root);
    expect_matches(separator);
}

TEST_F(Measure, SeparatorTextLeftWidth) {
    Separator separator("Section", &root);
    separator.set_left_width(40.0f).width(300.0f);
    expect_matches(separator);
}

TEST_F(Measure, SeparatorExplicitWidth) {
    Separator separator("", &root);
    separator.width(200.0f);
    const Sizes s = sizes(separator);
    EXPECT_NEAR(s.drawn.x, 200.0f, tolerance);
}

TEST_F(Measure, ExplicitBoxProgressBar) {
    ProgressBar bar(&root);
    expect_box(bar, 300.0f, 32.0f);
}

TEST_F(Measure, ExplicitBoxSlider) {
    SliderFloat<> slider("Volume", &root);
    expect_box(slider, 300.0f, 32.0f);
}

TEST_F(Measure, ExplicitBoxDrag) {
    DragInt<2> drag("Range", &root);
    expect_box(drag, 300.0f, 32.0f);
}

TEST_F(Measure, ExplicitBoxInput) {
    InputInt<> input("Count", &root);
    expect_box(input, 300.0f, 32.0f);
}

TEST_F(Measure, ExplicitBoxTextInput) {
    TextInput input("Name", &root);
    expect_box(input, 300.0f, 32.0f);
}

TEST_F(Measure, ExplicitBoxColorEdit) {
    ColorEdit<> edit("Tint", &root);
    expect_box(edit, 300.0f, 32.0f);
}

TEST_F(Measure, ExplicitBoxDropdown) {
    Dropdown dropdown("Color", &root);
    dropdown.add_option("Red");
    expect_box(dropdown, 300.0f, 32.0f);
}

TEST_F(Measure, ExplicitHeightShrinksFrame) {
    SliderFloat<> slider("Volume", &root);
    expect_box(slider, 300.0f, 16.0f);
}

TEST_F(Measure, ExplicitHeightWidensSquares) {
    // With an exact height the swatch and combo arrow are that tall and just as wide.
    ColorEdit<3> swatch("Swatch", &root);
    swatch.set_flags(ImGuiColorEditFlags_NoInputs).height(32.0f);
    expect_matches(swatch);

    Dropdown dropdown("Color", &root);
    dropdown.set_flags(ImGuiComboFlags_NoPreview).height(32.0f);
    expect_matches(dropdown);
}

// TreeNode

TEST_F(Measure, TreeNodeClosedTakesHeaderOnly) {
    TreeNode node("Node", &root);
    Text child("Child", &node);

    // The first frame hides the child; the second lays out without it.
    frame([&] { node.render(); });
    frame([&] { node.render(); });
    EXPECT_FALSE(child.layout().visible());
    EXPECT_NEAR(node.height(), ImGui::GetFontSize(), tolerance);
}

TEST_F(Measure, TreeNodeOpenIndentsChildren) {
    TreeNode node("Node", &root);
    Text child("Child", &node);
    node.set_open(true);

    // The first frame sets the indent; the second lays out with it.
    frame([&] { node.render(); });
    frame([&] { node.render(); });
    EXPECT_TRUE(child.layout().visible());
    EXPECT_NEAR(child.x() - node.x(), ImGui::GetStyle().IndentSpacing, tolerance);
    EXPECT_NEAR(node.height(), ImGui::GetFontSize() + child.height(), tolerance);
}

TEST_F(Measure, TreeNodeFramedHeaderFillsBox) {
    TreeNode node("Node", &root);
    node.set_flags(ImGuiTreeNodeFlags_Framed).width(200.0f);

    frame([&] { node.render(); });
    const Sizes s = sizes(node);
    EXPECT_NEAR(s.drawn.x, 200.0f, tolerance);
    EXPECT_NEAR(s.drawn.y, ImGui::GetFrameHeight(), tolerance);
    EXPECT_NEAR(s.layout.y, ImGui::GetFrameHeight(), tolerance);
}

TEST_F(Measure, TreeNodeDrawsLinesInsideBalancedScope) {
    TreeNode node("Node", &root);
    // Not a direct child: the line still reaches it, the scope spans all the node's children.
    Widget wrapper("", &node);
    TreeNode child("Child", &wrapper);
    // Each child draws its own connector, so it needs the flag too; the parent's decides.
    child.set_flags(ImGuiTreeNodeFlags_DrawLinesToNodes);
    node.set_open(true);

    // Vertices drawn into the window, and whether the node left ImGui's stacks as it found them.
    auto draw = [&](ImGuiTreeNodeFlags flags) {
        node.set_flags(flags);
        size_t vertices = 0;
        bool balanced = false;
        frame([&] {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            const int ids = window->IDStack.Size;
            const int before = window->DrawList->VtxBuffer.Size;
            node.render();
            vertices = static_cast<size_t>(window->DrawList->VtxBuffer.Size - before);
            balanced = window->IDStack.Size == ids && window->DC.TreeDepth == 0 && GImGui->TreeNodeStack.Size == 0;
        });
        return std::pair{vertices, balanced};
    };

    draw(ImGuiTreeNodeFlags_None);
    const auto [plain, plainBalanced] = draw(ImGuiTreeNodeFlags_None);
    const auto [lines, linesBalanced] = draw(ImGuiTreeNodeFlags_DrawLinesToNodes);
    EXPECT_TRUE(plainBalanced);
    EXPECT_TRUE(linesBalanced);
    EXPECT_GT(lines, plain);
}

TEST_F(Measure, TreeViewExpandsNestedNodes) {
    TreeView view("Tree", &root);
    TreeNode outer("Outer", &view);
    // Not a direct child of a TreeNode: expand_all() still reaches it.
    Widget wrapper("", &outer);
    TreeNode inner("Inner", &wrapper);
    TreeNode leaf("Leaf", &inner);
    leaf.set_flags(ImGuiTreeNodeFlags_Leaf);

    view.expand_all();
    EXPECT_TRUE(outer.is_open());
    EXPECT_TRUE(inner.is_open());
    EXPECT_FALSE(leaf.is_open());

    view.collapse_all();
    EXPECT_FALSE(outer.is_open());
    EXPECT_FALSE(inner.is_open());
}

// Moves

TEST_F(Measure, MovedRadioGroupKeepsSelection) {
    RadioGroup group(&root);
    group.add_option("A");
    group.add_option("B");

    RadioGroup moved(std::move(group));
    moved.at(1).select();
    EXPECT_EQ(moved.selected(), 1);
    EXPECT_TRUE(moved.at(1).selected());
    EXPECT_FALSE(moved.at(0).selected());
}

TEST_F(Measure, MoveAssignedRadioGroupKeepsSelection) {
    RadioGroup group(&root);
    group.add_option("A");
    group.add_option("B");

    RadioGroup target;
    target = std::move(group);
    target.at(0).select();
    target.set_selected(1);
    EXPECT_TRUE(target.at(1).selected());
}

// Re-parenting

TEST_F(Measure, ReparentedContainerRemeasuresItsLeaves) {
    Widget padded("padded", &root);
    padded.align_items(YGAlignFlexStart).set_frame_padding(ImVec2{30.0f, 3.0f});
    Widget plain("plain", &root);
    plain.align_items(YGAlignFlexStart);

    Widget box("box", &padded);
    box.align_items(YGAlignFlexStart);
    Button button("Click me", &box);
    Button reference("Click me", &plain);

    frame([] {});
    const float paddedWidth = button.size().x;
    ASSERT_GT(paddedWidth, reference.size().x + 1.0f);

    // Moving the container, not the leaf: the button now inherits plain's style.
    box.set_parent(&plain);
    frame([] {});
    EXPECT_NEAR(button.size().x, reference.size().x, tolerance);
}

// Entries drawn outside Yoga are measured with their own style

TEST_F(Measure, TableMeasuresCellsWithTheirStyle) {
    Table table(1, &root);
    auto& button = table.add_cell<Button>("Click me");

    float plain = 0.0f;
    float padded = 0.0f;
    frame([&] {
        plain = table.measure(0.0f, YGMeasureModeUndefined, 0.0f, YGMeasureModeUndefined).y;
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        button.set_frame_padding(ImVec2{padding.x, padding.y + 10.0f});
        padded = table.measure(0.0f, YGMeasureModeUndefined, 0.0f, YGMeasureModeUndefined).y;
    });
    EXPECT_NEAR(padded - plain, 20.0f, tolerance);
}

TEST_F(Measure, MenuBarMeasuresMenusWithTheirStyle) {
    MenuBar bar(&root);
    auto& menu = bar.add_menu("File");

    float plain = 0.0f;
    float spaced = 0.0f;
    frame([&] {
        plain = bar.measure(0.0f, YGMeasureModeUndefined, 0.0f, YGMeasureModeUndefined).x;
        const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        menu.set_item_spacing(ImVec2{spacing.x + 10.0f, spacing.y});
        spaced = bar.measure(0.0f, YGMeasureModeUndefined, 0.0f, YGMeasureModeUndefined).x;
    });
    EXPECT_NEAR(spaced - plain, 20.0f, tolerance);
}

// Scrolling tables

namespace {
    // A one-line cell that counts how often it is drawn.
    struct CountingCell : Widget {
        int* shown;

        explicit CountingCell(int* shown) : shown(shown) {
            layout().enable_measure();
        }

        ImVec2 measure(float, YGMeasureMode, float, YGMeasureMode) override {
            return ImVec2{ImGui::CalcTextSize("cell").x, ImGui::GetTextLineHeight()};
        }

        bool show() override {
            ++*shown;
            ImGui::TextUnformatted("cell");
            return false;
        }
    };
}

TEST_F(Measure, ScrollingTableMeasuresHeaderAndFirstRow) {
    Table few({"A", "B"}, &root);
    Table many({"A", "B"}, &root);
    few.set_flags(few.flags() | ImGuiTableFlags_ScrollY);
    many.set_flags(many.flags() | ImGuiTableFlags_ScrollY);
    for (int i = 0; i < 2; ++i) {
        few.add_cell(i);
    }
    for (int i = 0; i < 200; ++i) {
        many.add_cell(i);
    }

    frame([] {});
    const ImGuiStyle& style = ImGui::GetStyle();
    const float row = ImGui::GetTextLineHeight() + style.CellPadding.y * 2.0f;
    EXPECT_NEAR(few.size().y, row * 2.0f, tolerance);
    EXPECT_NEAR(many.size().y, row * 2.0f, tolerance);
}

TEST_F(Measure, ScrollingTableDrawsOnlyVisibleRows) {
    // Widget cells reach the window when drawn; this one is never created, so it opens nothing.
    Window window(std::make_unique<NullBackend>());
    root.set_window(&window);
    Table table(1, &root);
    table.set_flags(table.flags() | ImGuiTableFlags_ScrollY).height(100.0f);
    int shown = 0;
    for (int i = 0; i < 1000; ++i) {
        table.add_cell<CountingCell>(&shown);
    }

    // The clipper measures a row on its first frame; count the second.
    frame([&] { table.render(); });
    shown = 0;
    frame([&] { table.render(); });

    const float row = ImGui::GetTextLineHeight() + ImGui::GetStyle().CellPadding.y * 2.0f;
    EXPECT_GT(shown, 0);
    EXPECT_LE(shown, static_cast<int>(100.0f / row) + 3);
}

// Entries drawn outside Yoga dirty their owner

TEST_F(Measure, RenamingAMenuRemeasuresTheBar) {
    MenuBar bar(&root);
    bar.align_self(YGAlignFlexStart).width(auto_());
    auto& menu = bar.add_menu("File");

    frame([] {});
    const float before = bar.size().x;
    menu.set_title("File and more");
    frame([] {});
    EXPECT_GT(bar.size().x, before + 1.0f);
}

TEST_F(Measure, RestylingACellRemeasuresTheTable) {
    Table table(1, &root);
    auto& button = table.add_cell<Button>("Click me");

    frame([] {});
    const float before = table.size().y;
    const ImVec2 padding = ImGui::GetStyle().FramePadding;
    button.set_frame_padding(ImVec2{padding.x, padding.y + 10.0f});
    frame([] {});
    EXPECT_NEAR(table.size().y - before, 20.0f, tolerance);
}

TEST_F(Measure, RemovingAnOptionKeepsTheSelectedOne) {
    RadioGroup group(&root);
    group.add_option("A");
    group.add_option("B");
    group.add_option("C");
    group.set_selected(2);

    group.remove_option(0);
    EXPECT_EQ(group.count(), 2u);
    EXPECT_EQ(group.selected(), 1);
    EXPECT_TRUE(group.at(1).selected());

    // Values follow the new indices.
    group.at(0).select();
    EXPECT_EQ(group.selected(), 0);
    EXPECT_FALSE(group.at(1).selected());
}

TEST_F(Measure, RemovingTheSelectedOptionSelectsTheFirst) {
    RadioGroup group(&root);
    group.add_option("A");
    group.add_option("B");
    group.add_option("C");
    group.set_selected(1);

    group.remove_option(1);
    EXPECT_EQ(group.selected(), 0);
    EXPECT_TRUE(group.at(0).selected());
    EXPECT_FALSE(group.at(1).selected());
}

// Lifecycle

TEST_F(Measure, InWindowTreeOnlyForTheWindowsRootTree) {
    Window window(std::make_unique<NullBackend>());
    Widget* windowRoot = window.root_widget();
    windowRoot->set_window(&window);
    Widget child("child", windowRoot);
    EXPECT_TRUE(child.in_window_tree());

    // Given the window but not yet its root, as a root widget is until set_root_widget()'s swap runs.
    root.set_window(&window);
    Widget pending("pending", &root);
    EXPECT_FALSE(pending.in_window_tree());

    Widget loose("loose");
    EXPECT_FALSE(loose.in_window_tree());
}

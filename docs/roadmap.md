
# Roadmap

Ideas for what to build next, grouped by area. Sections marked (done) record what was built.
Design details for measuring live in [measure.md](measure.md); its styling section describes an
earlier `Styled<W, S...>` design that was replaced by `Style` (see below).

## Next

Before writing a real app with ironcast, in this order:

1. **Inspector: style view and picking** (see [Inspector](#inspector-done)).

The real app is `examples/zion.cpp`, a system monitor (CPU, processes, system info, logs). Gaps it
turns up go here.

## Disk repair app

A second real-app showcase, separate from Zion: inspect, check and repair disks and filesystems.

- **A frontend over existing tools**, not filesystem code: `lsblk --json`, `sfdisk --json` and
  `blkid` for discovery, `smartctl --json` for health, and each filesystem's checker
  (`fsck.ext4 -n`, `xfs_repair -n`, `btrfs check --readonly`, `fsck.fat -n`, `ntfsfix -n`). Each
  filesystem is a small adapter that runs its tool and maps the output to a common result.
- **Read-only by default.** Repair is an explicit step behind a confirmation modal, and never on a
  mounted filesystem.
- **Image files first.** A script makes broken ext4 / FAT / btrfs images, so it runs without root
  and doubles as a test setup. Real devices need root; use a pkexec helper rather than running the
  GUI as root.
- **What it exercises:** background jobs reporting progress to the UI thread, tool output streamed
  into a log pane, a device → partition → filesystem tree, SMART tables, a partition-layout bar
  and a hex view of the superblock.

Start with read-only discovery and checks on images, then add repair.

## Finish the measure work (done)

- **Explicit heights on frame widgets.** Slider, Drag, Input, TextInput (single line),
  ColorEdit and Dropdown push `ImGuiStyleVar_FramePadding` with
  `y = (height - FontSize) / 2` when their box differs from `GetFrameHeight()`
  (`Widget::push_frame_height`). Frame-height squares (combo arrow, colour swatch) are
  measured with the exact height too (`Widget::frame_height`). The hard-coded heights are
  gone from the examples.
- **Headless measure tests.** `tests/measure_test.cpp` (GoogleTest, `ctest`) creates an
  ImGui context with no window, lays each widget out and compares Yoga's box with
  `GetItemRectSize()`. Covers every leaf and the main flag variants, including the
  ColorPicker, plus explicit boxes on frame widgets. Build with `-DIRONCAST_BUILD_TESTS=OFF`
  to skip.
- **RadioGroup's `&selected_` pointer.** `RadioGroup`'s move operations re-point every
  option at the new group's `selected_`.

## Layout

### Automatic child rendering (done)

Implemented: `Widget::show()` renders its children by walking the Yoga tree, skipping
children with `display(YGDisplayNone)`. The examples no longer override `show()`, and
`RadioGroup::show()` calls the base. Original sketch:

```cpp
bool Widget::show() {
    bool changed = false;
    for (size_t i = 0; i < YGNodeGetChildCount(node); ++i) {
        // child node → context (Layout*) → owner() → render()
        changed |= child->render();
    }
    return changed;
}
```

- Removes the hand-written `show()` in every example
  (`changed |= a.render(); changed |= b.render(); ...`).
- Plain `Row`, `Column` and `Panel` containers come almost free: a `Widget` with
  `row()` / `column()` set.
- Children render in Yoga child order, which is insertion order.
- Leaves are unaffected: they override `show()` and have no children.
- Containers that draw something themselves (`RadioGroup`) can keep their override or call
  the base.

### Scroll containers (done)

`ChildWindow` draws its Yoga box as an ImGui child window; ImGui handles scrollbars and
clipping.

- Its layout is an origin (`Layout::set_origin`): children's positions are relative to it,
  which is what `SetCursorPos` inside `BeginChild` expects.
- The scroll range is ImGui's own, from the items drawn in the child window, plus the
  trailing Yoga padding and border. Popups inside it are drawn in their own window, so
  they don't stretch it.
- Scrollbars are given room through the Yoga border on the right / bottom edge.

## Styling (done)

Every widget owns a `Style` (`Widget::style()`), holding only the values that were set:

- Colours by index: `color(ImGuiCol)` / `color(ImGuiCol, value)`. Each style var has a named
  getter / setter (`set_frame_padding`, `set_alpha`, ...), kept apart by type in a float and an
  `ImVec2` vector. Getters return `std::optional`; setting `std::nullopt` removes the value.
- `Widget::draw()` pushes the style around `show()`, so children inherit it like in ImGui. It is
  popped before the tooltip.
- `measure_trampoline` pushes the styles of the widget and all its ancestors, root first,
  before calling `measure()`. Layout runs before anything is drawn, so this is how a widget is
  measured with the style it is drawn with.
- Style-var setters mark the widget's whole subtree dirty (colours don't): the leaves under a
  container inherit its vars, and `Layout::mark_dirty()` alone does nothing on a container.
- `Style` holds a `Widget*` for that; it is moved with the widget and can't be copied.

- **Fonts.** `Style::set_font(ImFont*)` and `set_font_size()` push `PushFont` with the rest.
  An `ImFont*` belongs to one window's atlas; `Window::load_font()` (file or memory) adds one
  there and returns it, so a font is loaded once and shared between styles.
  `Window::set_font()` makes one the window's primary font (`ImGuiIO::FontDefault`).
- **Widget shortcuts.** `Widget` forwards every `Style` getter and setter, like it does for
  `Layout`.
- **Re-parenting.** `set_parent()` marks the whole subtree dirty, since it now inherits a
  different style.
- **Widgets drawn outside Yoga** (menu entries, table cells) are drawn inside their owner's
  `draw()`, so they inherit its style. Owners that measure them (`Table`, `MenuBar`) push the
  entry's own style around it. Each entry has an entry owner (`Widget::set_entry_owner()`, an id,
  set when it is added): marking the entry dirty (title, style) marks its owner too, through a
  submenu up to the bar.
- **App-wide theme.** `Ironcast::set_theme(Theme)` (`ImGuiStyle`, `ImPlotStyle`, `ImPlot3DStyle`, unscaled)
  is copied into each window's contexts in `opengl::Backend::init()`, then DPI-scaled. Setting it later
  re-themes open windows through an action (between frames) and marks their trees dirty.
  `Theme::font` (`const Font*`, bytes loaded without an ImGui context) is borrowed by each window's atlas
  (`FontDataOwnedByAtlas = false`). Fonts loaded at runtime are owned by the app (`Ironcast::add_font`, a
  `std::vector<std::unique_ptr<Font>>` destroyed after the windows); `remove_font` frees one on the next
  frame, after any pending re-theme has dropped it from the atlases (`RemoveFont` stops reading the bytes).
  `Font::inter()`, `jetbrains_mono()`, `noto_sans()` view the TTFs in `fonts/` (OFL), turned into static byte
  arrays by `cmake/embed.cmake`; `IRONCAST_BUNDLE_FONTS=OFF` leaves them out.
- **Window background image.** `Image` holds RGBA pixels decoded by stb_image (no GL context needed), owned
  by the app (`Ironcast::add_image` / `remove_image`). `Window::set_background(const Image*)` makes that window's
  backend upload its own texture on the next frame and draw it stretched into the root window's draw list,
  above `WindowBg` and below every widget.
  `Ironcast::set_background(image, opacity)` is the default: `Window::init()` takes it unless the window already
  has one.

## New widgets

Each follows the established pattern: `enable_measure()` in the constructor, `measure()`
based on ImGui's own sizing, and `mark_dirty()` in setters that change size.

### Simple leaves

| Widget           | ImGui call                              | Measure notes                                      |
|------------------|-----------------------------------------|----------------------------------------------------|
| ColorButton      | `ColorButton`                           | takes a size; default frame-height square          |
| VSlider          | `VSliderScalar`                         | takes a size; needs an intrinsic height            |
| Image            | `Image`                                 | texture size, or aspect ratio via Yoga             |
| PlotLines/Histogram | `PlotLines` / `PlotHistogram`        | field width + label; takes a graph size            |

### Done

`Separator` (with `SeparatorText` title, vertical variant, title alignment) and `ProgressBar`
(overlay text, indeterminate mode).

### Collapsible containers

`TabWidget` and `TreeNode` are done and follow this pattern. TabWidget's tabs share the strip's
width equally by default, `set_stretch(false)` gives them their natural width. A CollapsingHeader
is a `TreeNode` with `ImGuiTreeNodeFlags_Framed`.

- The header or tab strip is drawn by the container itself.
- When collapsed, or when a tab isn't selected, children get `display(YGDisplayNone)`, so Yoga
  gives them no space.
- Toggling marks the layout dirty. `display` is a style setter, so Yoga notices on its own.
- These containers draw something *and* have children, so they can't use a measure function.
  The header would be its own leaf child placed first.

### Overlays (done)

- **Tooltip:** `Widget::set_tooltip`, shown while the widget's box is hovered.
- **Popup, Modal:** ordinary Yoga children, absolutely positioned (no space taken in the
  parent) and origin layouts. `show()` opens the ImGui popup at the Yoga box's screen
  position and size; children are laid out inside it. `Modal` is a `Popup` with
  `BeginPopupModal`. They nest in each other and in `ChildWindow`.

### Docking (done)

- **DockArea:** an ImGui dock space over its Yoga box. Panes are added with `add_pane()` and
  joined on the next dispatch; `remove_pane()` is deferred too.
- **Pane:** a widget drawn in its own ImGui window, absolutely positioned and an origin layout
  like `Popup`, sized to its window each frame. `set_dock(side)` and `set_ratio()` set where it
  starts and how much of the area it takes; edges and ratios are respected when the area is first
  built. Panes can be closable (a closed pane keeps its children and takes no space, `open()`
  brings it back) and minimized while floating. The layout isn't saved between runs.

## Widget lifecycle (done)

- **init / destroy:** `Widget::init()` and `destroy()` reach the children recursively, `init()`
  in Yoga order and `destroy()` in reverse.
- **post():** `widget.post(fn)` queues `fn` to run on the next dispatch with the widget as its
  own type. Widgets are looked up by id at that point, so the action is dropped if the widget
  was destroyed meanwhile, and a moved widget gets it at its new address. Callbacks such as
  `Button`'s go through it.

## OpenGL

`opengl::RenderWidget` draws a texture and owns a framebuffer with it as the colour attachment.

- **Render callback (done).** `set_on_render([](opengl::RenderWidget& self) {...})`.
  `opengl::RenderWidget::render_texture()` (called from `show()`, and from `opengl::ImageButton::show()`) binds the
  framebuffer, sets `glViewport` to the texture size, calls it, restores the previous framebuffer
  and viewport, and calls `request_redraw()`.
- **Auto-size (done).** `set_auto_size(true)` resizes the texture to the Yoga box times
  `io.DisplayFramebufferScale` whenever that changes, skipping an empty box. Needs a texture.
- **Depth attachment (done).** `set_depth(true)` attaches a `GL_DEPTH_COMPONENT24` renderbuffer,
  resized with the texture. Off by default.
- **Mouse input (done).** `set_capture_mouse(true)` puts an `InvisibleButton` over the box before
  the render callback runs, so drags and the wheel belong to the view instead of moving or scrolling
  the window around it. `hovered()` / `active()` give this frame's button state; everything else
  (`MousePos`, `MouseDelta`, `MouseDown`, `MouseWheel`) is read from `ImGui::GetIO()` in the callback,
  with `MousePos - screen_position()` for view-relative coordinates.

Later: sharing GL objects between windows. Each window has its own context and nothing is
shared (`SDL_GL_SHARE_WITH_CURRENT_CONTEXT`); only needed when several windows show the same
textures or meshes.

## Frame pacing (done)

Implemented with one `FramePolicy` per mode instead of the `Continuous` field combinations below:
`TargetFps`, `FixedDelay` (`delayMs`), `Uncapped`, `OnDemand`, `DisplayMatched`, `FocusBased`
(`idleFps` while unfocused) and `Adaptive` (`idleFps` after `activeTimeoutMs` without activity), dispatched
through a wait-function table indexed by the enum (`windowmanager.cpp`). `FrameConfig` and the
redraw counter live in `WindowManager`; `Ironcast::set_frame_config` and `Ironcast::request_redraw`
forward to it. There is no vsync and no `swapInterval`: every window swaps with interval 0, and
`DisplayMatched` sleeps to the fastest refresh rate among the windows' displays, so waits don't
stack per window. `add_action` pushes a registered wake event. `opengl::RenderWidget::render_texture()` requests a
redraw. Still open: the minimized/occluded skip at the end.

The main loop sleeps a hard-coded `SDL_Delay(20)` per iteration (`WindowManager::update`) and
every window swaps with vsync on (`SDL_GL_SetSwapInterval(1)` in `opengl::Backend::init`). The two
waits stack: the swap waits for the refresh after the sleep, about 30 fps at 60 Hz, and each
extra window can cost another refresh.

Replace both with an app-wide `FrameConfig` and a per-window swap interval:

```cpp
enum class FramePolicy { Continuous, OnDemand };

struct FrameConfig {
    FramePolicy policy = FramePolicy::Continuous;
    int targetFps = 60;         // Continuous: the cap. OnDemand: max rate while redrawing. 0 = uncapped
    int idleTimeoutMs = 500;    // OnDemand: wake at least this often, -1 = only on events
    int minSleepMs = 1;         // always yield a little, even when a frame is over budget
};

// WindowConfig
int swapInterval = 0;           // 1 = vsync, 0 = off, -1 = adaptive (fall back to 1 if it fails)
```

- **Continuous** sleeps `max(budget - elapsed, minSleep)` after each frame (`SDL_GetTicksNS`,
  `SDL_DelayPrecise`). It covers a frame-rate cap, a fixed delay (`targetFps = 0`, the sleep is
  `minSleep`), and uncapped (`targetFps = 0`, `minSleepMs = 0`).
- **OnDemand** blocks in `SDL_WaitEventTimeout(nullptr, idleTimeoutMs)` while nothing asks
  for a redraw, and runs like Continuous while something does. `idleTimeoutMs = -1` is purely
  event-driven.
- **Vsync** is `swapInterval`, separate from the policy; pure vsync is `Continuous` with
  `targetFps = 0` and `swapInterval = 1`.
- The policy is a `switch` in a `WindowManager::wait()` called where `SDL_Delay(20)` is now.

### Redraw requests

`Ironcast::request_redraw(int frames = 2)` keeps an OnDemand loop drawing for that many frames.
Every SDL event requests a redraw (ImGui needs a frame or two to settle hover and layout), and
an animating widget (e.g. `opengl::RenderWidget`) requests one from its `render_texture()` every frame it is
drawn.

Needed before OnDemand is usable:

- `ActionManager` delays count dispatches, so delayed actions stall while the loop sleeps. Keep
  redrawing while the delay ring has pending actions.
- `add_action` is thread-safe, but a sleeping loop won't see it until the timeout. It should
  also `SDL_PushEvent` a user event to wake the loop.

Later: skip `update()` for minimized or occluded windows (`SDL_WINDOW_MINIMIZED`,
`SDL_WINDOW_OCCLUDED`).

## Tools

### Inspector (done)

A devtools-like window per `Window`, toggled with F12 or `Window::set_inspector()`. Built with
`IRONCAST_INSPECTOR` (on by default); without it both do nothing.

- The widget tree, walked through the Yoga tree, with each widget's demangled type, title and id.
  The selected and hovered widgets are kept by id, so a destroyed widget just drops out.
- The selected widget's Yoga box drawn over the app (margin, border, padding, content), and the
  hovered tree row's box as a plain outline.
- Properties: title and enabled (editable), computed visibility, position, size and screen
  position, then margin, border, padding, size, flex and alignment properties, all editable.

Still open:

- Its `Style`: the values set on it and the ones it inherits from ancestors.
- Picking: hover a widget in the app to select it in the tree.
- The last measure result of leaves.

## App services (done)

- **Logging.** spdlog-based, configured with `Ironcast::set_log_config(LogConfig)`: console, a
  rotating file and an in-memory buffer (`memoryCapacity`, read through `log_memory()`). The
  framework's `"ironcast"` logger has its own level; `make_logger(name)` makes app loggers sharing
  the same sinks. `LogWidget` shows the buffer with a level filter, a text filter, Clear and Copy,
  and follows new messages while scrolled to the bottom.
- **Clipboard.** `ironcast::clipboard::set_text()`, `text()`, `has_text()`, shared with ImGui's text
  inputs.
- **Window icon.** `Ironcast::set_icon(const Image*)` is the default for every window,
  `Window::set_icon()` overrides it for one.

## Tests

`tests/` (GoogleTest, `ctest`, `IRONCAST_BUILD_TESTS`): measuring, colour parsing, `Series` and
`ActionManager`. Still uncovered: `Style` push/pop and inheritance, frame policies, logging.

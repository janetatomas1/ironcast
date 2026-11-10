# Changelog

All notable changes to ironcast (named thorin up to 0.3.0). The format follows [Keep a Changelog](https://keepachangelog.com),
and versions follow [Semantic Versioning](https://semver.org); while the version is 0.x, minor
versions may break the API. The version itself is set in `project()` in `CMakeLists.txt` and
exposed as `IRONCAST_VERSION` in `ironcast/version.hpp`. Plans live in [docs/roadmap.md](docs/roadmap.md).

## [0.4.0] - 2026-11-10

### Added

- `webgpu::Backend` (`ironcast::webgpu` library, `IRONCAST_WEBGPU`, ON by default): WebGPU through Dawn
  natively and the browser's WebGPU on Emscripten, with one device shared by all windows, background
  images, `device()` and `upload_texture(image)`.
- `vulkan::Backend` (`ironcast::vulkan` library, `IRONCAST_VULKAN`, ON except on Emscripten): Vulkan 1.3
  with dynamic rendering, built on Vulkan-Hpp RAII, vk-bootstrap and VMA. Vulkan is loaded through SDL, so
  the loader isn't linked. One device is shared by all windows, with background images and
  `shared_device()`.
- `webgpu::RenderWidget` and `vulkan::RenderWidget`: like `sdlgpu::RenderWidget`, a render callback into
  the widget's own texture, with clear colour, auto-size, an optional depth texture and mouse capture.
  The Vulkan one owns its image (`set_texture(width, height, format)`) and renders inside dynamic
  rendering; resizing it waits for the GPU to go idle.
- `ironcast/vulkan/device.hpp`: the Vulkan backend's shared device and helpers, for extending it. It
  pulls in Vulkan-Hpp, VMA and vk-bootstrap, so the other headers don't include it.
- `cmake --install` and `find_package(ironcast)` cover `ironcast::webgpu` and `ironcast::vulkan`.
- `opengl::RenderWidget` and `sdlgpu::RenderWidget`: with a render callback and no texture, `init()`
  creates an RGBA8 texture and turns `auto_size()` on.
- `scripts/install.sh` / `install.ps1` install the Conan dependencies for a compiler and build type;
  `scripts/build.sh` / `build.ps1` build with them.
- `hello` example: one centred button, the example in the new `README.md`.

### Changed

- Renamed the library from thorin to ironcast: the `ironcast` namespace, `<ironcast/...>` headers, the
  `Ironcast` app class (`ironcast/ironcast.hpp`, was `Thorin` in `thorin/thorin.hpp`), the
  `ironcast::ironcast` / `ironcast::opengl` / ... CMake targets and `find_package(ironcast)`, and the
  `IRONCAST_*` options and macros.
- The `chart` example uses the Vulkan backend.
- Tests moved to `tests/unit/`, and `IRONCAST_BUILD_TESTS` is now `IRONCAST_UNIT_TESTS`. `IRONCAST_GUI_TESTS`
  (off by default) is reserved for Dear ImGui Test Engine tests in `tests/gui/`, which need imgui built with
  the test engine.

### Fixed

- MSVC: `/Zc:preprocessor` is set on `ironcast::ironcast` (public), as libassert's macros need the
  conforming preprocessor.
- Windows: examples and tests copy the SDL3 DLL next to the executable.

## [0.3.0] - 2026-10-27

### Added

- `Thorin::window_count()`.
- `thorin/assert.hpp` with `THORIN_ASSERT` / `THORIN_DEBUG_ASSERT`, used instead of libassert's macros.
  On Emscripten they are no-ops and libassert isn't linked.
- `GPUBackend::fail()`: logs a critical message and aborts. `opengl::Backend` and `NullBackend` use it when
  the window, GL context or ImGui backends can't be created, so these failures are reported in release
  and Emscripten builds too, with SDL's reason and the requested context.
- `opengl::BackendConfig::es`: a GLES context instead of a core-profile one; the default on Emscripten, with
  version 3.0.
- Emscripten: `opengl::Backend` uses WebGL2 (GLES 3.0), `exec()` runs one `update()` per browser frame via
  `emscripten_set_main_loop`, and the frame policies are ignored (the browser paces frames). Examples
  build as `.html`.
- `NullBackend`: an SDL window and ImGui context without a graphics API. Frames are built (layout,
  widgets, events) but nothing is drawn; texture requests are acknowledged without uploading.
- `thorin::extras` library (`THORIN_EXTRAS`, ON by default): `ChartWidget`, `Chart3DWidget` and their
  series moved there, under `thorin/extras/`. It is the only part that links implot / implot3d.
- `ChartTheme` and `set_chart_theme(app, theme)`: the ImPlot / ImPlot3D styles and an optional series
  palette. Like `set_theme()`, it is applied between frames, re-measures every window and redraws; each
  window's plot contexts pick it up the next time it draws a chart. The plot contexts are created
  on a window's first chart and destroyed with its ImGui context.

- `RenderWidget::set_clear_color()`: clears the texture (and the depth buffer, with `depth()`) before
  each render callback. Empty by default.
- `thorin::opengl` library (`THORIN_OPENGL`, ON by default): the OpenGL backend and the GL widgets. It is
  the only part that links glbinding / globjects.
- `Thorin::set_backend_factory()` / `make_backend()`: the app makes each window's backend. There is no
  default; set one before adding windows.
- `GPUBackend` helpers for custom backends: `create_window()`, `create_context()`,
  `destroy_context()`, `build_frame()` and the `upload_background()` hook; `process_event()` and
  `make_current()` (sets the ImGui context) now have defaults.
- `sdlgpu::Backend` (`thorin::sdlgpu` library, `THORIN_SDLGPU`, ON except on Emscripten): SDL_GPU
  (Vulkan, Metal or D3D12) with one device shared by all windows, background images, `device()` and
  `upload_texture(image)`.
- `sdlgpu::RenderWidget` / `sdlgpu::ImageWidget`: draws an SDL_GPU texture and runs a render callback
  in a render pass on it, with clear colour, auto-size, an optional depth texture and mouse capture.
  `sdlgpu::ImageButton` is a clickable `sdlgpu::RenderWidget`.
- `Window::post()`: runs a function with the window on the next dispatch, dropped if the window was
  closed meanwhile, like `Widget::post()`.
- `Widget::post()` and `Window::post()` take a `delay`, like `add_action()`.
- `RadioGroup::remove_option(index)`: later options move down, so values stay equal to indices; the
  same option stays selected, or the first one if the selected option was removed.
- `Widget::in_window_tree()`: whether the widget's tree is its window's root tree.
- `Widget::set_entry_owner()` / `entry_owner()`: the widget that measures an entry drawn outside the Yoga
  tree. `MenuBar`, `Menu` and `Table` set it on what they add.
- Scrolling `Table` (`ImGuiTableFlags_ScrollY`): only the visible rows are drawn (`ImGuiListClipper`),
  and the header row stays on top. It takes its height from `height()` or flex; without one it measures
  as the header and the first row, so measuring no longer walks every cell.

### Fixed

- `Thorin::get_window_at()` returns `nullptr` for an index out of range, as documented, instead of asserting.
- `maximize()`, `minimize()`, `set_size()` and `set_title()` no longer touch a closed window: their
  queued actions go through `Window::post()` instead of capturing the backend.
- `Container::remove()` (so `TabWidget::remove_tab()`, `DockArea::remove_pane()`) calls the widget's
  `destroy()` before freeing it: a removed `RenderWidget` released its GL objects in whatever context
  was current, possibly another window's.
- `Window::set_title()` changes the OS window title (`SDL_SetWindowTitle`); it only changed the stored
  one. The root ImGui window has a fixed name, so renaming no longer resets tree nodes, scroll, tables
  and other ImGui state.
- `TabWidget` follows tab switches that aren't a click on the tab (the tab list popup, keyboard
  navigation); before, the next frame switched back.
- `Window::init()` calls `init()` on its root widget tree; only a root set with `set_root_widget()` got it,
  while `Window::destroy()` destroyed every root.
- Pages and panes added to a `TabWidget` / `DockArea` after the window started get `init()`, like those
  added before (`remove()` already calls `destroy()`).
- Renaming or restyling a menu, menu entry or table cell marks its `MenuBar` / `Table` dirty, so its
  measured size no longer goes stale.
- A delay longer than the action ring runs at its limit in debug builds too, as documented, instead of
  asserting.
- `Table` and `MenuBar` measure their cells and menus with the entry's own style, as they draw them;
  a cell with its own padding or a menu with its own font was measured with the owner's style.
- `set_parent()` marks the whole subtree dirty, as documented: moving a container left its leaves
  measured with the old parent's style.
- `Window::set_root_widget()` no longer touches a closed window: the swap goes through `Window::post()`
  instead of capturing the window.

### Changed

- Each graphics API has its own namespace and directory, with the same class names in each: the
  OpenGL classes moved to `thorin::opengl` and `thorin/opengl/` (`GLBackend` -> `opengl::Backend`,
  `GLBackendConfig` -> `opengl::BackendConfig`, `RenderWidget` / `ImageWidget` / `ImageButton` ->
  `opengl::RenderWidget` / `ImageWidget` / `ImageButton`; headers `backend.hpp`, `renderwidget.hpp`,
  `imagewidget.hpp`, `imagebutton.hpp`). `thorin::gl` is avoided because it would hide glbinding's `gl`.
- Removed the unused `enum class Backend` from `windowconfig.hpp`; backends are chosen with
  `set_backend_factory()`.
- `Theme` no longer has `plot` / `plot3d`, and windows no longer create ImPlot / ImPlot3D contexts;
  use `ChartTheme` from `thorin/extras/charttheme.hpp`.
- `Window` takes its backend in the constructor (`Window(std::unique_ptr<GPUBackend>, widget)`)
  instead of creating a `opengl::Backend` itself; use `app.make_backend(config)`.
- `WindowConfig::sdlFlags` no longer includes `SDL_WINDOW_OPENGL`; `opengl::Backend` adds it.
- Each dispatch runs the delayed actions that are due before the queued ones, so a re-post with delay 1
  runs once per dispatch even from a delayed action (it used to run every other dispatch). An action
  queued from a running action with delay 0 still runs in the same dispatch; this is now documented, and
  an action that re-posts itself needs delay 1.
- Each backend is its own library linking `thorin::thorin`: apps link `thorin::opengl` or
  `thorin::sdlgpu` and call `set_backend_factory()`. The core library no longer defines `THORIN_GL` /
  `THORIN_SDLGPU`.

## [0.2.0] - 2026-10-20

### Added

- `TreeNode`: a collapsible node with an ImGui tree node header. Its children can be any widgets;
  they are indented by `IndentSpacing` and hidden from Yoga while it is closed. Nest TreeNodes to
  build a tree; `set_flags(ImGuiTreeNodeFlags_Framed)` gives a collapsing header. ImGui's tree
  lines work (`DrawLinesFull` / `DrawLinesToNodes` on the nodes, or `style.TreeLinesFlags`).
- `TreeView`: a scrolling, bordered child window for a tree of TreeNodes, with `expand_all()` and
  `collapse_all()`.
- `ChartWidget`: an ImPlot plot at the widget's Yoga box, with axes (label, fixed limits, tick
  format) and a legend. It owns its series: `LineSeries`, `ScatterSeries` and `BarSeries`, each
  with a stable id, a colour and its axes. Their values are owned (`SeriesBuffer`), borrowed
  (`SeriesView`) or a fixed-capacity ring (`SeriesRing`) for live data.
- `Chart3DWidget`: the ImPlot3D counterpart, with `Line3DSeries` and `Scatter3DSeries`.
- Themes: `Thorin::set_theme(Theme)` sets the ImGui, ImPlot and ImPlot3D styles and the font of
  every window, including open ones. Built in: `dark`, `light`, `classic`, `matrix`, `nord`,
  `blueprint`, `one_dark`.
- Fonts: `Font` holds TTF bytes without an ImGui context; `Thorin::add_font()` / `remove_font()`.
  Inter, JetBrains Mono and Noto Sans are bundled (`Font::inter()`, ...), and can be left out with
  `THORIN_BUNDLE_FONTS=OFF`.
- Window backgrounds: `Image` (decoded with stb_image, `Thorin::add_image()`), drawn stretched
  behind every widget with `Window::set_background()` or `Thorin::set_background()` for all
  windows. A `BackgroundPainter` draws a custom background on top of it
  (`Window::set_background_painter()`, or a factory in `Thorin::set_background_painter()`).
- Window icons: `Thorin::set_icon()` for every window, `Window::set_icon()` for one.
- Logging (spdlog): `Thorin::set_log_config(LogConfig)` with console, rotating file and an
  in-memory buffer (`log_memory()`). thorin logs to its own `"thorin"` logger with a separate
  level; `make_logger(name)` makes app loggers that share the sinks.
- `LogWidget`: the in-memory log with a level filter, a text filter, Clear and Copy; follows new
  messages while scrolled to the bottom.
- `thorin::clipboard`: `set_text()`, `text()`, `has_text()`, shared with ImGui's text inputs.
- `Table`: `set_column_width()` and `set_column_stretch()`, and a constructor for a table without
  a header row.
- `RenderWidget::set_capture_mouse()`: an invisible button over the view, so camera controls can drag
  and use the wheel without moving or scrolling the window. `hovered()` / `active()` tell the render
  callback whether the input from `ImGui::GetIO()` is meant for the view.
- Inspector: a devtools-like window, toggled with F12 or `Window::set_inspector()`. It shows the
  widget tree, draws the selected widget's box over the app, and edits its layout properties.
  Built with `THORIN_INSPECTOR` (on by default).
- Examples: `zion`, a system monitor (CPU, processes, system info, logs), and `chart` /
  `chart3d`.
- Tests: colour parsing, `Series` and `ActionManager`, next to the measure tests.

- CMake package: `find_package(thorin)` and `thorin::thorin`, from the build tree (`thorin_DIR`) or
  after `cmake --install`. The app provides the same dependencies, e.g. through the same Conan toolchain.
- thorin can be used with `add_subdirectory()`; examples and tests then stay off by default
  (`THORIN_BUILD_EXAMPLES`, `THORIN_BUILD_TESTS`).

### Changed

- `WindowConfig::iniFile` sets the ImGui settings file for that window's context. It is empty by
  default, which turns saving off. Before, every window wrote to the same `imgui.ini` in the
  working directory and they overwrote each other's settings.
- Themes default to larger text (`style.FontSizeBase` 0 means 19.5 px, 1.5x ImGui's 13 px; the
  built-in themes use 22 px) and roomier table cells.
- `Pane` resizes its Yoga box to its ImGui window as soon as the window changes size, and
  requests a redraw.

## [0.1.0] - 2026-10-07

First version: the features thorin has today.

### Application and windows

- `Thorin` app with a main loop (`exec()` / `exit()`), multiple windows, each with its own ImGui
  context, OpenGL context and root widget. `add_window<W>(config, args...)` creates the root widget
  in place.
- `WindowConfig`: size, title, ImGui window flags, SDL window flags, initial state (maximized,
  minimized). `Window` can maximize, minimize, resize and retitle at runtime.
- Actions: `add_action(fn, delay)` runs `fn` on a later dispatch, delayed by a number of frames;
  it can be called from any thread. `widget.post(fn)` does the same for a widget and drops the
  action if the widget is destroyed in the meantime.
- Widget lifecycle: `init()` / `destroy()` reach children recursively, `destroy()` in reverse order.
- OpenGL 3.3 backend on SDL3 (`GLBackend`), behind the `GPUBackend` interface.
- `thorin/version.hpp` with `THORIN_VERSION` and its major / minor / patch parts.

### Layout

- Yoga flexbox layout on every widget: margin, padding, position, size and min / max size, gap,
  flex direction / grow / shrink / basis / wrap, alignment and justification, as chainable setters.
- Measure functions for every leaf widget, based on ImGui's own sizing, so widgets get their
  natural size. Explicit heights on frame widgets are honoured through frame padding.
- Automatic child rendering in Yoga order; `display(YGDisplayNone)` children are skipped.
- `ChildWindow`: a scrolling, clipping container drawn as an ImGui child window.
- Headless measure tests (`tests/measure_test.cpp`, GoogleTest, `THORIN_BUILD_TESTS`).

### Styling

- Per-widget `Style`: colours by `ImGuiCol` and every ImGui style var, inherited by children,
  also applied while measuring. Getters return `std::optional`; `std::nullopt` removes a value.
- Fonts: `Window::load_font()` (file or memory), `Window::set_font()` for the window default,
  `Style::set_font()` / `set_font_size()` per widget.
- Colours: `from_hex(0xRRGGBBAA)` and the CSS named colours in `thorin::colors`.
- Tooltips on any widget (`set_tooltip`), enabled / disabled state (`set_enabled`).

### Widgets

- **Basic:** `Text`, `Button`, `Checkbox`, `RadioGroup` / `RadioButton`, `Separator` (horizontal,
  vertical, with title and title alignment), `ProgressBar` (overlay text, indeterminate mode).
- **Input:** `TextInput` (single line, multiline, hint), `Slider`, `Drag` and `Input` (int or
  float, 1-4 components, range, step, format), `ColorEdit` and `ColorPicker` (RGB / RGBA,
  reference colour), `Dropdown` (options, placeholder), `Selectable` (multi-select list).
- **Containers:** `Table` (headers, number cells or any widget as a cell), `TabWidget` (tabs added and
  removed at runtime, stretched or natural-width), `MenuBar` / `Menu` / `MenuItem` (submenus, separators,
  shortcuts, checkable items).
- **Overlays:** `Popup` and `Modal`, laid out by Yoga and positioned at their box; they nest.
- **Docking:** `DockArea` with `Pane`s: initial side and ratio, closable panes that take no space
  when closed, minimize / restore while floating.
- **OpenGL:** `RenderWidget` / `ImageWidget` draws a GL texture and owns a framebuffer to render
  into it (resize, UV flip); `ImageButton` is a clickable `RenderWidget`.

### Examples

One example per widget in `examples/`, plus `helloworld`.

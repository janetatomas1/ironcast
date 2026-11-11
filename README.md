# ironcast

A C++23 GUI framework on top of [Dear ImGui](https://github.com/ocornut/imgui), with
[Yoga](https://github.com/facebook/yoga) flexbox layout and SDL3 windows. It runs the app: the event
loop, windows and GPU backends. Your UI is a tree of retained widgets, which ImGui draws each frame.

## Backends

Each graphics backend is its own library and namespace. Pick one in code with `set_backend_factory()`.

| Target              | API                                         | Linux (GCC, Clang) | Windows (MSVC, clang-cl) | Web (Emscripten) |
|---------------------|---------------------------------------------|:------------------:|:------------------------:|:----------------:|
| `ironcast::opengl`  | OpenGL 3 / WebGL2 (glbinding, globjects)    | ✓                  | ✓                        | ✓                |
| `ironcast::sdlgpu`  | SDL_GPU (Vulkan, Metal or D3D12)            | ✓                  | ✓                        |                  |
| `ironcast::webgpu`  | WebGPU (Dawn, or the browser's)             | ✓                  | ✓                        | ✓                |
| `ironcast::vulkan`  | Vulkan 1.3 (Vulkan-Hpp, vk-bootstrap, VMA)  | ✓                  | ✓                        |                  |

`ironcast::extras` adds `ChartWidget` and `Chart3DWidget` (ImPlot / ImPlot3D).

## Example

```cpp
#include <ironcast/ironcast.hpp>
#include <ironcast/button.hpp>
#include <ironcast/vulkan/backend.hpp>
#include <ironcast/log.hpp>

using namespace ironcast;
using namespace literals;

struct Hello: Widget {
    Button button = {"Click me", [] { logger().info("clicked"); }, this};

    Hello(const std::string& title): Widget(title) {
        // Percentage margins are of the parent's width, even on the top edge: let the layout centre it.
        center();
        button.width(20_pcts).height(10_pcts);
    }
};

int main(int argc, char** argv) {
    auto& app = Ironcast::app(argc, argv);
    app.set_backend_factory([](const WindowConfig& config) {
        return std::make_unique<vulkan::Backend>(config);
    });
    app.add_window<Hello>(std::string("Hello"));
    return app.exec();
}
```

Examples, in [examples/](examples):

- `hello`: the example above (Vulkan).
- `zion`: a Linux system monitor, the full-app demo (SDL_GPU; Linux only).
- `chart`: sine and cosine scrolling in a `ChartWidget` (Vulkan).
- `chart3d`: a helix traced in a `Chart3DWidget` (OpenGL).

## Building

Needs CMake 3.28+, Ninja, Python 3 and Conan 2. Dependencies come from the Conan recipes in the
`submodules/conanfiles` submodule. Install them once per compiler and build type, then build:

```sh
scripts/install.sh gcc --debug        # gcc, clang or emscripten; --debug, --release or both
scripts/build.sh gcc --debug          # output in build/<compiler>-<type>
```

`install.sh` checks out the submodule and deploys the dependencies to `submodules/conanfiles/third-party/`.
The first run builds the missing packages, which takes a while. Emscripten needs `EMSDK` set and `node` on
the `PATH`.

On Windows, use `scripts\install.ps1` and `scripts\build.ps1` with `msvc` or `clang-cl`.

Instead of `build.sh` / `build.ps1`, CMake presets do the same, one per compiler and build type
(`cmake --list-presets` shows the ones for your OS):

```sh
cmake --preset gcc-debug              # <compiler>-<debug|release>, same build/ folder as build.sh
cmake --build build/gcc-debug
ctest --preset gcc-debug
```

The `msvc` presets need a Visual Studio developer prompt, as Ninja uses its environment.

CMake options, all on by default: `IRONCAST_OPENGL`, `IRONCAST_SDLGPU`, `IRONCAST_WEBGPU`, `IRONCAST_VULKAN`,
`IRONCAST_EXTRAS`, `IRONCAST_BUNDLE_FONTS`, `IRONCAST_INSPECTOR`. Examples and unit tests build when ironcast is
the top-level project (`IRONCAST_BUILD_EXAMPLES`, `IRONCAST_UNIT_TESTS`). The GUI tests (`IRONCAST_GUI_TESTS`) are
off by default: they need imgui built with Dear ImGui Test Engine.

## Using it in a project

Either `add_subdirectory()` it, or `find_package(ironcast)` from its build tree or install prefix, then
link a backend:

```cmake
find_package(ironcast REQUIRED)
target_link_libraries(app PRIVATE ironcast::vulkan)
```

Each backend brings in the core library, `ironcast::ironcast`, so it doesn't need linking separately.

The app has to provide the same dependencies, e.g. through the same Conan toolchain.

## Docs

- [CHANGELOG.md](CHANGELOG.md)
- [docs/roadmap.md](docs/roadmap.md)
- [docs/measure.md](docs/measure.md)

## AI usage

AI coding tools helped write parts of ironcast.

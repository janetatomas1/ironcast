
cmake_minimum_required(VERSION 3.28)
project(ironcast VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 23)

include(GNUInstallDirs)

set(IRONCAST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/ironcast)
set(IRONCAST_HEADER_DIR ${PROJECT_SOURCE_DIR}/include/)
set(IRONCAST_EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/examples)
set(
    IRONCAST_SOURCES
    ${IRONCAST_SOURCE_DIR}/ironcast.cpp
    ${IRONCAST_SOURCE_DIR}/theme.cpp
    ${IRONCAST_SOURCE_DIR}/font.cpp
    ${IRONCAST_SOURCE_DIR}/image.cpp
    ${IRONCAST_SOURCE_DIR}/log.cpp
    ${IRONCAST_SOURCE_DIR}/windowmanager.cpp
    ${IRONCAST_SOURCE_DIR}/actionmanager.cpp
    ${IRONCAST_SOURCE_DIR}/window.cpp
    ${IRONCAST_SOURCE_DIR}/widget.cpp
    ${IRONCAST_SOURCE_DIR}/layout.cpp
    ${IRONCAST_SOURCE_DIR}/gpubackend.cpp
    ${IRONCAST_SOURCE_DIR}/nullbackend.cpp
    ${IRONCAST_SOURCE_DIR}/button.cpp
    ${IRONCAST_SOURCE_DIR}/checkbox.cpp
    ${IRONCAST_SOURCE_DIR}/radiobutton.cpp
    ${IRONCAST_SOURCE_DIR}/radiogroup.cpp
    ${IRONCAST_SOURCE_DIR}/dropdown.cpp
    ${IRONCAST_SOURCE_DIR}/textinput.cpp
    ${IRONCAST_SOURCE_DIR}/text.cpp
    ${IRONCAST_SOURCE_DIR}/colorpicker.cpp
    ${IRONCAST_SOURCE_DIR}/menu.cpp
    ${IRONCAST_SOURCE_DIR}/menubar.cpp
    ${IRONCAST_SOURCE_DIR}/table.cpp
    ${IRONCAST_SOURCE_DIR}/tabwidget.cpp
    ${IRONCAST_SOURCE_DIR}/childwindow.cpp
    ${IRONCAST_SOURCE_DIR}/selectable.cpp
    ${IRONCAST_SOURCE_DIR}/treenode.cpp
    ${IRONCAST_SOURCE_DIR}/treeview.cpp
    ${IRONCAST_SOURCE_DIR}/popup.cpp
    ${IRONCAST_SOURCE_DIR}/modal.cpp
    ${IRONCAST_SOURCE_DIR}/progressbar.cpp
    ${IRONCAST_SOURCE_DIR}/logwidget.cpp
    ${IRONCAST_SOURCE_DIR}/clipboard.cpp
    ${IRONCAST_SOURCE_DIR}/separator.cpp
    ${IRONCAST_SOURCE_DIR}/pane.cpp
    ${IRONCAST_SOURCE_DIR}/dockarea.cpp
    ${IRONCAST_SOURCE_DIR}/style.cpp
    ${IRONCAST_SOURCE_DIR}/color.cpp
)

find_package(sdl REQUIRED)
find_package(concurrentqueue REQUIRED)
find_package(imgui REQUIRED)
find_package(function2 REQUIRED)
find_package(yoga REQUIRED)
find_package(stb REQUIRED)
find_package(spdlog REQUIRED)

add_library(
    ironcast
    STATIC
    ${IRONCAST_SOURCES}
)
# The name apps link against, the same whether ironcast comes from find_package() or add_subdirectory().
add_library(ironcast::ironcast ALIAS ironcast)
# The headers use C++23 (deducing this), so apps need it too.
target_compile_features(ironcast PUBLIC cxx_std_23)

target_link_libraries(
    ironcast
    PUBLIC
    sdl::sdl
    concurrentqueue::concurrentqueue
    imgui::imgui
    function2::function2
    yoga::yoga
    spdlog::spdlog
    PRIVATE
    # Only image.cpp decodes images.
    stb::stb
)

# libassert isn't available on Emscripten; ironcast/assert.hpp turns the assertions into no-ops there.
if(NOT EMSCRIPTEN)
    find_package(libassert REQUIRED)
    target_link_libraries(ironcast PUBLIC libassert::assert)
    # libassert's macros need MSVC's conforming preprocessor; public because apps expand them too.
    target_compile_options(ironcast PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)
endif()

# OpenGL backend and widgets, in ironcast::opengl. The only part that links glbinding / globjects.
option(IRONCAST_OPENGL "Build the OpenGL backend" ON)
if(IRONCAST_OPENGL)
    add_library(
        ironcast_opengl
        STATIC
        ${IRONCAST_SOURCE_DIR}/opengl/backend.cpp
        ${IRONCAST_SOURCE_DIR}/opengl/renderwidget.cpp
        ${IRONCAST_SOURCE_DIR}/opengl/imagebutton.cpp
    )
    add_library(ironcast::opengl ALIAS ironcast_opengl)
    set_target_properties(ironcast_opengl PROPERTIES EXPORT_NAME opengl)
    find_package(glbinding REQUIRED)
    find_package(globjects REQUIRED)
    target_link_libraries(ironcast_opengl PUBLIC ironcast glbinding::glbinding globjects::globjects)
    # Emscripten links WebGL 1 only by default; opengl::Backend needs WebGL2 (GLES 3.0).
    if(EMSCRIPTEN)
        target_link_options(ironcast_opengl PUBLIC -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
    endif()
endif()

# SDL_GPU backend (Vulkan, Metal or D3D12) and widgets, in ironcast::sdlgpu. SDL_GPU has no web driver, so it is off on Emscripten.
include(CMakeDependentOption)
cmake_dependent_option(IRONCAST_SDLGPU "Build the SDL_GPU backend" ON "NOT EMSCRIPTEN" OFF)
if(IRONCAST_SDLGPU)
    add_library(
        ironcast_sdlgpu
        STATIC
        ${IRONCAST_SOURCE_DIR}/sdlgpu/backend.cpp
        ${IRONCAST_SOURCE_DIR}/sdlgpu/renderwidget.cpp
        ${IRONCAST_SOURCE_DIR}/sdlgpu/imagebutton.cpp
    )
    add_library(ironcast::sdlgpu ALIAS ironcast_sdlgpu)
    set_target_properties(ironcast_sdlgpu PROPERTIES EXPORT_NAME sdlgpu)
    target_link_libraries(ironcast_sdlgpu PUBLIC ironcast)
endif()

# WebGPU backend (Dawn natively, emdawnwebgpu on Emscripten), in ironcast::webgpu.
option(IRONCAST_WEBGPU "Build the WebGPU backend" ON)
if(IRONCAST_WEBGPU)
    add_library(
        ironcast_webgpu
        STATIC
        ${IRONCAST_SOURCE_DIR}/webgpu/backend.cpp
    )
    add_library(ironcast::webgpu ALIAS ironcast_webgpu)
    set_target_properties(ironcast_webgpu PROPERTIES EXPORT_NAME webgpu)
    find_package(Dawn REQUIRED)
    find_package(sdl3webgpu REQUIRED)
    target_link_libraries(ironcast_webgpu PUBLIC ironcast dawn::webgpu_dawn PRIVATE sdl3webgpu::sdl3webgpu)
    # Backend::init() blocks in WaitAny() for the adapter and device; in the browser that needs JSPI.
    if(EMSCRIPTEN)
        target_link_options(ironcast_webgpu PUBLIC -sJSPI)
    endif()
endif()

# Vulkan 1.3 backend, in ironcast::vulkan; native only. Vulkan is loaded at runtime through SDL, so the Vulkan
# loader isn't linked.
cmake_dependent_option(IRONCAST_VULKAN "Build the Vulkan backend" ON "NOT EMSCRIPTEN" OFF)
if(IRONCAST_VULKAN)
    add_library(
        ironcast_vulkan
        STATIC
        ${IRONCAST_SOURCE_DIR}/vulkan/backend.cpp
        ${IRONCAST_SOURCE_DIR}/vulkan/vma.cpp
    )
    add_library(ironcast::vulkan ALIAS ironcast_vulkan)
    set_target_properties(ironcast_vulkan PROPERTIES EXPORT_NAME vulkan)
    find_package(VulkanHeaders REQUIRED)
    find_package(VulkanMemoryAllocator REQUIRED)
    find_package(vk-bootstrap REQUIRED)
    target_link_libraries(
        ironcast_vulkan
        PUBLIC ironcast vulkan-headers::vulkan-headers GPUOpen::VulkanMemoryAllocator vk-bootstrap::vk-bootstrap
    )
    # No prototypes: functions come through SDL's vkGetInstanceProcAddr (imgui is built the same way), so
    # Vulkan-Hpp's own loader is off too. Vulkan-Hpp returns OutOfDate from acquire and present instead of
    # throwing, as the frame loop expects it.
    target_compile_definitions(
        ironcast_vulkan
        PUBLIC VK_NO_PROTOTYPES VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0 VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
    )
endif()

if(NOT IRONCAST_OPENGL AND NOT IRONCAST_SDLGPU AND NOT IRONCAST_WEBGPU AND NOT IRONCAST_VULKAN)
    message(STATUS "ironcast: no real backend built (IRONCAST_OPENGL, IRONCAST_SDLGPU, IRONCAST_WEBGPU and IRONCAST_VULKAN are off); only NullBackend, which draws nothing")
endif()

# Optional widgets on top of ironcast: ChartWidget and Chart3DWidget (ImPlot / ImPlot3D).
option(IRONCAST_EXTRAS "Build the extras library" ON)
if(IRONCAST_EXTRAS)
    add_library(
        ironcast_extras
        STATIC
        ${IRONCAST_SOURCE_DIR}/extras/charttheme.cpp
        ${IRONCAST_SOURCE_DIR}/extras/series.cpp
        ${IRONCAST_SOURCE_DIR}/extras/chartwidget.cpp
        ${IRONCAST_SOURCE_DIR}/extras/series3d.cpp
        ${IRONCAST_SOURCE_DIR}/extras/chart3dwidget.cpp
    )
    add_library(ironcast::extras ALIAS ironcast_extras)
    set_target_properties(ironcast_extras PROPERTIES EXPORT_NAME extras)
    find_package(implot REQUIRED)
    find_package(implot3d REQUIRED)
    target_link_libraries(ironcast_extras PUBLIC ironcast implot::implot implot3d::implot3d)
endif()

# Inter, JetBrains Mono and Noto Sans (OFL, see fonts/*/OFL.txt) compiled in as Font::inter() etc.
option(IRONCAST_BUNDLE_FONTS "Embed the fonts in fonts/ (~1.3 MB)" ON)
if(IRONCAST_BUNDLE_FONTS)
    foreach(font IN ITEMS
        "inter|inter/Inter-Regular.ttf"
        "jetbrains_mono|jetbrains-mono/JetBrainsMono-Regular.ttf"
        "noto_sans|noto-sans/NotoSans-Regular.ttf"
    )
        string(REPLACE "|" ";" font "${font}")
        list(GET font 0 name)
        list(GET font 1 file)
        set(input ${PROJECT_SOURCE_DIR}/fonts/${file})
        set(output ${CMAKE_CURRENT_BINARY_DIR}/fonts/${name}.cpp)
        add_custom_command(
            OUTPUT ${output}
            COMMAND ${CMAKE_COMMAND} -DINPUT=${input} -DOUTPUT=${output} -DNAME=${name}
                -P ${PROJECT_SOURCE_DIR}/cmake/embed.cmake
            DEPENDS ${input} ${PROJECT_SOURCE_DIR}/cmake/embed.cmake
            COMMENT "Embedding font ${file}"
        )
        target_sources(ironcast PRIVATE ${output})
    endforeach()
    target_compile_definitions(ironcast PUBLIC IRONCAST_BUNDLE_FONTS)
endif()

# Widget inspector overlay, opened with F12 or Window::set_inspector(). Turn off for shipped apps.
option(IRONCAST_INSPECTOR "Build the widget inspector overlay" ON)
if(IRONCAST_INSPECTOR)
    target_sources(ironcast PRIVATE ${IRONCAST_SOURCE_DIR}/inspector.cpp)
    target_compile_definitions(ironcast PRIVATE IRONCAST_INSPECTOR)
endif()

configure_file(
    ${IRONCAST_HEADER_DIR}/ironcast/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/ironcast/version.hpp
    @ONLY
)
target_include_directories(
    ironcast
    PUBLIC
    $<BUILD_INTERFACE:${IRONCAST_HEADER_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# CMake package: find_package(ironcast) and link a backend, ironcast::opengl or ironcast::sdlgpu (both bring in
# ironcast::ironcast). Usable from the build tree (ironcast_DIR = this build directory) or after cmake --install
# (CMAKE_PREFIX_PATH = the install prefix). The app has to provide the same dependencies, e.g. through the
# same Conan toolchain.
include(CMakePackageConfigHelpers)
install(TARGETS ironcast EXPORT ironcastTargets)
if(IRONCAST_OPENGL)
    install(TARGETS ironcast_opengl EXPORT ironcastTargets)
endif()
if(IRONCAST_SDLGPU)
    install(TARGETS ironcast_sdlgpu EXPORT ironcastTargets)
endif()
if(IRONCAST_EXTRAS)
    install(TARGETS ironcast_extras EXPORT ironcastTargets)
endif()
if(NOT IRONCAST_OPENGL)
    list(APPEND IRONCAST_INSTALL_EXCLUDE PATTERN "opengl" EXCLUDE)
endif()
if(NOT IRONCAST_SDLGPU)
    list(APPEND IRONCAST_INSTALL_EXCLUDE PATTERN "sdlgpu" EXCLUDE)
endif()
if(NOT IRONCAST_EXTRAS)
    list(APPEND IRONCAST_INSTALL_EXCLUDE PATTERN "extras" EXCLUDE)
endif()
install(
    DIRECTORY ${IRONCAST_HEADER_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
    ${IRONCAST_INSTALL_EXCLUDE}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ironcast/version.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ironcast)
install(EXPORT ironcastTargets NAMESPACE ironcast:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ironcast)
export(EXPORT ironcastTargets NAMESPACE ironcast:: FILE ${CMAKE_CURRENT_BINARY_DIR}/ironcastTargets.cmake)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ironcastConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ironcastConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ironcast
)
# 0.x: minor versions may break the API, so only the same minor version matches.
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ironcastConfigVersion.cmake
    COMPATIBILITY SameMinorVersion
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/ironcastConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/ironcastConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ironcast
)
# Examples and tests only build by default when ironcast is the top-level project, not when an app pulls
# it in with add_subdirectory().
option(IRONCAST_BUILD_EXAMPLES "Build the examples" ${PROJECT_IS_TOP_LEVEL})
if(IRONCAST_BUILD_EXAMPLES)
    add_subdirectory(${IRONCAST_EXAMPLES_DIR})
endif()

option(IRONCAST_BUILD_TESTS "Build the headless tests" ${PROJECT_IS_TOP_LEVEL})
if(IRONCAST_BUILD_TESTS)
    enable_testing()
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()

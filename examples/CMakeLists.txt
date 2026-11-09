# Each example picks its backend in code and is built when that backend is. Every built backend is linked,
# so switching an example's backend needs no change here, only the condition below.
set(IRONCAST_EXAMPLE_NAMES)
set(IRONCAST_EXAMPLE_BACKENDS)
if(IRONCAST_VULKAN)
    list(APPEND IRONCAST_EXAMPLE_NAMES chart)
    list(APPEND IRONCAST_EXAMPLE_BACKENDS ironcast::vulkan)
endif()
if(IRONCAST_WEBGPU)
    list(APPEND IRONCAST_EXAMPLE_BACKENDS ironcast::webgpu)
endif()
if(IRONCAST_OPENGL)
    list(APPEND IRONCAST_EXAMPLE_NAMES chart3d)
    list(APPEND IRONCAST_EXAMPLE_BACKENDS ironcast::opengl)
endif()
if(IRONCAST_SDLGPU)
    list(APPEND IRONCAST_EXAMPLE_BACKENDS ironcast::sdlgpu)
    # zion reads /proc and /sys, so it only builds on Linux.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(PREPEND IRONCAST_EXAMPLE_NAMES zion)
    endif()
endif()

# Emscripten defaults to .js; .html also generates a page that loads the .js and .wasm.
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

foreach(example_name ${IRONCAST_EXAMPLE_NAMES})
    add_executable(${example_name} ${IRONCAST_EXAMPLES_DIR}/${example_name}.cpp)
    target_link_libraries(${example_name} PRIVATE ironcast::extras ${IRONCAST_EXAMPLE_BACKENDS})
    # Examples load their images from the source tree, wherever they are run from.
    target_compile_definitions(${example_name} PRIVATE IRONCAST_EXAMPLES_ASSETS="${IRONCAST_EXAMPLES_DIR}/assets")
    # Windows has no rpath: copy DLLs (SDL3) next to the example.
    if(WIN32)
        add_custom_command(
            TARGET ${example_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${example_name}> $<TARGET_FILE_DIR:${example_name}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endforeach()

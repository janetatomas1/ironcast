# GUI tests, built with IRONCAST_GUI_TESTS: one executable, one CTest test (and process) per IRONCAST_TEST.
add_executable(gui_tests main.cpp layout_test.cpp)
target_link_libraries(gui_tests PRIVATE ironcast::testing)
# Windows has no rpath: copy DLLs (SDL3) next to the executable. Before ironcast_discover_tests, which runs
# it right after linking.
if(WIN32)
    add_custom_command(
        TARGET gui_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:gui_tests> $<TARGET_FILE_DIR:gui_tests>
        COMMAND_EXPAND_LISTS
    )
endif()
ironcast_discover_tests(gui_tests)

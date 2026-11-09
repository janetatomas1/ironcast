find_package(GTest REQUIRED)
include(GoogleTest)

# One executable per test file, each registered with CTest. Add new test files to this list.
# Headless: no window or GL context, only an ImGui context with a font atlas.
set(
    IRONCAST_TESTS
    measure_test
    color_test
    series_test
    actionmanager_test
    container_test
)

foreach(test ${IRONCAST_TESTS})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE ironcast GTest::gtest_main)
    # Windows has no rpath: copy DLLs (SDL3) next to the test. Before gtest_discover_tests, which
    # runs the test right after linking.
    if(WIN32)
        add_custom_command(
            TARGET ${test} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${test}> $<TARGET_FILE_DIR:${test}>
            COMMAND_EXPAND_LISTS
        )
    endif()
    gtest_discover_tests(${test})
endforeach()
target_link_libraries(series_test PRIVATE ironcast::extras)

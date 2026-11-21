# ironcast_discover_tests(<target> [TIMEOUT <seconds>])
#
# Registers every test in <target>, an executable whose main() returns ironcast::testing::run(), with CTest:
# one CTest test per IRONCAST_TEST, named "category/name", each run in its own process. After each build,
# "<target> --list" is run and its output written to a file CTest reads, like gtest_discover_tests(). Call it
# after any other POST_BUILD step the executable needs (e.g. copying DLLs on Windows).
function(ironcast_discover_tests target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "TIMEOUT" "")
    if(NOT arg_TIMEOUT)
        set(arg_TIMEOUT 60)
    endif()

    set(tests_file "${CMAKE_CURRENT_BINARY_DIR}/${target}_tests.cmake")
    set(include_file "${CMAKE_CURRENT_BINARY_DIR}/${target}_tests_include.cmake")

    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:${target}>
            -DOUTPUT=${tests_file}
            -DTIMEOUT=${arg_TIMEOUT}
            -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ironcastDiscoverTests.cmake
        VERBATIM
    )

    # Read by CTest when it runs; the tests file exists once the target has been built.
    file(WRITE ${include_file} "if(EXISTS \"${tests_file}\")\n  include(\"${tests_file}\")\nendif()\n")
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${include_file})
endfunction()

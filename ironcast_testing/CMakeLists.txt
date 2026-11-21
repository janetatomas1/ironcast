
add_library(
    ironcast_testing
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ironcast/testing/test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ironcast/testing/runner.cpp
)
add_library(ironcast::testing ALIAS ironcast_testing)

target_link_libraries(ironcast_testing PUBLIC ironcast)
target_include_directories(
    ironcast_testing
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ironcast_discover_tests(), for tests/gui and any test executable in a project that adds ironcast.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ironcastTesting.cmake)

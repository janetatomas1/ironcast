# unit/: GoogleTest tests, one executable per file. gui/: Dear ImGui Test Engine tests. Both run with ctest.
if(IRONCAST_UNIT_TESTS)
    add_subdirectory(unit)
endif()
if(IRONCAST_GUI_TESTS)
    add_subdirectory(gui)
endif()

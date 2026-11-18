# unit/: GoogleTest tests, one executable per file. gui/: GUI tests (none yet). Both run with ctest.
if(IRONCAST_UNIT_TESTS)
    add_subdirectory(unit)
endif()
if(IRONCAST_GUI_TESTS)
    add_subdirectory(gui)
endif()

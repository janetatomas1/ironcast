# Run by ironcast_discover_tests() after each build (cmake -P) with EXECUTABLE, OUTPUT and TIMEOUT: writes
# one add_test() per line of "EXECUTABLE --list" to OUTPUT, for CTest.
execute_process(
    COMMAND ${EXECUTABLE} --list
    OUTPUT_VARIABLE listing
    ERROR_VARIABLE errors
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "${EXECUTABLE} --list failed (${result}):\n${errors}")
endif()

string(REPLACE "\n" ";" names "${listing}")
set(content "")
foreach(name IN LISTS names)
    string(STRIP "${name}" name)
    if(name STREQUAL "")
        continue()
    endif()
    string(APPEND content "add_test(\"${name}\" \"${EXECUTABLE}\" \"${name}\")\n")
    string(APPEND content "set_tests_properties(\"${name}\" PROPERTIES TIMEOUT ${TIMEOUT})\n")
endforeach()
file(WRITE ${OUTPUT} "${content}")

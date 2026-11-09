# Writes OUTPUT, a .cpp defining ironcast::embedded::NAME[] / NAME_size with the bytes of INPUT.
# Run with cmake -DINPUT=... -DOUTPUT=... -DNAME=... -P embed.cmake.
file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" length)
math(EXPR size "${length} / 2")
# 32 bytes per line, so the generated file stays readable for the compiler and diffs.
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
string(REGEX REPLACE "((0x..,){32})" "\\1\n" bytes "${bytes}")
file(WRITE "${OUTPUT}"
"// Generated from ${INPUT} by cmake/embed.cmake; do not edit.\n"
"#include <cstddef>\n\n"
"namespace ironcast::embedded {\n"
"extern const unsigned char ${NAME}[] = {\n${bytes}\n};\n"
"extern const std::size_t ${NAME}_size = ${size};\n"
"}\n")

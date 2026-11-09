#pragma once

// ironcast's assertions: libassert's, or no-ops on Emscripten, where ironcast is built without libassert.
#ifdef __EMSCRIPTEN__
#define IRONCAST_ASSERT(...) ((void)0)
#define IRONCAST_DEBUG_ASSERT(...) ((void)0)
#else
#include <libassert/assert.hpp>
#define IRONCAST_ASSERT(...) ASSERT(__VA_ARGS__)
#define IRONCAST_DEBUG_ASSERT(...) DEBUG_ASSERT(__VA_ARGS__)
#endif

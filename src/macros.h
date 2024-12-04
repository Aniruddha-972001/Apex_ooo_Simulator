#pragma once

#include <stdio.h>

#define DEBUG 1

#define DBG(tag, fmt, ...) if (DEBUG) { printf("[%s] " fmt "\n", tag, __VA_ARGS__); }
/* GLSL -> C shims for the BC transcoder shaders (the shader bodies are
 * written in the common GLSL/C subset, see panvk_bc_etc2.comp). */
#pragma once
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef unsigned int uint;
#define I(x) ((int)(x))
#define U(x) ((uint)(x))
#define F(x) ((float)(x))
#define GLOBAL static
#define CARR(type, name, n) static const type name[n] = {
#define CEND };
#define clamp(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define PANVK_BC_C_TEST 1

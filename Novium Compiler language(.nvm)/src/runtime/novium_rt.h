// ============================================================================
// runtime/novium_rt.h — Novium Runtime Library C ABI
// ============================================================================
//
// Defines the C-compatible runtime types and entry points used by
// compiled Novium programs and FFI bridges (C, Moji, Python).

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOVIUM_RT_VERSION "0.1.5"

// ── C ABI Type Mappings ──────────────────────────────────────────────────────
// Novium logical types map to fixed-width C types for ABI stability.
// These match the Novium SDK's include/novium.h mappings.

typedef int64_t  novium_int;
typedef double   novium_float;
typedef bool     novium_bool;
typedef struct { const char* data; uint64_t len; } novium_string;

// ── Runtime Allocator ────────────────────────────────────────────────────────
// Compiled Novium code allocates through the runtime so that FFI bridges
// (Moji, Python, C) can share a single allocation strategy.

void* novium_runtime_malloc(size_t size);
void  novium_runtime_free(void* ptr);
const char* novium_runtime_version(void);

#ifdef __cplusplus
}
#endif
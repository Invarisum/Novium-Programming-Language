#ifndef NOVIUM_ABI_H
#define NOVIUM_ABI_H

/*
 * Novium C ABI v1.  This header is deliberately C11-compatible so a future
 * Novium native backend can expose libraries to C, C++, Rust, Go (cgo), Zig,
 * Python ctypes, C#, Swift, and other FFI-capable languages without C++ name
 * mangling or unstable class layouts.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#define NOVIUM_EXTERN_C extern "C"
#else
#define NOVIUM_EXTERN_C extern
#endif

#if defined(_WIN32)
#define NOVIUM_API NOVIUM_EXTERN_C __declspec(dllimport)
#else
#define NOVIUM_API NOVIUM_EXTERN_C __attribute__((visibility("default")))
#endif

#define NOVIUM_ABI_VERSION 1u

typedef int32_t novium_status;
enum {
    NOVIUM_OK = 0,
    NOVIUM_ERROR_INVALID_ARGUMENT = 1,
    NOVIUM_ERROR_OUT_OF_MEMORY = 2,
    NOVIUM_ERROR_PANIC = 3,
    NOVIUM_ERROR_ABI_MISMATCH = 4
};

typedef struct novium_string_view {
    const char* data;
    size_t length;
} novium_string_view;

typedef struct novium_slice {
    const void* data;
    size_t length;
    size_t stride;
} novium_slice;

/* Every public Novium library should export this exact symbol. */
NOVIUM_API uint32_t novium_abi_version(void);

/* A future generated library owns errors; consumers copy the view if needed. */
NOVIUM_API novium_string_view novium_last_error(void);

#endif /* NOVIUM_ABI_H */

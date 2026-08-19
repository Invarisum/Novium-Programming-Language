// ============================================================================
// moji/novium_moji_bridge.h — Moji Cross-Language FFI Bridge Header
// ============================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdbool>
#include <string>

#include "runtime/novium_rt.h"

// ── Moji ABI Type Mappings ──────────────────────────────────────────────
// These map Moji types to Novium C ABI types for zero-overhead interop.

// Moji i32  → Novium i32 (int)
#define MOVIUM_MOJI_I32     int

// Moji i64  → Novium i64 (long long)
#define MOVIUM_MOJI_I64     long long

// Moji f32  → Novium f32 (float)
#define MOVIUM_MOJI_F32     float

// Moji f64  → Novium f64 (double)
#define MOVIUM_MOJI_F64     double

// Moji bool → Novium bool (uint8_t)
#define MOVIUM_MOJI_BOOL    uint8_t

// Moji void → Novium void
#define MOVIUM_MOJI_VOID    void

// ── Moji Function Signature ─────────────────────────────────────────────
// Represents a Moji function type for FFI declaration.

struct MojiFnSig {
    // Return type
    void* ret_type;  // Pointer to Novium type descriptor

    // Parameter count and types
    size_t arg_count;
    void** arg_types;  // Array of type descriptors

    // Calling convention hint
    bool is_cdecl;   // true = cdecl, false = default (platform)
};

// ── Moji Import Declaration ──────────────────────────────────────────────
// Declares a Moji function imported into Novium scope.

struct MojiImport {
    std::string name;       // Novium name for the function
    std::string moji_name;  // Original Moji function name (may differ)
    MojiFnSig sig;          // Function signature
    bool is_owned;          // true = Novium will finalize/cleanup

    // Runtime handle (populated at link time)
    void* runtime_handle;  // Platform-specific function pointer
};

// ── Moji Arena Allocation ────────────────────────────────────────────────
// Zero-copy arena for FFI data exchange between Novium and Moji.

struct MojiArena {
    // Arena-allocated buffer for FFI data
    uint8_t* buffer;
    size_t capacity;
    size_t offset;

    // Arena construction / destruction
    void* (*alloc)(size_t size);
    void (*free)(void* ptr);

    // Arena methods
    void* allocate(size_t size);
    void* allocate_aligned(size_t size, size_t align);
    void reset();
    bool exhausted() const;
};

// ── Moji FFI Bridge Functions ────────────────────────────────────────────
// Core bridge operations for Novium ⇄ Moji interop.

// Import a Moji function into Novium module
MojiImport moji_import(const char* moji_fn_name, const MojiFnSig* sig);

// Export a Novium function to Moji context
void moji_export(const char* novium_fn_name, void* fn_ptr, const MojiFnSig* sig);

// Allocate memory in shared arena for FFI data exchange
void* moji_arena_alloc(MojiArena* arena, size_t size);

// Free arena-allocated memory
void moji_arena_free(MojiArena* arena, void* ptr);

// Create a new FFI arena with custom allocator
MojiArena moji_arena_create(void* (*alloc)(size_t size), void (*free)(void* ptr));

// Dispose of an FFI arena
void moji_arena_destroy(MojiArena* arena);

// Bridge version info
const char* moji_bridge_version();

// Runtime version info
const char* moji_runtime_version();

// ── Utility Macros ───────────────────────────────────────────────────────
// Declaring Moji-imported functions

// Import a Moji function with auto-derived signature
// Usage: MOVIUM_MOJI_IMPORT(my_moji_fn, "moji_some_function")
#define MOVIUM_MOJI_IMPORT(novium_name, moji_name) \
    extern "Moji" novium_name;

// Export a Novium function to Moji
// Usage: MOVIUM_MOJI_EXPORT(novium_name, "moji_export_name")
#define MOVIUM_MOJI_EXPORT(novium_name, moji_name) \
    extern "Moji" novium_name;

// Shared arena for FFI data exchange (extern "Moji" module scope)
extern MojiArena moji_shared_arena;

// Alignment constant for FFI data
#define MOVIUM_MOJI_ALIGNMENT __alignof__(double)

// ── Inline Implementations (inline for header-only use) ─────────────────

inline void* MojiArena::allocate(size_t size) {
    if (offset + size > capacity) return nullptr;
    void* ptr = buffer + offset;
    offset += size;
    return ptr;
}

inline void* MojiArena::allocate_aligned(size_t size, size_t align) {
    // Align offset to 'align' boundary
    size_t aligned = (offset + align - 1) & ~(align - 1);
    if (aligned + size > capacity) return nullptr;
    offset = aligned + size;
    return buffer + aligned;
}

inline void MojiArena::reset() { offset = 0; }

inline bool MojiArena::exhausted() const { return offset >= capacity; }

inline const char* moji_bridge_version() { return "0.1.5"; }
inline const char* moji_runtime_version() { return NOVIUM_RT_VERSION; }
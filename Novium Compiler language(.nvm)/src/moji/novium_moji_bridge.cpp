// ============================================================================
// moji/novium_moji_bridge.cpp — Moji Cross-Language FFI Bridge Implementation
// ============================================================================

#include "novium_moji_bridge.h"
#include <cstring>
#include <cstdlib>

// ── MojiArena Implementation ─────────────────────────────────────────────

MojiArena moji_arena_create(void* (*alloc)(size_t size), void (*free)(void* ptr)) {
    MojiArena arena{};
    arena.alloc = alloc;
    arena.free = free;
    // Initial capacity: 4KB for FFI data exchange
    arena.capacity = 4096;
    arena.buffer = static_cast<uint8_t*>(arena.alloc(arena.capacity));
    arena.offset = 0;
    return arena;
}

void moji_arena_destroy(MojiArena* arena) {
    if (arena->buffer && arena->free) {
        arena->free(arena->buffer);
        arena->buffer = nullptr;
        arena->capacity = 0;
        arena->offset = 0;
    }
}

void* moji_arena_alloc(MojiArena* arena, size_t size) {
    if (arena->exhausted()) return nullptr;
    return arena->allocate(size);
}

void moji_arena_free(MojiArena* arena, void* ptr) {
    // Arena doesn't support individual freeing; reset entire arena
    (void)ptr;  // Suppress unused parameter
    arena->reset();
}

// ── MojiImport / MojiExport Implementation ───────────────────────────────

// Simplified import: records a Moji function declaration
// In a full implementation, this would register the function with the runtime
MojiImport moji_import(const char* moji_fn_name, const MojiFnSig* sig) {
    MojiImport imp{};
    imp.name = sig ? sig->ret_type ? "void" : "unknown" : "unnamed";
    imp.moji_name = moji_fn_name ? moji_fn_name : "unknown";
    imp.sig = sig ? *sig : MojiFnSig{};
    imp.is_owned = false;
    imp.runtime_handle = nullptr;
    return imp;
}

// Export a Novium function to Moji context
// In a full implementation, this would generate Moji-compatible FFI stub
void moji_export(const char* novium_fn_name, void* fn_ptr, const MojiFnSig* sig) {
    // Stub: in full implementation would generate Moji FFI wrapper
    (void)novium_fn_name;
    (void)fn_ptr;
    (void)sig;
}

// ── Bridge Version / Runtime Info ──────────────────────────────────────────
// moji_bridge_version() and moji_runtime_version() are defined inline in
// novium_moji_bridge.h — no redefinition here.

// ── Shared Arena (module-level) ───────────────────────────────────────────

// Exported for use in extern "Moji" module blocks
MojiArena moji_shared_arena;

// Default arena initializer (called once at module init)
// In full implementation, this would be called from runtime startup
void moji_bridge_init() {
    // Create shared arena with runtime allocator
    moji_shared_arena = moji_arena_create(
        novium_runtime_malloc,
        novium_runtime_free
    );
}

// ── Utility: Parse Moji type string to Novium type descriptor ─────────────
// In full implementation, this would convert Moji type names to Novium types

void* moji_type_to_novium(const char* moji_type) {
    if (!moji_type) return nullptr;
    
    if (strcmp(moji_type, "i32") == 0 || strcmp(moji_type, "int") == 0) {
        return reinterpret_cast<void*>(1);  // Novium i32 descriptor
    }
    if (strcmp(moji_type, "i64") == 0 || strcmp(moji_type, "long") == 0) {
        return reinterpret_cast<void*>(2);  // Novium i64 descriptor
    }
    if (strcmp(moji_type, "f32") == 0 || strcmp(moji_type, "float") == 0) {
        return reinterpret_cast<void*>(3);  // Novium f32 descriptor
    }
    if (strcmp(moji_type, "f64") == 0 || strcmp(moji_type, "double") == 0) {
        return reinterpret_cast<void*>(4);  // Novium f64 descriptor
    }
    if (strcmp(moji_type, "bool") == 0) {
        return reinterpret_cast<void*>(5);  // Novium bool descriptor
    }
    if (strcmp(moji_type, "void") == 0) {
        return reinterpret_cast<void*>(6);  // Novium void descriptor
    }
    
    return nullptr;
}
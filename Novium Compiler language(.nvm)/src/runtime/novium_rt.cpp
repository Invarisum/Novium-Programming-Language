// ============================================================================
// runtime/novium_rt.cpp — Novium Runtime Library Implementation
// ============================================================================

#include "runtime/novium_rt.h"

#include <cstdlib>

extern "C" {

void* novium_runtime_malloc(size_t size) {
    return std::malloc(size);
}

void novium_runtime_free(void* ptr) {
    std::free(ptr);
}

const char* novium_runtime_version(void) {
    return NOVIUM_RT_VERSION;
}

} // extern "C"
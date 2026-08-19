// ============================================================================
// hw_detector.h — Lightweight Hardware Detector for Novium
// ============================================================================
// Provides compile-time and runtime information about the target hardware:
// - CPU vendor and features (x86/x64 ARM extensions)
// - Core count and cache size
// - SIMD support (SSE/AVX/AVX-510 NEON)
// - Operating system page size and thread scheduling info
// - NUMA node information (if applicable)
// Design goals: header-only, no external dependencies, minimal overhead
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>

namespace novium {
namespace hw {

// ── CPU Vendor ──────────────────────────────────────────────────────────────
enum class CpuVendor {
    UNKNOWN,
    INTEL,
    AMD,
    ARM,
    MIPS
};

// ── CPU Feature Bits ────────────────────────────────────────────────────────
// Bit positions for CPU capabilities (matching common ISAs)
enum class CpuFeature : uint32_t {
    // x86-64 / x86
    SSE3          = (1 << 0),
    SSSE3         = (1 << 1),
    SSE4_1        = (1 << 2),
    SSE4_2        = (1 << 3),
    AVX           = (1 << 4),
    AVX2          = (1 << 5),
    AVX512F       = (1 << 6),
    AVX512DQ      = (1 << 7),
    AVX512PF      = (1 << 8),
    AVX512ER      = (1 << 9),
    AVX512CD      = (1 << 10),
    FMA           = (1 << 11),

    // ARM / NEON
    NEON          = (1 << 16),
    NEON_DOTPROD  = (1 << 17),

    // Generic
    FAST_MATH     = (1 << 20),
    FP16          = (1 << 21,
};

// ── Detected Hardware Information ───────────────────────────────────────────
struct CpuInfo {
    CpuVendor vendor = CpuVendor::UNKNOWN;
    std::string brand_string; // e.g. "GenuineIntel", "AMD Ryzen 7 5800H", "Apple M2"
    uint32_t core_count = 1;      // Physical cores
    uint32_t logical_count = 1;   // Logical/hyperthreads
    uint32_t l1_cache_kb = 0;     // Per core
    uint32_t l2_cache_kb = 0;     // Per core
    uint32_t l3_cache_kb = 0;     // Shared
    uint64_t max_freq_mhz = 0;    // Max turbo frequency
    std::array<CpuFeature, 32> features{}; // Detected feature bits

    // Runtime dispatching support
    bool has_sse() const { return features[static_cast<size_t>(CpuFeature::SSE3)]; }
    bool has_avx() const   { return features[static_cast<size_t>(CpuFeature::AVX)]; }
    bool has_avx2() const  { return features[static_cast<size_t>(CpuFeature::AVX2)]; }
    bool has_avx512() const { return features[static_cast<size_t>(CpuFeature::AVX512F)]; }
    bool has_neon() const  { return features[static_cast<size_t>(CpuFeature::NEON)]; }
};

// ── Operating System Information ────────────────────────────────────────────
struct OsInfo {
    std::string name;        // e.g. "Windows 10", "Ubuntu 22.04", "macOS 13"
    std::string release;     // e.g. "10.0.19044", "5.15.0-91-generic"
    std::string machine;     // e.g. "x86_64", "aarch64"
    uint32_t page_size = 4096; // Memory page size in bytes
    bool real_time_scheduling = false; // Can we get real-time priority?
};

// ── NUMA / Memory Topology ────────────────────────────────────────────────
struct NumaNode {
    int node_id = -1;
    uint64_t memory_bytes = 0; // Approximate local memory
    std::vector<int> cpu_ids; // CPUs in this node
};

// ── Lightweight Hardware Detection ────────────────────────────────────────
// Returns CPU information. Thread-safe, no malloc beyond small strings.
// Caller should cache the result if called frequently.
CpuInfo detect_cpu();

/// Returns operating system information.
OsInfo detect_os();

/// Returns NUMA node information for the current thread.
/// Returns empty vector if NUMA not available or not supported.
std::vector<NumaNode> detect_numa();

/// Query if the current program should target CPU, GPU, or NPU based on
/// hardware capabilities and configuration.
/// Default heuristic: multi-core CPU for general work, GPU for data-parallel,
/// NPU for ML/inference workloads.
enum class TargetAccelerator {
    CPU,
    GPU,
    NPU,
    AUTO // Let the detector decide based on workload profile
};
TargetAccelerator select_best_accelerator(
    const CpuInfo& cpu,
    const OsInfo& os,
    // Simple workload profile: "general", "data_parallel", "ml_inference"
    const std::string& workload_profile = "general"
);

} // namespace hw
} // namespace novium

// ============================================================================
// Inline implementations (MSVC and GCC/Clang compatible)
// ============================================================================

#ifdef _MSC_VER
    #include <intrin.h>
    #define NOVIUM_HW_HAS_RDSETP 1
#else
    #include <cpuid.h>
    #define NOVIUM_HW_HAS_RDSETP 0
#endif

// CPUID helper (simple, portable)
inline uint32_t get_cpuid(uint32_t eax, uint32_t ecx = 0) {
    uint32_t ebx, ecx_out, edx;
#if defined(_MSC_VER)
    __cpuid(&ebx, &ecx_out, &edx, eax); // MSVC intrinsic
#else
    __cpuid_count(eax, ecx, ebx, ecx_out, edx);
#endif
    // Return EAX in caller's eax, but we'll just use the function to set features
    // This is a simplified approach; real implementation would check bits
    return ebx; // placeholder
}

// Detect CPU features via CPUID and / or CPR (Control Registers)
inline void detect_cpu_features(std::array<novium::hw::CpuFeature, 32>& features) {
    // Default: mark nothing, platform-specific code will set bits
    // In a real implementation, we'd call CPUID with leaf 0x00000001 for feature bits
    // and leaf 0x00000007 for extended features (AVX/AVX-510)
    // For now, leave defaults; the OS/driver will populate via other means
}

// Simple Windows/Linux/ARM detection stubs — platforms should override
inline novium::hw::CpuInfo detect_cpu() {
    CpuInfo info;
    info.core_count = 1; // fallback
    info.logical_count = 1;
    // Attempt to read CPUID brand string (small buffer, safe)
    // brand_string left as empty fallback; real impl would use CPUID leaf 0x80000002-0x80000004
    return info;
}

inline novium::hw::OsInfo detect_os() {
    OsInfo info;
    info.page_size = 4096;
    return info;
}

inline std::vector<novium::hw::NumaNode> detect_numa() {
    return {}; // empty if not supported
}

inline novium::hw::TargetAccelerator select_best_accelerator(
    const novium::hw::CpuInfo& cpu,
    const novium::hw::OsInfo& os,
    const std::string& workload_profile
) {
    // Default: CPU for general-purpose; could be extended
    // Future: check cpu.has_avx512() && workload_profile == "data_parallel" -> GPU
    //          cpu.has_neon() && workload_profile == "ml_inference" -> NPU
    return novium::hw::TargetAccelerator::CPU;
}

// Helper to stringify CpuVendor
inline std::string vendor_to_string(novium::hw::CpuVendor v) {
    switch (v) {
        case novium::hw::CpuVendor::INTEL:  return "Intel";
        case novium::hw::CpuVendor::AMD:    return "AMD";
        case novium::hw::CpuVendor::ARM:    return "ARM";
        case novium::hw::CpuVendor::MIPS:   return "MIPS";
        default: return "Unknown";
    }
}

inline std::string feature_to_string(novium::hw::CpuFeature f) {
    switch (f) {
        case novium::hw::CpuFeature::SSE3:  return "SSE3";
        case novium::hw::CpuFeature::SSSE3: return "SSSE3";
        case novium::hw::CpuFeature::SSE4_1: return "SSE4.1";
        case novium::hw::CpuFeature::SSE4_2: return "SSE4.2";
        case novium::hw::CpuFeature::AVX:   return "AVX";
        case novium::hw::CpuFeature::AVX2:  return "AVX2";
        case novium::hw::CpuFeature::AVX512F: return "AVX-512F";
        case novium::hw::CpuFeature::NEON:  return "NEON";
        default: return "Unknown";
    }
}
#else
// GCC / Clang / ARM compilers — use builtin __get_cpuid or asm
// For now, delegate to the same inline stubs; real impl would use __builtin_cpu_id etc.
inline namespace {
    // Minimal stubs — same as MSVC path for now
}
#endif // _MSC_VER
// ============================================================================
// hw_opt_engine.h — Hardware-Optimized Code Generation Engine for Novium
// ============================================================================
// Purpose: Given detected hardware characteristics, emit optimized IR / code
// patterns for the target. Supports CPU (x86/x64/AArch64), GPU (SIMD/DataParallel),
// and NPU (ML inference) code generation.
// ============================================================================

#pragma once

#include "hardware/hw_detector.h"
#include <string>
#include <vector>
#include <memory>

namespace novium {
namespace opt {

// ── Optimization Profiles ───────────────────────────────────────────────────
// Predefined optimization strategies for different hardware classes
enum class OptimizationProfile {
    // General-purpose: balance between code size and speed
    BALANCED,
    // Maximum performance: aggressive inlining, unrolling, vectorization
    PERFORMANCE,
    // Minimum code size: reduce binary footprint
    SIZE,
    // Data-parallel: for GPU / SIMD workloads
    DATA_PARALLEL,
    // ML inference: optimized for tensor operations, low latency
    ML_INFERENCE,
};

// ── Target Accelerator ──────────────────────────────────────────────────────
// The class of hardware we're optimizing for
enum class TargetAccelerator {
    CPU,
    GPU,
    NPU,
    // Auto-detect based on hardware and workload
    AUTO
};

// ── Compilation Output Description ──────────────────────────────────────────
// What the optimization engine produces
struct CompilationOutput {
    // Optimized Novium IR (internal representation)
    std::string ir; // could be text or binary format

    // Target-specific flags / pragma strings
    std::string compiler_flags; // e.g., "-mavx2 -O3" or CUDA flags

    // Memory layout hints for the runtime
    struct MemoryHints {
        // Preferred alignment for arrays (bytes)
        size_t alignment = 64;
        // NUMA node preference (if known)
        int numa_node = -1;
        // Thread count suggestion
        size_t suggested_threads = 0;
    } memory_hints;

    // Whether the output is safe to execute on the target
    bool is_safe = true;

    // Diagnostic notes
    std::vector<std::string> notes;
};

// ── Optimization Engine ─────────────────────────────────────────────────────
class OptimizationEngine {
public:
    OptimizationEngine() = default;

    // Initialize with detected hardware. If hw is null, self-detects.
    explicit OptimizationEngine(const novium::hw::CpuInfo* hw = nullptr,
                              const novium::hw::OsInfo* os = nullptr);

    // Optimize the given Novium IR for the detected hardware.
    // Returns optimized IR and associated flags/output.
    CompilationOutput optimize(const std::string& ir,
                              const std::string& workload_type = "general");

    // Set the optimization profile (BALANCED, PERFORMANCE, SIZE, etc.)
    void set_profile(OptimizationProfile profile);

    // Set the target accelerator (CPU, GPU, NPU, AUTO)
    void set_target_accelerator(TargetAccelerator accel);

    // Get the currently detected hardware info (const reference)
    const novium::hw::CpuInfo& cpu_info() const { return cpu_info_; }
    const novium::hw::OsInfo& os_info() const { return os_info_; }

private:
    // Apply CPU-specific optimizations (instruction selection, vectorization)
    std::string apply_cpu_opt(const std::string& ir);

    // Apply GPU-specific optimizations (SIMD expansion, work-group mapping)
    std::string apply_gpu_opt(const std::string& ir);

    // Apply NPU-specific optimizations (layer fusion, quantized ops)
    std::string apply_npu_opt(const std::string& ir);

    // Apply size-reduced optimizations (dead code elimination, constant folding)
    std::string apply_size_opt(const std::string& ir);

    // Merge optimization passes based on profile
    std::string apply_profile_opts(const std::string& ir);

    // Hardware contexts
    novium::hw::CpuInfo cpu_info_;
    novium::hw::OsInfo os_info_;
    OptimizationProfile profile_ = OptimizationProfile::BALANCED;
    TargetAccelerator target_ = TargetAccelerator::AUTO;
};

// ── Lightweight Compilation Pipeline Hook ───────────────────────────────────
// This is the interface the Novium compiler frontend/back-end can call.
// The engine takes Novium IR (or AST text) and returns optimized output.

// Entry point: optimize a Novium module for the target hardware.
// ir: the Novium IR text (or AST dump)
// returns optimized IR + flags/notes
CompilationOutput run_optimization_pipeline(
    const std::string& ir,
    const novium::hw::CpuInfo* cpu = nullptr,
    const novium::hw::OsInfo* os = nullptr,
    OptimizationProfile profile = OptimizationProfile::BALANCED,
    novium::opt::TargetAccelerator target = novium::opt::TargetAccelerator::AUTO
);

// Convenience: auto-detect hardware and run optimization
CompilationOutput auto_optimize(
    const std::string& ir,
    const std::string& workload_type = "general");

// ── Profile-to-Flag Mappers ────────────────────────────────────────────────
// Convert optimization profile to compiler-specific flags/strings

// CPU flags mapping
inline std::string profile_to_cpu_flags(OptimizationProfile profile) {
    switch (profile) {
        case OptimizationProfile::BALANCED:
            return "-O2 -march=native";
        case OptimizationProfile::PERFORMANCE:
            return "-O3 -march=native -mfma -mavx2";
        case OptimizationProfile::SIZE:
            return "-Os -march=native";
        case OptimizationProfile::DATA_PARALLEL:
            return "-O2 -mavx2 -mfma -mf16c";
        case OptimizationProfile::ML_INFERENCE:
            return "-O2 -mavx2 -mfma -mf16c -mfp16";
        default:
            return "-O2";
    }
}

// GPU/SIMD flags mapping
inline std::string profile_to_gpu_flags(OptimizationProfile profile) {
    switch (profile) {
        case OptimizationProfile::DATA_PARALLEL:
            return "--gpu-arch=compute_86 -Xclang -fmix-math -Xclang -fsave-math-syntax";
        case OptimizationProfile::ML_INFERENCE:
            return "--gpu-arch=compute_89 -Xclang -fuse-ld=gold";
        default:
            return "--gpu-arch=compute_86";
    }
}

// NPU flags mapping (placeholder for future expansion)
inline std::string profile_to_npu_flags(OptimizationProfile profile) {
    // NPU support is emerging; return empty for now
    return "";
}

} // namespace opt
} // namespace novium
// =============================================================================
// hardware-detector.d - Novium Hardware Detector Documentation
// =============================================================================
// Version: 1.0.0
// Part of the Novium Language Ecosystem
// Auto-detects CPU, GPU, RAM, Storage, OS for adaptive toolchain configuration
// =============================================================================

/**
 * HARDWARE DETECTOR - Documentation
 * 
 * Overview:
 *   The HardwareDetector class auto-detects the host system's hardware
 *   specifications including CPU, GPU, RAM, storage, OS, and environment
 *   (container/virtualization). This profile is used by the entire Novium
 *   adaptive toolchain to auto-configure builds, optimize compiler flags,
 *   and make intelligent targeting decisions (CUDA vs native, etc.).
 * 
 * Key Features:
 *   - Comprehensive CPU detection (cores, threads, model, frequency, cache, SIMD)
 *   - GPU detection (NVIDIA, AMD, Intel, Apple Silicon)
 *   - Memory profiling (total, available, speed, channels)
 *   - Storage type and speed detection
 *   - OS and architecture detection
 *   - Container and virtualization awareness
 *   - Singleton pattern for consistent profiling
 *   - Optimal flag generation for compiler configuration
 * ==========================================================================*/

// ============================================================================
// HardwareProfile Interface
// ============================================================================
/**
 * Interface representing the complete hardware profile detected by HardwareDetector.
 * 
 * This profile is used throughout the Novium ecosystem to:
 *   - Auto-configure build targets (native/cuda/gpu/wasm)
 *   - Generate optimal compiler flags
 *   - Determine parallel job counts
 *   - Allocate optimal memory for builds
 *   - Select optimal CUDA architectures
 *   - Generate hardware-specific lockfile fingerprints
 * ==========================================================================*/

// CPU Sub-Profile:
//   - cores: number        - Number of physical CPU cores
//        Determined from require('os').cpus().length
//   - threads: number      - Number of logical threads
//        May include hyperthreading (same as cores if HT not enabled)
//   - model: string        - CPU model/brand string
//        e.g., "Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz"
//        Obtained from cpus[0]?.model
//   - frequencyMHz: number - Base frequency in MHz
//        Math.round(cpus[0]?.speed || 0)
//   - cacheL1KB: number    - L1 cache size in KB
//        Hardcoded default (32KB) - would detect from CPUID
//   - cacheL2KB: number    - L2 cache size in KB
//        Hardcoded default (256KB) - would detect from CPUID
//   - cacheL3KB: number    - L3 cache size in KB
//        Hardcoded default (8MB) - would detect from CPUID
//   - simd: string[]       - Detected SIMD extensions
//        // ['SSE', 'SSE2', 'SSE3', 'SSSE3', 'SSE4.1', 'SSE4.2', 'AVX', 'AVX2']
//        // Plus: AVX-512F, AVX-512VL, AVX-512BW, AVX-512DQ if supported
//        // ARM: NEON, SVE, SVE2
 //        // RISC-V: V (vector)
//   - architecture: 'x86_64' | 'arm64' | 'riscv64'
//        Detected from process.arch:
//        process.arch === 'x64' ? 'x86_64' :
//        process.arch === 'arm64' ? 'arm64' : 'riscv64'

// GPU Sub-Profile (optional):
//   - vendor: 'nvidia' | 'amd' | 'intel' | 'apple' | 'unknown'
//        GPU vendor identification
//        Detected via nvidia-smi, rocm-smi, clinfo, system_profiler
//   - model: string        - GPU model string
//        e.g., "GeForce GTX 1650"
//   - vramGB: number       - VRAM in gigabytes
//        e.g., 4, 8, 16, 24
//   - cudaCores?: number   - Number of CUDA cores (NVIDIA only)
//        e.g., 1024, 2048, 4096
//   - computeCapability?: string
//        // e.g., 'sm_86' (NVIDIA Turing), 'sm_89' (Ada Lovelace)
//        // Used for -arch flag in NVCC compilation
//   - openclVersion?: string   - OpenCL version support
//        e.g., "1.2", "2.0", "3.0"
//   - metalVersion?: string    - Metal/API version (Apple Silicon)
//        e.g., "metal_2_0"
//   - driverVersion?: string   - GPU driver version
//        e.g., "535.104.05"

// Memory Sub-Profile:
//   - totalGB: number      - Total system RAM in GB
//        Math.round(totalMem / (1024 ** 3) * 100) / 100
//   - availableGB: number  - Available/free RAM in GB
//        Math.round(freeMem / (1024 ** 3) * 100) / 100
//   - speedMTs: number     - Memory speed in MT/s (MegaTransfers per second)
//        Hardcoded default (3200) - would read from SMBIOS/DMI
//   - channels: number     - Memory channel count
//        Hardcoded default (2) - would detect from memory controller

// Storage Sub-Profile:
//   - type: 'nvme' | 'ssd' | 'hdd' | 'unknown'
//        Storage device type detection
//        // Would check: df -h, lsblk, diskutil, Get-PhysicalDisk
//   - freeGB: number       - Free storage space in GB
//        Hardcoded default (100) - would read from filesystem
//   - ioSpeedMBps: number  - Sequential IO speed in MB/s
//        Hardcoded default (3500) - would read from fio, hdparm

// OS Sub-Profile:
//   - platform: 'linux' | 'darwin' | 'win32' | 'freebsd' | 'unknown'
//        Detected from process.platform:
//        if (platform === 'linux') -> 'linux'
//        if (platform === 'darwin') -> 'darwin'
//        if (platform === 'win32') -> 'win32'
//        if (platform === 'freebsd') -> 'freebsd'
//   - arch: 'x64' | 'arm64' | 'riscv64' | 'unknown'
//        Detected from process.arch:
//        process.arch === 'x64' ? 'x64' :
//        process.arch === 'arm64' ? 'arm64' : 'unknown'
//   - kernelVersion: string - OS kernel version
//        require('os').release() // uname -r equivalent
//   - distribution?: string // e.g., "Ubuntu 22.04"
//        // Would read /etc/os-release on Linux

// Container Sub-Profile (optional):
//   - runtime: 'docker' | 'podman' | 'containerd' | 'kubernetes' | 'none'
//        Container runtime detection
//        // Checks: /.dockerenv file, /proc/1/cgroup, KUBERNETES_SERVICE_HOST env var
//   - cpus?: number        - Limited CPU count (if constrained)
//   - memoryGB?: number    - Limited memory (if constrained)

// Virtualization Sub-Profile (optional):
//   - type: 'vmware' | 'virtualbox' | 'hyperv' | 'kvm' | 'xen' | 'none'
//        Virtualization type detection
//        // Would check: /proc/cpuinfo hypervisor flag, dmidecode,
//        // systemd-detect-virt, CPUID hypervisor bit
//   - hostCpus?: number    - Host physical CPU count

// ============================================================================
// HardwareDetector Class
// ============================================================================
/**
 * HardwareDetector - Main class for hardware detection
 * ==========================================================================*/
/**
 * Class: HardwareDetector
 * 
 * Description:
 *   Detects and profiles the host system's hardware specifications.
 *   Uses singleton pattern via getHardwareDetector() function.
 *   
 * Detection Methods:
 * ==========================================================================*/
//   - detect(): HardwareProfile
 //     * Main entry point
 //     * Returns cached profile if already detected (performance optimization)
 //     * Otherwise detects all hardware components and caches result
 //
 //   - detectCPU(): HardwareProfile['cpu']
 //     * Detects CPU specifications
 //     * Uses require('os').cpus() for model, cores, threads, frequency
 //     * Calls detectSIMD() for SIMD extension detection
 //     * Determines architecture from process.arch
 //
 //   - detectSIMD(): string[]
 //     * Detects SIMD (Single Instruction, Multiple Data) extensions
 //     * Returns array of supported SIMD instruction sets
 //     // Common x86: SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, AVX-512F...
 //     // Common ARM: NEON, SVE, SVE2
 //     // Common RISC-V: V (vector)
 //     // Currently returns: ['SSE', 'SSE2', 'SSE3', 'SSSE3', 'SSE4.1', 'SSE4.2', 'AVX', 'AVX2']
 //
 //   - detectGPU(): HardwareProfile['gpu'] | undefined
 //     * Detects GPU specifications
 //     * Would query: nvidia-smi (NVIDIA), rocm-smi/clinfo (AMD),
 //       intel_gpu_top/clinfo (Intel), system_profiler (Apple Silicon)
 //     * Mock detection returns undefined for now
 //     * In production: would use lspci, glxinfo, vulkaninfo, etc.
 //
 //   - detectMemory(): HardwareProfile['memory']
 //     * Detects memory specifications
 //     * Uses require('os').totalmem() and require('os').freemem()
 //     * Calculates total and available GB
 //     * Returns speed and channel count (would read from SMBIOS)
 //
 //   - detectStorage(): HardwareProfile['storage']
 //     * Detects storage specifications
 //     * Would check: df -h, lsblk, diskutil, SMBIOS/DMI, fio benchmarks
 //     * Returns type, free space, and IO speed
 //
 //   - detectOS(): HardwareProfile['os']
 //     * Detects operating system specifications
 //     * Reads process.platform, process.arch, require('os').release()
 //     // Reads /etc/os-release for distribution on Linux
 //
 //   - detectContainer(): HardwareProfile['container']
 //     * Checks for container environment
 //     // Checks: /.dockerenv file, /proc/1/cgroup, KUBERNETES_SERVICE_HOST
 //     // Returns { runtime: 'none' } if not in container
 //
 //   - detectVirtualization(): HardwareProfile['virtualization']
 //     * Checks for virtual machine environment
 //     // Would check: /proc/cpuinfo hypervisor flag, dmidecode,
 //     // systemd-detect-virt, CPUID hypervisor bit
 //     // Returns { type: 'none' } if not virtualized

 //   - getOptimalCompilerFlags(): string[]
 //     * Generates optimal compiler flags for the detected hardware
 //     * CPU-specific flags:
 //       - If AVX2: '-mavx2'
 //       - If AVX-512F: '-mavx512f -mavx512vl -mavx512bw -mavx512dq'
 //       - If AVX: '-mavx'
 //       - If SSE4.2: '-msse4.2'
 //       - If NEON: '-mfpu=neon'
 //     * Architecture-specific:
 //       - x86_64: '-march=native -mtune=native'
 //       - arm64: '-march=armv8-a -mtune=cortex-a76'
 //     * Link-time optimization: '-flto=thin'
 //     * GPU-specific:
 //       - If NVIDIA with computeCapability: '-arch=<computeCapability>'
 //
 //   - getOptimalParallelJobs(): number
 //     * Gets optimal number of parallel build jobs
 //     // Returns Math.max(1, cpus.threads - 1) // Leave 1 core free
 //
 //   - getOptimalBuildMemoryGB(): number
 //     * Gets optimal memory allocation for builds
 //     // Returns Math.max(2, Math.floor(availableGB * 0.75)) // 75% of available, min 2GB
 //
 //   - getOptimalCUDAArch(): string
 //     * Gets optimal CUDA architecture for the detected GPU
 //     // If GPU with computeCapability: returns that capability
 //     // Default: 'sm_86' (Ampere, RTX 30 series) as safe default

 // ============================================================================
 // Singleton Pattern
 // ============================================================================
/**
 * Singleton Implementation:
 * ==========================================================================*/
/**
 * let detectorInstance: HardwareDetector | null = null;
 * 
 * export function getHardwareDetector(): HardwareDetector {
 //   if (!detectorInstance) {
 //     detectorInstance = new HardwareDetector();
 //   }
 //   return detectorInstance;
 // }
 // 
 * Behavior:
 *   - Ensures only one HardwareDetector instance exists
 //   - Cache persists across calls for performance
 //   - First call performs detection, subsequent calls return cached result
 //   - Useful for extension lifetime (single detection needed)

// ============================================================================
// Optimal Flag Generation Details
// ============================================================================
/**
 * getOptimalCompilerFlags() Algorithm:
// ==========================================================================*/
//   1. Initialize empty flags array
//   2. CPU optimization:
//      - If simd includes 'AVX512F': add -mavx512f, -mavx512vl, -mavx512bw, -mavx512dq
 //      - Else if simd includes 'AVX2': add -mavx2
 //      - Else if simd includes 'AVX': add -mavx
 //      - If simd includes 'SSE4.2': add -msse4.2
 //      - If simd includes 'NEON': add -mfpu=neon
//   3. Architecture-specific:
//      - If architecture === 'x86_64': add -march=native -mtune=native
 //      - Else if architecture === 'arm64': add -march=armv8-a -mtune=cortex-a76
//   4. Link-time optimization: add -flto=thin
//   5. GPU-specific (if gpu detected):
 //      - If gpu.vendor === 'nvidia' && gpu.computeCapability:
//          add `-arch=${gpu.computeCapability}`
//   6. Return flags array

 // Example Output:
//   ['-mavx2', '-march=native', '-mtune=native', '-flto=thin']
 //   or for CUDA:
//   ['-mavx2', '-flto=thin', '-arch=sm_86']

// ============================================================================
// Memory Allocation for Builds
// ============================================================================
/**
 * getOptimalBuildMemoryGB() Algorithm:
// ==========================================================================*/
//   1. Get available memory from hardware profile
//   2. Calculate 75% of available memory
//   3. Apply minimum floor of 2GB
 //   // Math.max(2, Math.floor(availableGB * 0.75))
//   4. Return result in GB

 // Example:
//   - availableGB: 8 -> Math.max(2, Math.floor(8 * 0.75)) = Math.max(2, 6) = 6GB
 //   - availableGB: 16 -> Math.max(2, Math.floor(16 * 0.75)) = Math.max(2, 12) = 12GB
 //   - availableGB: 4 -> Math.max(2, Math.floor(4 * 0.75)) = Math.max(2, 3) = 3GB
 //   - availableGB: 1 -> Math.max(2, Math.floor(1 * 0.75)) = Math.max(2, 0) = 2GB (minimum)

// ============================================================================
// CUDA Architecture Selection
// ============================================================================
/**
 * getOptimalCUDAArch() Algorithm:
// ==========================================================================*/
//   1. Check if GPU detected with computeCapability
//   2. If yes: return that computeCapability string (e.g., 'sm_86')
//   3. If no GPU or no computeCapability: return default 'sm_86'
//   
 // Default Rationale:
//   - 'sm_86' = NVIDIA Ampere architecture (RTX 30 series)
//   - Supports a wide range of modern NVIDIA GPUs
 //   - Good balance of compatibility and feature availability
 //   - Can be overridden by user configuration
 
// ============================================================================
// Environment Detection Summary
// ============================================================================
/*
 * Detection Priority and Fallback:
// ==========================================================================*/
//   - CPU: Always detected successfully (os.cpus() always available)
//   - GPU: May return undefined if detection methods not available
 //     * Falls back to undefined - treated as "no GPU" (native target)
//   - Memory: Always detected (os.totalmem() always available)
//   - Storage: May have default values if filesystem queries fail
 //   - OS: Always detected (process.platform always set)
//   - Architecture: Always detected (process.arch always set)
//   - Container: Returns 'none' if not in container
 //   - Virtualization: Returns 'none' if not virtualized

 // Example Full Profile:
//   {
//     cpu: {
//       cores: 8,
//       threads: 8,
//       model: "Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz",
//       frequencyMHz: 2600,
//       cacheL1KB: 32,
//       cacheL2KB: 256,
//       cacheL3KB: 8192,
//       simd: ["SSE", "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2", "AVX", "AVX2"],
//       architecture: "x86_64"
//     },
//     memory: {
//       totalGB: 16,
//       availableGB: 8,
//       speedMTs: 3200,
//       channels: 2
//     },
//     storage: {
//       type: "nvme",
//       freeGB: 512,
//       ioSpeedMBps: 3500
//     },
//     os: {
//       platform: "win32",
//       arch: "x64",
//       kernelVersion: "10.0.19045",
//       distribution: undefined
//     }
//   }

// ============================================================================
// Integration with Toolchain
// ============================================================================
/*
 * Usage Across Novium Ecosystem:
// ==========================================================================*/
//   - AutoConfigurator.generateConfig(hardware) -> NoviumProjectConfig
 //   - BuildOptimizer.build() -> uses hardware profile for parallelism/flags
 //   - ErrorCorrector.correctAndCompile() -> uses hardware for fixes
 //   - NoviumCLI commands -> use hardware for auto-configuration
 //   - CUDA target detection -> hardware.gpu?.computeCapability
 //   - GPU/CPU target selection -> hardware.gpu?.vendor
 //   - Optimization level determination -> hardware.memory.totalGB < 8 ? 'size' : 'speed'
// ============================================================================
// Related Files
// ============================================================================
// Source:
//   - novium-adaptive-extension/src/hardware-detector.ts  # Implementation
 //
 // Usage In:
//   - novium-adaptive-extension/src/auto-configurator.ts  # generateConfig()
 //   - novium-adaptive-extension/src/diagnostic-engine.ts  # checks
 //   - novium-adaptive-extension/src/build-optimizer.ts    # flags/jobs/memory
 //   - novium-cli/main.go                                # Go CLI hardware detection
 //   - novium-adaptive-extension/src/extension.ts          # extension activation
 //
 // Documentation:
//   - .documentation/hardware-detector.d                 # This file

 // ============================================================================
 // Version History
 // ============================================================================
/*
 * Version 1.0.0 (2026-08-18)
 *   - Initial release
 //   - Full CPU detection implemented
 //   - GPU detection (mock, extensible)
 //   - Memory detection implemented
 //   - Storage detection (mock)
 //   - OS detection implemented
 //   - Container detection implemented
 //   - Virtualization detection implemented
 //   - SIMD extension detection
 //   - Architecture detection
 //   - Singleton pattern via getHardwareDetector()
 //   - Optimal flag generation
 //   - Parallel job calculation
 //   - Build memory allocation
 //   - CUDA architecture selection
 //   - Integration with all toolchain components

 // ============================================================================
 // Known Limitations
 // ============================================================================
/*
 * - GPU detection is mock/placeholder in initial version
 //   - Full detection would require nvidia-smi, rocm-smi, clinfo binaries
 //   - May not be available in all environments (Docker, CI, headless)
 // - Cache sizes hardcoded as defaults (would use CPUID in production)
 // - Memory speed/channels from SMBIOS may not be accessible in all OS
 // - Container/virtualization detection may have false positives/negatives
 // - Some SIMD extensions may not be detected without cpuid instruction
 // - Architecture detection limited to process.arch (may not detect ARM64 emulation)

// ============================================================================
// Requirements
// ============================================================================
/*
 * Node.js Environment:
//   - Node.js 16+ (for os module, crypto module used)
//   - Operating system with /proc/cpuinfo (Linux), SMBIOS (Windows/macOS)
//   - Administrator/contributor privileges for some hardware queries
 //
 * Novium Integration:
//   - Hardware profile used by: AutoConfigurator, BuildOptimizer, ErrorCorrector
 //   - Detection must complete before project configuration generation
 //   - Singleton pattern ensures consistent profiling across extension lifetime

// ============================================================================
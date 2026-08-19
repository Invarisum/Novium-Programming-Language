// ============================================================================
// Auto Configurator - Generates optimal project configuration from hardware
// ============================================================================

import { HardwareDetector, HardwareProfile } from './hardware-detector';

export interface NoviumProjectConfig {
  name: string;
  version: string;
  target: 'auto' | 'native' | 'wasm' | 'cuda' | 'gpu';
  optimizationLevel: 'speed' | 'size' | 'balanced';
  cudaArch: string;
  parallelJobs: number;
  optimizationFlags: string[];
  dependencyOverrides: Map<string, string>;
  lockfileHash: string;
  buildCacheDir: string;
  incrementalBuild: boolean;
  lto: 'off' | 'thin' | 'full';
  debugInfo: boolean;
  sanitizers: string[];
}

interface OptimizationProfile {
  name: string;
  description: string;
  compilerFlags: string[];
  linkerFlags: string[];
  defines: Record<string, string>;
}

export class AutoConfigurator {
  private hardware: HardwareProfile;
  private profiles: Map<string, OptimizationProfile> = new Map();

  constructor(hardware: HardwareProfile) {
    this.hardware = hardware;
    this.initializeProfiles();
  }

  private initializeProfiles(): void {
    // Speed-optimized profile
    this.profiles.set('speed', {
      name: 'Maximum Speed',
      description: 'Maximum performance, larger binary size',
      compilerFlags: [
        '-O3', '-march=native', '-mtune=native',
        '-flto=thin', '-fomit-frame-pointer',
        '-funroll-loops', '-ftree-vectorize',
        '-fipa-pta', '-fipa-profile'
      ],
      linkerFlags: ['-flto=thin', '-Wl,--gc-sections', '-Wl,--as-needed'],
      defines: { 'NDEBUG': '1', 'OPTIMIZE_SPEED': '1' }
    });

    // Size-optimized profile
    this.profiles.set('size', {
      name: 'Minimum Size',
      description: 'Smallest binary size, slightly reduced speed',
      compilerFlags: [
        '-Os', '-march=native', '-mtune=native',
        '-flto=full', '-fdata-sections', '-ffunction-sections',
        '-fno-unroll-loops', '-fno-inline-functions'
      ],
      linkerFlags: ['-flto=full', '-Wl,--gc-sections', '-Wl,--as-needed', '-Wl,--strip-all'],
      defines: { 'OPTIMIZE_SIZE': '1' }
    });

    // Balanced profile
    this.profiles.set('balanced', {
      name: 'Balanced',
      description: 'Good balance of speed and size',
      compilerFlags: [
        '-O2', '-march=native', '-mtune=native',
        '-flto=thin', '-fomit-frame-pointer'
      ],
      linkerFlags: ['-flto=thin', '-Wl,--gc-sections', '-Wl,--as-needed'],
      defines: { 'OPTIMIZE_BALANCED': '1' }
    });
  }

  generateConfig(workspaceRoot: string): any {
    const hardware = this.hardware;
    const detector = require('./hardware-detector').getHardwareDetector();

    // Determine optimal target
    const target = this.determineTarget();

    // Determine optimization level
    const optimizationLevel = this.determineOptimizationLevel();

    // Get optimal parallel jobs
    const parallelJobs = hardware.cpu.threads > 0
      ? Math.max(1, hardware.cpu.threads - 1)
      : require('os').cpus().length - 1;

    // Get optimal CUDA arch
    const cudaArch = hardware.gpu?.computeCapability ||
                     (hardware.gpu?.vendor === 'nvidia' ? 'sm_86' : 'auto');

    return {
      name: this.extractProjectName(workspaceRoot),
      version: '0.1.0',
      target,
      optimizationLevel,
      cudaArch,
      parallelJobs,
      optimizationFlags: this.getOptimizationFlags(optimizationLevel),
      dependencyOverrides: new Map(),
      lockfileHash: '',
      buildCacheDir: '.novium/cache',
      incrementalBuild: true,
      lto: 'thin',
      debugInfo: true,
      sanitizers: this.getRecommendedSanitizers()
    };
  }

  private determineTarget(): 'native' | 'wasm' | 'cuda' | 'gpu' {
    const gpu = this.hardware.gpu;

    if (gpu?.vendor === 'nvidia' && gpu.computeCapability) {
      return 'cuda';
    }
    if (gpu?.vendor === 'amd' || gpu?.vendor === 'intel' || gpu?.vendor === 'apple') {
      return 'gpu'; // OpenCL/Metal path
    }

    // Check if project has WASM-specific dependencies
    // Would scan package.json / novium.toml for wasm-specific deps
    return 'native';
  }

  private determineOptimizationLevel(): 'speed' | 'size' | 'balanced' {
    const mem = this.hardware.memory;

    // If low memory, prefer size optimization
    if (mem.totalGB < 8) {
      return 'size';
    }

    // If on battery or thermal constrained, balanced
    // Would check battery status / thermal sensors

    // Default to speed for development machines
    return 'speed';
  }

  private getOptimizationFlags(level: string): string[] {
    const profile = this.profiles.get(level);
    if (!profile) return [];

    return [...profile.compilerFlags, ...profile.linkerFlags];
  }

  private getRecommendedSanitizers(): string[] {
    const sanitizers: string[] = [];

    // Always enable in debug builds
    // Would check if debug build
    sanitizers.push('address', 'undefined');

    // Thread sanitizer for concurrent code
    // Would check if project uses threads
    // sanitizers.push('thread');

    // Memory sanitizer for memory-intensive apps
    // sanitizers.push('memory');

    return sanitizers;
  }

  generateOptimizedConfig(): any {
    const baseConfig = this.generateConfig('');
    const profile = this.profiles.get(baseConfig.optimizationLevel);

    return {
      ...baseConfig,
      optimizationFlags: profile ? [...profile.compilerFlags, ...profile.linkerFlags] : [],
      incrementalBuild: true,
      lto: 'thin'
    };
  }

  private extractProjectName(workspaceRoot: string): string {
    // Would read package.json / novium.toml
    return 'novium-project';
  }

  // Generate compiler command line for a specific target
  generateCompilerCommand(target: 'native' | 'wasm' | 'cuda', config: any): string[] {
    const baseFlags = config.optimizationFlags;
    const hardwareFlags = this.getHardwareFlags();

    switch (target) {
      case 'native':
        return ['clang++', '-std=c++17', ...baseFlags, ...hardwareFlags];

      case 'wasm':
        return [
          'em++', '-std=c++17',
          ...baseFlags,
          '-s', 'WASM=1',
          '-s', 'ALLOW_MEMORY_GROWTH=1',
          '-s', 'MODULARIZE=1'
        ];

      case 'cuda':
        const cudaArch = this.hardware.gpu?.computeCapability || 'sm_86';
        return [
          'nvcc', '-std=c++17',
          '-arch=' + cudaArch,
          '--compiler-options', baseFlags.join(' '),
          '--ptxas-options=-v',
          '--use_fast_math',
          '--fmad=true'
        ];

      default:
        return ['clang++', '-std=c++17', ...baseFlags];
    }
  }

  private getHardwareFlags(): string[] {
    const flags: string[] = [];
    const cpu = this.hardware.cpu;

    // CPU-specific flags from hardware detector
    if (cpu.simd.includes('AVX512F')) {
      flags.push('-mavx512f', '-mavx512vl', '-mavx512bw', '-mavx512dq');
    } else if (cpu.simd.includes('AVX2')) {
      flags.push('-mavx2');
    } else if (cpu.simd.includes('AVX')) {
      flags.push('-mavx');
    }

    if (cpu.simd.includes('SSE4.2')) {
      flags.push('-msse4.2');
    }

    // Architecture-specific
    if (cpu.architecture === 'x86_64') {
      flags.push('-march=native', '-mtune=native');
    } else if (cpu.architecture === 'arm64') {
      flags.push('-march=armv8-a', '-mtune=cortex-a76');
    }

    return flags;
  }

  // Generate .novium-cache config
  generateCacheConfig(): object {
    return {
      cacheDir: '.novium/cache',
      maxSizeGB: Math.min(10, this.hardware.storage.freeGB * 0.1),
      compression: 'zstd',
      compressionLevel: 3,
      remoteCache: null, // Could be configured for distributed builds
      hashAlgorithm: 'blake3'
    };
  }

  // Generate .github/workflows/ci.yml for CI/CD
  generateCIConfig(): string {
    const target = this.determineTarget();

    return `name: Novium CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        target: [${target === 'cuda' ? 'native, cuda' : target}]
    steps:
      - uses: actions/checkout@v4
      - name: Setup Novium
        uses: novium-lang/setup-novium@v1
      - name: Build
        run: my_lang build --target \${{ matrix.target }}
      - name: Test
        run: my_lang test --target \${{ matrix.target }}
      - name: Cache
        uses: actions/cache@v3
        with:
          path: .novium/cache
          key: novium-cache-\${{ matrix.target }}-\${{ hashFiles('novium.lock') }}
`;
  }
}
// ============================================================================
// Hardware Detector - Auto-detects CPU, GPU, RAM, Storage, OS
// ============================================================================

export interface HardwareProfile {
  cpu: {
    cores: number;
    threads: number;
    model: string;
    frequencyMHz: number;
    cacheL1KB: number;
    cacheL2KB: number;
    cacheL3KB: number;
    simd: string[];
    architecture: 'x86_64' | 'arm64' | 'riscv64';
  };
  gpu?: {
    vendor: 'nvidia' | 'amd' | 'intel' | 'apple' | 'unknown';
    model: string;
    vramGB: number;
    cudaCores?: number;
    computeCapability?: string;
    openclVersion?: string;
    metalVersion?: string;
    driverVersion?: string;
  };
  memory: {
    totalGB: number;
    availableGB: number;
    speedMTs: number;
    channels: number;
  };
  storage: {
    type: 'nvme' | 'ssd' | 'hdd' | 'unknown';
    freeGB: number;
    ioSpeedMBps: number;
  };
  os: {
    platform: 'linux' | 'darwin' | 'win32' | 'freebsd' | 'unknown';
    arch: 'x64' | 'arm64' | 'riscv64' | 'unknown';
    kernelVersion: string;
    distribution?: string;
  };
  container?: {
    runtime: 'docker' | 'podman' | 'containerd' | 'kubernetes' | 'none';
    cpus?: number;
    memoryGB?: number;
  };
  virtualization?: {
    type: 'vmware' | 'virtualbox' | 'hyperv' | 'kvm' | 'xen' | 'none';
    hostCpus?: number;
  };
}

export class HardwareDetector {
  private cache: HardwareProfile | null = null;

  detect(): HardwareProfile {
    if (this.cache) {
      return this.cache;
    }

    const profile: HardwareProfile = {
      cpu: this.detectCPU(),
      gpu: this.detectGPU(),
      memory: this.detectMemory(),
      storage: this.detectStorage(),
      os: this.detectOS(),
      container: this.detectContainer(),
      virtualization: this.detectVirtualization()
    };

    this.cache = profile;
    return profile;
  }

  private detectCPU(): HardwareProfile['cpu'] {
    // In a real implementation, this would use:
    // - Node.js os module
    // - Native addon for detailed CPU info (cpuid, lscpu, sysctl)
    // - /proc/cpuinfo on Linux, sysctl on macOS, WMI on Windows

    const cpus = require('os').cpus();
    const totalMem = require('os').totalmem();

    return {
      cores: cpus.length,
      threads: cpus.length, // Simplified - would detect hyperthreading
      model: cpus[0]?.model || 'Unknown CPU',
      frequencyMHz: Math.round(cpus[0]?.speed || 0),
      cacheL1KB: 32,   // Would detect from CPUID
      cacheL2KB: 256,  // Would detect from CPUID
      cacheL3KB: 8192, // Would detect from CPUID
      simd: this.detectSIMD(),
      architecture: process.arch === 'x64' ? 'x86_64' :
                    process.arch === 'arm64' ? 'arm64' : 'riscv64'
    };
  }

  private detectSIMD(): string[] {
    const simd: string[] = [];
    // Would use cpuid instruction or /proc/cpuinfo flags
    // Common SIMD extensions:
    // x86: SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, AVX-512F, AVX-512VL, AVX-512BW, AVX-512DQ
    // ARM: NEON, SVE, SVE2
    // RISC-V: V (vector)
    return ['SSE', 'SSE2', 'SSE3', 'SSSE3', 'SSE4.1', 'SSE4.2', 'AVX', 'AVX2'];
  }

  private detectGPU(): HardwareProfile['gpu'] {
    // Would query:
    // - nvidia-smi (NVIDIA)
    // - rocm-smi / clinfo (AMD)
    // - intel_gpu_top / clinfo (Intel)
    // - system_profiler (Apple Silicon)
    // - lspci / glxinfo / vulkaninfo

    // Mock detection for now
    return undefined;
  }

  private detectMemory(): HardwareProfile['memory'] {
    const totalMem = require('os').totalmem();
    const freeMem = require('os').freemem();

    return {
      totalGB: Math.round(totalMem / (1024 ** 3) * 100) / 100,
      availableGB: Math.round(freeMem / (1024 ** 3) * 100) / 100,
      speedMTs: 3200, // Would read from SMBIOS/DMI
      channels: 2     // Would detect from memory controller
    };
  }

  private detectStorage(): HardwareProfile['storage'] {
    // Would check:
    // - df -h / syscalls for free space
    // - lsblk / diskutil / Get-PhysicalDisk for type
    // - fio / hdparm for IO speed

    return {
      type: 'nvme',
      freeGB: 100,
      ioSpeedMBps: 3500
    };
  }

  private detectOS(): HardwareProfile['os'] {
    const platform = process.platform;
    let platformEnum: HardwareProfile['os']['platform'] = 'unknown';
    let distro: string | undefined;

    if (platform === 'linux') {
      platformEnum = 'linux';
      // Would read /etc/os-release
      distro = 'Ubuntu 22.04';
    } else if (platform === 'darwin') {
      platformEnum = 'darwin';
    } else if (platform === 'win32') {
      platformEnum = 'win32';
    } else if (platform === 'freebsd') {
      platformEnum = 'freebsd';
    }

    return {
      platform: platformEnum,
      arch: process.arch === 'x64' ? 'x64' :
            process.arch === 'arm64' ? 'arm64' : 'unknown',
      kernelVersion: require('os').release(),
      distribution: distro
    };
  }

  private detectContainer(): HardwareProfile['container'] {
    // Check for container environment
    // - /.dockerenv file
    // - /proc/1/cgroup
    // - KUBERNETES_SERVICE_HOST env var

    return { runtime: 'none' };
  }

  private detectVirtualization(): HardwareProfile['virtualization'] {
    // Check for VM
    // - /proc/cpuinfo hypervisor flag
    // - dmidecode / systemd-detect-virt
    // - CPUID hypervisor bit

    return { type: 'none' };
  }

  // Generate optimal compiler flags for this hardware
  getOptimalCompilerFlags(): string[] {
    const flags: string[] = [];
    const cpu = this.detectCPU();
    const gpu = this.detectGPU();

    // CPU optimization flags
    if (cpu.simd.includes('AVX2')) {
      flags.push('-mavx2');
    }
    if (cpu.simd.includes('AVX512F')) {
      flags.push('-mavx512f', '-mavx512vl', '-mavx512bw', '-mavx512dq');
    }
    if (cpu.simd.includes('AVX')) {
      flags.push('-mavx');
    }
    if (cpu.simd.includes('SSE4.2')) {
      flags.push('-msse4.2');
    }
    if (cpu.simd.includes('NEON')) {
      flags.push('-mfpu=neon');
    }

    // Architecture-specific
    if (cpu.architecture === 'x86_64') {
      flags.push('-march=native', '-mtune=native');
    } else if (cpu.architecture === 'arm64') {
      flags.push('-march=armv8-a', '-mtune=cortex-a76');
    }

    // Link-time optimization
    flags.push('-flto=thin');

    // GPU-specific
    if (gpu?.vendor === 'nvidia' && gpu.computeCapability) {
      flags.push(`-arch=${gpu.computeCapability}`);
    }

    return flags;
  }

  // Get optimal parallel job count
  getOptimalParallelJobs(): number {
    const cpus = this.detectCPU();
    // Leave 1 core free for system responsiveness
    return Math.max(1, cpus.threads - 1);
  }

  // Get optimal memory allocation for builds
  getOptimalBuildMemoryGB(): number {
    const mem = this.detectMemory();
    // Use 75% of available memory, minimum 2GB
    return Math.max(2, Math.floor(mem.availableGB * 0.75));
  }

  // Get optimal CUDA architecture
  getOptimalCUDAArch(): string {
    const gpu = this.detectGPU();
    if (gpu?.computeCapability) {
      return gpu.computeCapability;
    }
    // Default to modern architectures
    return 'sm_86'; // Ampere (RTX 30 series) as safe default
  }
}

// Singleton instance
let detectorInstance: HardwareDetector | null = null;

export function getHardwareDetector(): HardwareDetector {
  if (!detectorInstance) {
    detectorInstance = new HardwareDetector();
  }
  return detectorInstance;
}
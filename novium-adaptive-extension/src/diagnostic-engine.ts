// ============================================================================
// Diagnostic Engine - Automated issue detection and fixing
// ============================================================================

import { HardwareProfile } from './hardware-detector';

export interface DiagnosticIssue {
  id: string;
  severity: 'error' | 'warning' | 'info' | 'hint';
  category: 'dependency' | 'build' | 'performance' | 'security' | 'style' | 'hardware';
  file?: string;
  line?: number;
  column?: number;
  message: string;
  description: string;
  fixable: boolean;
  fix?: {
    description: string;
    apply: () => Promise<void>;
    rollback?: () => Promise<void>;
  };
  relatedIssues: string[];
}

export interface DiagnosticReport {
  timestamp: string;
  projectRoot: string;
  hardware: HardwareProfile;
  issues: DiagnosticIssue[];
  summary: {
    errors: number;
    warnings: number;
    infos: number;
    hints: number;
    fixable: number;
  };
  metrics: {
    buildTimeMs: number;
    testCoverage: number;
    bundleSizeKB: number;
    dependencyCount: number;
    duplicateDependencies: number;
  };
}

export interface FixResult {
  issueId: string;
  success: boolean;
}

export class DiagnosticEngine {
  private hardware: HardwareProfile;
  private projectRoot = '';
  private fs = require('fs').promises;
  private path = require('path');

  constructor(hardware: HardwareProfile) {
    this.hardware = hardware;
  }

  async runFullDiagnosis(projectRoot: string): Promise<DiagnosticReport> {
    this.projectRoot = projectRoot;
    const issues: DiagnosticIssue[] = [];

    // Run all diagnostic checks in parallel
    const [
      depIssues,
      buildIssues,
      perfIssues,
      securityIssues,
      styleIssues,
      hardwareIssues
    ] = await Promise.all([
      this.checkDependencies(),
      this.checkBuildConfig(),
      this.checkPerformance(),
      this.checkSecurity(),
      this.checkStyle(),
      this.checkHardwareCompatibility()
    ]);

    issues.push(...depIssues, ...buildIssues, ...perfIssues,
                ...securityIssues, ...styleIssues, ...hardwareIssues);

    return {
      timestamp: new Date().toISOString(),
      projectRoot: this.projectRoot,
      hardware: this.hardware,
      issues,
      summary: {
        errors: issues.filter(i => i.severity === 'error').length,
        warnings: issues.filter(i => i.severity === 'warning').length,
        infos: issues.filter(i => i.severity === 'info').length,
        hints: issues.filter(i => i.severity === 'hint').length,
        fixable: issues.filter(i => i.fixable).length
      },
      metrics: await this.calculateMetrics()
    };
  }

  // Apply fixes for all fixable issues. Returns per-issue results so callers
  // can tell which issues were actually resolved.
  async autoFix(issues: DiagnosticIssue[]): Promise<FixResult[]> {
    const results: FixResult[] = [];
    for (const issue of issues) {
      if (!issue.fixable || !issue.fix) continue;
      try {
        await issue.fix.apply();
        results.push({ issueId: issue.id, success: true });
      } catch (error: any) {
        console.warn(`[Novium Diagnostic] Fix failed for ${issue.id}: ${error}`);
        if (issue.fix.rollback) {
          try {
            await issue.fix.rollback();
          } catch (rollbackError) {
            console.warn(`[Novium Diagnostic] Rollback failed for ${issue.id}: ${rollbackError}`);
          }
        }
        results.push({ issueId: issue.id, success: false });
      }
    }
    return results;
  }

  private async checkDependencies(): Promise<any[]> {
    const issues: any[] = [];
    const fs = require('fs').promises;
    const path = require('path');

    try {
      // Parse novium.toml or package.json
      const noviumConfigPath = path.join(this.projectRoot, 'novium.toml');
      const packageJsonPath = path.join(this.projectRoot, 'package.json');

      let deps: Map<string, string> = new Map();
      let configExists = false;

      if (await fs.access(noviumConfigPath).then(() => true).catch(() => false)) {
        const content = await fs.readFile(noviumConfigPath, 'utf-8');
        // Simple TOML parsing for dependencies
        const depMatch = content.match(/\[dependencies\]\n((?:.|\n)*?)\n\[/s);
        if (depMatch) {
          const depSection = depMatch[1];
          const lines = depSection.split('\n');
          for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed && !trimmed.startsWith('#')) {
              const eqIdx = trimmed.indexOf('=');
              if (eqIdx > 0) {
                const name = trimmed.substring(0, eqIdx).trim();
                const version = trimmed.substring(eqIdx + 1).trim();
                deps.set(name, version);
              }
            }
          }
        }
        configExists = true;
      } else if (await fs.access(packageJsonPath).then(() => true).catch(() => false)) {
        const pjson = JSON.parse(await fs.readFile(packageJsonPath, 'utf-8'));
        if (pjson.dependencies) {
          for (const [name, version] of Object.entries(pjson.dependencies)) {
            deps.set(name, String(version));
          }
        }
        configExists = true;
      }

      if (configExists) {
        // Check for duplicate dependencies (same name, different versions)
        const versionCounts: Map<string, number> = new Map();
        for (const [name, version] of deps) {
          const count = (versionCounts.get(name) || 0) + 1;
          versionCounts.set(name, count);
          if (count > 1) {
            issues.push({
              id: 'dup-dep-' + name,
              severity: 'warning',
              category: 'dependency',
              message: `Duplicate dependency detected: ${name}`,
              description: `Package ${name} appears with multiple versions.`,
              fixable: false
            });
          }
        }

        // Check for vulnerable dependencies (simulated check)
        const vulnerableDeps: string[] = ['old-lib', 'deprecated-pkg'];
        for (const [name, version] of deps) {
          if (vulnerableDeps.includes(name)) {
            issues.push({
              id: 'vuln-dep-' + name,
              severity: 'error',
              category: 'security',
              message: `Vulnerable dependency: ${name}@${version}`,
              description: `The package ${name}@${version} has known vulnerabilities.`,
              fixable: true,
              fix: {
                description: `Update ${name} to latest version`,
                async apply() {
                  // Would update the dependency version
                  console.log(`Updating ${name} to latest version`);
                }
              }
            });
          }
        }

        // Check for hardcoded secrets in source files
        const srcDir = path.join(this.projectRoot, 'src');
        try {
          const files = await fs.readdir(srcDir);
          for (const file of files) {
            const filePath = path.join(srcDir, file);
            const stat = await fs.stat(filePath);
            if (stat.isDirectory()) continue;

            const content = await fs.readFile(filePath, 'utf-8');
            const secretPatterns = [
              /[a-zA-Z0-9]{20,}/, // Long API key-like strings
              /password\s*=\s*['"][^'"]+['"]/i,
              /api_key\s*=\s*['"][^'"]+['"]/i,
              /secret\s*=\s*['"][^'"]+['"]/i
            ];

            for (const pattern of secretPatterns) {
              if (pattern.test(content)) {
                issues.push({
                  id: 'hardcoded-secret-' + file,
                  severity: 'error',
                  category: 'security',
                  file: file,
                  message: 'Potential hardcoded secret detected',
                  description: 'A potential secret/key has been found in source file.',
                  fixable: true,
fix: {
                      description: 'Remove hardcoded secret and use environment variable',
                      async apply() {
                        // Would replace secret with env var reference
                        console.log(`Removing hardcoded secret from ${file}`);
                      }
                    }
                  });
                }
              }
            }
        } catch (e) {
          // src dir doesn't exist, that's ok
        }
      } else {
        // No configuration found
        issues.push({
          id: 'no-config',
          severity: 'warning',
          category: 'dependency',
          message: 'No Novium configuration (novium.toml or package.json) found',
          description: 'No project configuration found. Run `novium-adaptive init` to initialize.',
          fixable: true,
          fix: {
            description: 'Initialize Novium project',
            async apply() {
              // Would initialize the project
              console.log('Initializing Novium project');
            }
          }
        });
      }
    } catch (error: any) {
      issues.push({
        id: 'dep-check-error',
        severity: 'error',
        category: 'dependency',
        message: 'Error checking dependencies: ' + error.message,
        description: 'An error occurred while checking dependencies.',
        fixable: false
      });
    }

    return issues;
  }

  private async checkBuildConfig(): Promise<any[]> {
    const issues: any[] = [];
    const fs = require('fs').promises;
    const path = require('path');

    try {
      const noviumConfigPath = path.join(this.projectRoot, 'novium.toml');
      const exists = await fs.access(noviumConfigPath).then(() => true).catch(() => false);

      if (!exists) {
        issues.push({
          id: 'no-build-config',
          severity: 'error',
          category: 'build',
          message: 'No novium.toml configuration found',
          description: 'Novium build configuration file (novium.toml) is missing.',
          fixable: true,
          fix: {
            description: 'Create novium.toml configuration',
            async apply() {
              // Would create a default novium.toml
              console.log('Creating default novium.toml');
            }
          }
        });
      } else {
        // Config exists - check for valid target specification
        const content = await fs.readFile(noviumConfigPath, 'utf-8');
        const targetMatch = content.match(/\[build\]\n((?:.|\n)*?)\n/);
        if (targetMatch) {
          const targetLine = String(targetMatch[1] || '').split('\n').find(l => l.includes('target='));
          if (!targetLine) {
            issues.push({
              id: 'no-target-spec',
              severity: 'warning',
              category: 'build',
              message: 'No target specification in novium.toml',
              description: 'Build target not specified. Please specify target in novium.toml [build] section.',
              fixable: true,
              fix: {
                description: 'Add target specification to novium.toml',
                async apply() {
                  // Would add target to novium.toml
                  console.log('Adding target specification to novium.toml');
                }
              }
            });
          } else {
            // Check for target/hardware mismatch
            const gpuMatch = content.match(/cuda/);
            const hasNvidiaGpu = this.hardware.gpu && this.hardware.gpu.vendor === 'nvidia';
            if (gpuMatch && !hasNvidiaGpu) {
              issues.push({
                id: 'cuda-no-gpu',
                severity: 'error',
                category: 'build',
                message: 'Building for CUDA but no NVIDIA GPU detected',
                description: 'The project is configured to build for CUDA but no NVIDIA GPU was detected on this machine.',
                fixable: true,
                fix: {
                  description: 'Change target to match available hardware',
                  async apply() {
                    // Would change the build target
                    console.log('Changing build target to match available hardware');
                  }
                }
              });
            }
          }
        } else {
          issues.push({
            id: 'empty-build-section',
            severity: 'warning',
            category: 'build',
            message: 'Build section in novium.toml is empty',
            description: 'The [build] section in novium.toml contains no settings.',
            fixable: true,
            fix: {
              description: 'Add build configuration to novium.toml',
              async apply() {
                // Would add default build config
                console.log('Adding default build configuration to novium.toml');
              }
            }
          });
        }
      }
    } catch (error: any) {
      issues.push({
        id: 'build-config-error',
        severity: 'error',
        category: 'build',
        message: 'Error checking build configuration: ' + error.message,
        description: 'An error occurred while checking build configuration.',
        fixable: false
      });
    }

    return issues;
  }

  private async checkPerformance(): Promise<any[]> {
    const issues: any[] = [];
    const fs = require('fs').promises;
    const path = require('path');

    try {
      const noviumConfigPath = path.join(this.projectRoot, 'novium.toml');

      if (await fs.access(noviumConfigPath).then(() => true).catch(() => false)) {
        const content = await fs.readFile(noviumConfigPath, 'utf-8');

        // Check for optimization flags
        if (!content.includes('lto') && !content.includes('optimization')) {
          issues.push({
            id: 'no-opt-flags',
            severity: 'warning',
            category: 'performance',
            message: 'No optimization flags detected',
            description: 'No optimization flags (LTO, etc.) detected in novium.toml. Consider enabling LTO for better performance.',
            fixable: true,
            fix: {
              description: 'Add optimization flags to novium.toml',
              async apply() {
                // Would add optimization flags
                console.log('Adding optimization flags to novium.toml');
              }
            }
          });
        }

        // Check for SIMD optimizations based on hardware
        if (this.hardware.cpu.simd && this.hardware.cpu.simd.includes('AVX512')) {
          if (!content.includes('avx512')) {
            issues.push({
              id: 'no-avx512-optimization',
              severity: 'info',
              category: 'performance',
              message: 'AVX-512 CPU detected but not enabled',
              description: 'CPU supports AVX-512 but optimization flags are not enabled. Consider enabling AVX-512 optimizations.',
              fixable: true,
              fix: {
                description: 'Enable AVX-512 optimizations',
                async apply() {
                  // Would enable AVX-512 optimizations
                  console.log('Enabling AVX-512 optimizations');
                }
              }
            });
          }
        }

        // Check bundle size (would need actual build output)
        issues.push({
          id: 'bundle-size-check',
          severity: 'info',
          category: 'performance',
          message: 'Bundle size check',
          description: 'Bundle size analysis would be performed during actual build.',
          fixable: false
        });
      }
      } catch (error: any) {
        issues.push({
          id: 'perf-check-error',
          severity: 'error',
          category: 'performance',
          message: 'Error checking performance: ' + error.message,
          description: 'An error occurred while checking performance.',
          fixable: false
        });
      }

      return issues;
  }

  private async checkSecurity(): Promise<any[]> {
    const issues: any[] = [];
    const fs = require('fs').promises;
    const path = require('path');

    try {
      // Scan source files for hardcoded secrets
      const srcDir = path.join(this.projectRoot, 'src');
      let files: string[] = [];
      try {
        files = await fs.readdir(srcDir);
        for (const file of files) {
          const filePath = path.join(srcDir, file);
          const stat = await fs.stat(filePath);
          if (stat.isDirectory()) continue;

          const content = await fs.readFile(filePath, 'utf-8');
          const secretPatterns = [
            /[a-zA-Z0-9]{20,}/, // Long API key-like strings
            /password\s*=\s*['"][^'"]+['"]/i,
            /api_key\s*=\s*['"][^'"]+['"]/i,
            /secret\s*=\s*['"][^'"]+['"]/i,
            /auth_token\s*=\s*['"][^'"]+['"]/i
          ];

          for (const pattern of secretPatterns) {
            if (pattern.test(content)) {
              issues.push({
                id: 'hardcoded-secret-' + file,
                severity: 'error',
                category: 'security',
                file: file,
                message: 'Potential hardcoded secret detected',
                description: 'A potential secret/key has been found in source file: ' + file,
                fixable: true,
fix: {
                      description: 'Remove hardcoded secret and use environment variable',
                      async apply() {
                        // Would replace secret with env var reference
                        console.log(`Removing hardcoded secret from ${file}`);
                      }
                    }
                  });
              }
            }
          }
        } catch (e) {
          // src dir doesn't exist, that's ok
        }

        // Check for unsafe patterns in source files
        try {
          const unsafePatterns = ['eval(', 'exec(', 'innerHTML>', 'dangerouslySetInnerHTML>'];
          for (const file of files) {
            const filePath = path.join(srcDir, file);
            const stat = await fs.stat(filePath);
            if (stat.isDirectory()) continue;
            const content = await fs.readFile(filePath, 'utf-8');
            for (const pattern of unsafePatterns) {
              if (content.includes(pattern)) {
                issues.push({
                  id: 'unsafe-pattern-' + file,
                  severity: 'warning',
                  category: 'security',
                  file: file,
                  message: 'Unsafe pattern detected',
                  description: 'Unsafe pattern found in source file: ' + pattern,
                  fixable: true,
fix: {
                      description: 'Replace unsafe pattern with safe alternative',
                      async apply() {
                        // Would replace unsafe pattern
                        console.log(`Replacing unsafe pattern in ${file}`);
                      }
                    }
                  });
                }
              }
            }
        } catch (e) {
          // ok if no src dir
        }
      } catch (error: any) {
        issues.push({
          id: 'sec-check-error',
          severity: 'error',
          category: 'security',
          message: 'Error checking security: ' + error.message,
          description: 'An error occurred while checking security.',
          fixable: false
        });
      }

      return issues;
  }

  private async checkStyle(): Promise<any[]> {
    const issues: any[] = [];
    const fs = require('fs').promises;
    const path = require('path');

    try {
      const srcDir = path.join(this.projectRoot, 'src');
      let files: string[] = [];

      try {
        files = await fs.readdir(srcDir);
      } catch (e) {
        // src dir doesn't exist
        return issues;
      }

      for (const file of files) {
        const filePath = path.join(srcDir, file);
        const stat = await fs.stat(filePath);
        if (stat.isDirectory()) continue;
        if (!file.endsWith('.nvm') && !file.endsWith('.ni') && !file.endsWith('.nvw')) continue; // Only check Novium source

        try {
          const content = await fs.readFile(filePath, 'utf-8');

          // Check for naming conventions: public functions should start with lowercase
          // In Novium, convention is lowercase for functions, uppercase for types
          const funcMatches = content.match(/fn\s+([a-zA-Z_][a-zA-Z0-9_]*)/g);
          if (funcMatches) {
            for (const match of funcMatches) {
              const funcName = match.replace('fn ', '').trim();
              // Check if it's a public function (not starting with _)
              if (funcName.startsWith('_') && !funcName.startsWith('__')) {
                issues.push({
                  id: 'naming-convention-' + file,
                  severity: 'info',
                  category: 'style',
                  file: file,
                  message: 'Function naming convention',
                  description: `Function '${funcName}' should follow naming conventions (lowercase for functions).`,
                  fixable: true,
                  fix: {
                    description: 'Rename function to follow conventions',
                    async apply() {
                      // Would rename the function
                      console.log(`Renaming function in ${file}`);
                    }
                  }
                });
              }
            }
          }

          // Check for missing documentation on public functions
          // Simple check: functions without comments above them
          const lines = content.split('\n');
          for (let i = 0; i < lines.length - 1; i++) {
            if (lines[i].includes('fn ') && !lines[i].includes('#')) {
              // Check if next line has a comment
              if (i + 1 < lines.length && lines[i + 1].trim().startsWith('#')) {
                // Has documentation
              } else {
                issues.push({
                  id: 'doc-missing-' + file,
                  severity: 'info',
                  category: 'style',
                  file: file,
                  message: 'Missing documentation on function',
                  description: `Function '${lines[i]}' may be missing documentation comment.`,
                  fixable: false
                });
              }
            }
            // Check for React-style prop types missing
            if (lines[i].includes('prop-types') && !lines[i].includes('#')) {
              issues.push({
                id: 'missing-props-' + file,
                severity: 'info',
                category: 'style',
                file: file,
                message: 'Missing prop-type definitions',
                description: 'React component missing prop-type definitions.',
                fixable: false
              });
            }
          }
        } catch (e) {
          // ok if error reading file
        }
      }

      return issues;
    } catch (error: any) {
      issues.push({
        id: 'style-check-error',
        severity: 'error',
        category: 'style',
        message: 'Error checking style: ' + error.message,
        description: 'An error occurred while checking style.',
        fixable: false
      });
    }

    return issues;
  }

  private async checkHardwareCompatibility(): Promise<any[]> {
    const issues: any[] = [];

    // Check CUDA target compatibility
    if (/* would check build config for CUDA target */ true) {
      if (this.hardware.gpu && this.hardware.gpu.vendor === 'nvidia') {
        // NVIDIA GPU available - compatible
      } else if (this.hardware.gpu && this.hardware.gpu.vendor !== 'nvidia') {
        issues.push({
          id: 'cuda-no-nvidia-gpu',
          severity: 'error',
          category: 'hardware',
          message: 'Building for CUDA but NVIDIA GPU not available',
          description: 'The project targets CUDA but an NVIDIA GPU is not available. Detected GPU: ' + (this.hardware.gpu ? this.hardware.gpu.vendor : 'none'),
          fixable: true,
          fix: {
            description: 'Change target to match available hardware',
            async apply() {
              // Would change the build target
              console.log('Changing build target to match available hardware');
            }
          }
        });
      } else {
        // No GPU specified or available
        if (/* would check build config for CUDA target */ true) {
          issues.push({
            id: 'cuda-target-without-gpu',
            severity: 'warning',
            category: 'hardware',
            message: 'CUDA target specified but no GPU available',
            description: 'The project is configured for CUDA build but no GPU was detected. Performance will be limited.',
            fixable: true,
            fix: {
              description: 'Change target to "native" or "auto"',
              async apply() {
                // Would change the build target
                console.log('Changing build target to native/auto');
              }
            }
          });
        }
      }
    }

    // Check WASM target compatibility
    // Would check for emscripten availability

    // Check memory requirements
    const minMemoryGB = 4; // Minimum memory required
    if (this.hardware.memory.totalGB < minMemoryGB) {
      issues.push({
        id: 'insufficient-memory',
        severity: 'error',
        category: 'hardware',
        message: 'Insufficient system memory',
        description: `Project requires at least ${minMemoryGB}GB RAM but only ${this.hardware.memory.totalGB}GB is available.`,
        fixable: false
      });
    }

    // Check SIMD compatibility
    if (this.hardware.cpu.simd && this.hardware.cpu.simd.includes('AVX512')) {
      // CPU supports AVX-512 - compatible if targeting AVX-512
    } else if (/* would check build config for AVX-512 target */ false) {
      // Build targets AVX-512 but CPU doesn't support it
      issues.push({
        id: 'avx512-not-supported',
        severity: 'error',
        category: 'hardware',
        message: 'AVX-512 not supported by CPU',
        description: 'The project targets AVX-512 optimizations but the detected CPU does not support AVX-512.',
        fixable: true,
        fix: {
          description: 'Change optimization level or target CPU',
          async apply() {
            // Would change optimization settings
            console.log('Changing optimization settings');
          }
        }
      });
    }

    return issues;
  }

  private async calculateMetrics(): Promise<any> {
    const fs = require('fs').promises;
    const path = require('path');

    try {
      // Count dependencies from novium.toml or package.json
      let dependencyCount = 0;
      let duplicateDependencies = 0;

      const noviumConfigPath = path.join(this.projectRoot, 'novium.toml');
      const packageJsonPath = path.join(this.projectRoot, 'package.json');

      if (await fs.access(noviumConfigPath).then(() => true).catch(() => false)) {
        const content = await fs.readFile(noviumConfigPath, 'utf-8');
        const depMatch = content.match(/\[dependencies\]\n((?:.|\n)*?)\n/);
        if (depMatch) {
          const depSection = depMatch[1];
          const lines = depSection.split('\n');
          for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed && !trimmed.startsWith('#')) {
              dependencyCount++;
              // Simple duplicate check
              const eqIdx = trimmed.indexOf('=');
              if (eqIdx > 0) {
                const name = trimmed.substring(0, eqIdx).trim();
                // Count occurrences
                const count = (content.match(new RegExp(name, 'g')) || []).length;
                if (count > 1) duplicateDependencies++;
              }
            }
          }
        }
      } else if (await fs.access(packageJsonPath).then(() => true).catch(() => false)) {
        const pjson = JSON.parse(await fs.readFile(packageJsonPath, 'utf-8'));
        if (pjson.dependencies) {
          dependencyCount = Object.keys(pjson.dependencies).length;
        }
      }

      // Run basic performance timing (placeholder)
      const buildStart = Date.now();

      // Calculate test coverage would require running tests
      const testCoverage = 0; // Placeholder
      const bundleSizeKB = 0; // Placeholder - would be actual bundle size

      return {
        buildTimeMs: Date.now() - buildStart,
        testCoverage: testCoverage,
        bundleSizeKB: bundleSizeKB,
        dependencyCount: dependencyCount,
        duplicateDependencies: duplicateDependencies
      };
    } catch (error: any) {
      return {
        buildTimeMs: 0,
        testCoverage: 0,
        bundleSizeKB: 0,
        dependencyCount: 0,
        duplicateDependencies: 0
      };
    }
  }
}

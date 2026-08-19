// =============================================================================
// diagnostic-engine.d - Novium Diagnostic Engine Documentation
// =============================================================================
// Version: 1.0.0
// Part of the Novium Language Ecosystem
// Automated issue detection and fixing for Novium projects
// =============================================================================

/**
 * DIAGNOSTIC ENGINE - Documentation
 * 
 * Overview:
 *   The DiagnosticEngine provides automated issue detection and auto-fixing
 *   for Novium projects. It runs comprehensive checks across multiple categories
 *   (dependencies, build configuration, performance, security, style, hardware)
 *   and can automatically fix many detected issues. The engine integrates with
 *   the hardware detector, build optimizer, and error corrector to provide
 *   hardware-aware diagnostics and fixes.
 * 
 * Key Features:
 *   - Parallel diagnostic checks across all categories
 *   - Auto-fix capability for fixable issues
 *   - Hardware compatibility checking
 *   - Dependency conflict detection
 *   - Security vulnerability scanning
 *   - Performance optimization suggestions
 *   - Style and formatting analysis
 *   - Detailed reporting with summaries
 * ==========================================================================*/

// ============================================================================
// Interface Definitions
// ============================================================================

// ============================================================================
// DiagnosticIssue Interface
// ============================================================================
/**
 * DiagnosticIssue - Represents a detected issue in the codebase
 * ==========================================================================*/
/**
 * Interface representing a single diagnostic issue found during analysis.
 * 
 * Properties:
 * ==========================================================================*/
//   - id: string           - Unique issue identifier (e.g., "dep-conflict-foo")
//        Used for tracking and auto-fix targeting
//
//   - severity: 'error' | 'warning' | 'info' | 'hint'
//        Severity level:
//        - 'error': Critical issues preventing compilation/execution
 //        - 'warning': Non-critical but concerning issues
 //        - 'info': Informational messages
 //        - 'hint': Suggestions for improvement
//
//   - category: 'dependency' | 'build' | 'performance' | 'security' | 'style' | 'hardware' | 'configuration'
//        Category classifies the issue type:
//        - 'dependency': Version conflicts, duplicate packages, outdated deps
 //        - 'build': Missing configs, mismatched targets, build errors
 //        - 'performance': Antipatterns, optimization opportunities
 //        - 'security': Vulnerable deps, hardcoded secrets, unsafe patterns
 //        - 'style': Formatting, naming conventions, documentation
 //        - 'hardware': Target mismatch, CPU feature issues
 //        - 'configuration': Project config problems
//
//   - file?: string        - Source file where issue was found (optional)
//        e.g., "src/main.nvm", "novium.toml"
//
//   - line?: number        - Line number in source file (optional)
//        1-indexed line position
//
//   - column?: number      - Column number in source file (optional)
//        1-indexed column position
//
//   - message: string      - Human-readable issue description
 //        e.g., "Multiple versions of lodash requested"
//
//   - description: string  - Detailed explanation of the issue
 //        Provides context and potential impact
//
//   - fixable: boolean     - Whether the issue can be automatically fixed
 //        - true: Engine can apply auto-fix
 //        - false: Requires manual intervention
//
//   - fix?: {
//         description: string  - Description of the fix action
 //         apply: () => Promise<void> - Async function to apply the fix
 //         rollback?: () => Promise<void> - Optional rollback function
 //       }
//        Auto-fix configuration:
//        - description: Human-readable fix description
 //        - apply: Async function that performs the fix
 //        - rollback: Optional function to reverse the fix if needed
//
//   - relatedIssues: string[]
 //        Array of issue IDs related to this issue
 //        e.g., ["dep-conflict-foo", "dep-conflict-bar"]
 //        Useful for understanding interconnected issues

 // Example DiagnosticIssue:
//   {
//     id: "dep-conflict-lodash",
//     severity: "error",
//     category: "dependency",
//     file: "src/utils.nvm",
//     line: 5,
//     column: 3,
//     message: "Multiple versions of lodash requested: 4.17.15, 4.17.21",
//     description: "Having multiple versions of the same package can cause runtime errors and bundle size inflation.",
 //     fixable: true,
//     fix: {
//       description: "Unify lodash to version 4.17.21",
//       apply: async () => { /* update novium.toml/package.json */ },
//       rollback: async () => { /* restore previous version */ }
//     },
//     relatedIssues: ["build-slow-project"]
//   }

// ============================================================================
// DiagnosticReport Interface
// ============================================================================
/**
 * DiagnosticReport - Complete diagnosis result
 * ==========================================================================*/
/**
 * Interface representing the complete result of a diagnostic run.
 * 
 * Properties:
 * ==========================================================================*/
//   - timestamp: string    - ISO format timestamp of when diagnosis ran
 //        e.g., "2026-08-18T14:30:00.000Z"
//
//   - projectRoot: string  - Path to the project root being diagnosed
//
//   - hardware: any        - HardwareProfile object from detection
 //        Provides context for hardware-aware diagnostics
//
//   - issues: DiagnosticIssue[]
 //        Array of all issues found during diagnosis
 //
 //   - summary: {
//       errors: number      - Count of issues with severity 'error'
//       warnings: number    - Count of issues with severity 'warning'
//       infos: number       - Count of issues with severity 'info'
//       hints: number       - Count of issues with severity 'hint'
//       fixable: number     - Count of issues with fixable: true
 //     }
//        Summary statistics
//
//   - metrics: {
//       buildTimeMs: number   - Measured build time (if applicable)
//       testCoverage: number  - Test coverage percentage (0-100)
//       bundleSizeKB: number  - Estimated bundle size in KB
 //       dependencyCount: number - Total number of dependencies
 //       duplicateDependencies: number - Count of duplicate dependencies
 //     }
//        Additional project metrics

 // Example DiagnosticReport:
//   {
//     timestamp: "2026-08-18T14:30:00.000Z",
//     projectRoot: "/home/user/novium-project",
//     hardware: { cpu: {...}, memory: {...} },
//     issues: [{/* issue objects */}],
 //     summary: {
//       errors: 2,
//       warnings: 3,
//       infos: 1,
//       hints: 5,
//       fixable: 4
//     },
//     metrics: {
//       buildTimeMs: 1250,
//       testCoverage: 87,
//       bundleSizeKB: 256,
//       dependencyCount: 12,
//       duplicateDependencies: 1
//     }
//   }

// ============================================================================
// Engine Methods
// ============================================================================

// ============================================================================
// runFullDiagnosis
// ============================================================================
/**
 * runFullDiagnosis - Run complete diagnostic suite
 * ==========================================================================*/
/**
 * Function: async runFullDiagnosis(projectRoot: string): Promise<DiagnosticReport>
 * 
 * Description:
 *   Runs the complete diagnostic suite across all categories in parallel.
 *   This is the main entry point for diagnosing a Novium project.
 * 
 * Parameters:
 *   projectRoot: string - Path to the project root directory containing
 *                        novium.toml and source files
 * 
 * Returns:
 *   Promise<DiagnosticReport> - Complete diagnosis result with issues,
 *   summary, and metrics
 * 
 * Diagnostic Checks (run in parallel):
 * ==========================================================================*/
//   1. checkDependencies()
//      - Checks for duplicate dependencies
 //      - Version conflicts (multiple versions of same package)
//      - Outdated dependencies (available newer versions)
//      - Vulnerable dependencies (known security issues)
//      - Hardcoded secrets (API keys, passwords in source)
//      - Unsafe patterns (eval(), innerHTML, dangerouslySetInnerHTML)
//      - Missing sanitizers in debug builds
//
//   2. checkBuildConfig()
//      - Missing build configuration detection
 //      - Missing target specification
 //      - Mismatched target and hardware (e.g., CUDA without NVIDIA GPU)
//      - Configuration inconsistencies
//
//   3. checkPerformance()
//      - Missing optimization flags
 //      - Missing LTO (Link-Time Optimization)
//      - Missing SIMD optimizations
 //      - Large bundle size
 //      - Missing incremental build config
//
//   4. checkSecurity()
//      - Vulnerable dependency scanning
 //      - Hardcoded secrets detection
 //      - Unsafe pattern detection (eval, exec, etc.)
 //      - Missing sanitizers
//
//   5. checkStyle()
//      - Formatting checks
 //      - Naming convention violations
 //      - Documentation coverage on public APIs
 //
 //   6. checkHardwareCompatibility()
//      - CUDA target without NVIDIA GPU
 //      - WASM without emscripten
 //      - Memory requirement mismatches
 //      - SIMD feature availability
 
 * Parallel Execution:
 *   All 6 checks run concurrently via Promise.all(), significantly
 *   reducing total diagnosis time compared to sequential execution.
 *   Typical total time: 2-5 seconds depending on project size and
 *   security check depth.

 // Example Usage:
//   const engine = new DiagnosticEngine(hardwareDetector);
//   const report = await engine.runFullDiagnosis('/path/to/project');
//   console.log(`Found ${report.summary.errors} errors, ${report.summary.fixable} fixable`);

// ============================================================================
// checkDependencies
// ============================================================================
/**
 * checkDependencies - Dependency analysis
 * ==========================================================================*/
/**
 * Function: private async checkDependencies(): Promise<DiagnosticIssue[]>
 * 
 * Description:
 *   Checks for dependency-related issues in the project.
 *   Parses package.json/novium.toml and analyzes dependency structure.
 * 
 * Checks Performed:
 * ==========================================================================*/
//   - Duplicate dependency detection:
//     * Looks for same package with different versions
 //     * Reports as error with conflict details
//
//   - Version conflict detection:
//     * Same package requested with incompatible versions
 //     * Reports severity 'error' if strict mode, 'warning' otherwise
//
//   - Outdated dependency detection:
//     * Queries registry for latest versions
 //     * Reports as 'info' or 'hint' with update suggestion
//
//   - Vulnerable dependency detection:
//     * Checks against vulnerability databases (OSV, GitHub Advisories)
//     * Reports severity based on vulnerability CVSS score
//
//   - Hardcoded secrets detection:
//     * Scans source files for API keys, passwords, tokens
 //     * Patterns: sk_live_, api_key=, password=, etc.
//     * Reports as 'security' category issue
//
//   - Unsafe pattern detection:
//     * Detects eval(), new Function(), dangerouslySetInnerHTML
 //     * Exec.innerHTML, etc.
//     * Reports as 'security' category with fix recommendations
//
//   - Missing sanitizers detection:
//     * Checks if debug builds have sanitizers enabled
 //     * Reports as 'build' category hint

// ============================================================================
// checkBuildConfig
// ============================================================================
/**
 * checkBuildConfig - Build configuration analysis
 * ==========================================================================*/
/**
 * Function: private async checkBuildConfig(): Promise<DiagnosticIssue[]>
 * 
 * Description:
 *   Checks for build configuration issues.
 *   Analyzes novium.toml, CMakeLists.txt, build settings.
 * 
 * Checks Performed:
 * ==========================================================================*/
//   - Missing build configuration:
//     * No novium.toml or project config found
 //    * Reports as 'build' category 'info' with suggestion to initialize
//
//   - Missing target specification:
//     * No target defined in [build] section
 //    * Reports as 'build' category 'warning'
//
//   - Mismatched target and hardware:
//     * Targeting CUDA but no NVIDIA GPU detected
 //    * Targeting WASM but no emscripten/Wasmtime available
 //    * Reports as 'hardware' category 'error' with fix suggestion
//
//   - Missing optimization flags:
//     * No -O flag or suboptimal flags for hardware
 //    * Reports as 'performance' category 'hint'
//
//   - Suboptimal LTO configuration:
//     * LTO off when could benefit, or full when thin sufficient
 //    * Reports as 'performance' category 'hint'

// ============================================================================
// checkPerformance
// ============================================================================
/**
 * checkPerformance - Performance analysis
 * ==========================================================================*/
/**
 * Function: private async checkPerformance(): Promise<DiagnosticIssue[]>
 * 
 * Description:
 *   Checks for performance-related issues and optimization opportunities.
 * 
 * Checks Performed:
 * ==========================================================================*/
//   - Missing optimization flags:
//     * Project not using -O2/-O3 or hardware-specific flags
 //    * Reports as 'performance' 'hint' with recommended flags
//
//   - Missing LTO:
//     * Link-Time Optimization not enabled
 //    * Reports as 'performance' 'hint' with enabling suggestion
//
//   - Missing SIMD optimizations:
//     * Code not using available SIMD extensions (AVX2, NEON, etc.)
 //    * Reports as 'performance' 'hint' with flag additions
//
//   - Large bundle size:
//     * Output bundle exceeds recommended size thresholds
 //    * Reports as 'performance' 'warning' with optimization tips
//
//   - Missing incremental build config:
//     * No incremental/build caching enabled
 //    * Reports as 'build' 'hint' for faster rebuilds
//
//   - Suboptimal parallelism:
//     * Not using available CPU cores for parallel builds
 //    * Reports as 'build' 'hint' with recommended job count

// ============================================================================
// checkSecurity
// ============================================================================
/**
 * checkSecurity - Security analysis
 * ==========================================================================*/
/**
 * Function: private async checkSecurity(): Promise<DiagnosticIssue[]>
 * 
 * Description:
 *   Checks for security vulnerabilities and unsafe patterns.
 * 
 * Checks Performed:
 * ==========================================================================*/
//   - Vulnerable dependencies:
//     * Checks against known vulnerability databases
 //    * Reports with CVE IDs and severity scores
//
//   - Hardcoded secrets:
//     * Detects API keys, passwords, tokens in source code
 //    * Patterns: sk_live_, ACCESS_TOKEN, password, secret key patterns
 //    * Reports as 'security' 'error' with remediation steps
//
//   - Unsafe patterns:
//     * eval(), new Function(), setTimeout("...")
//     * innerHTML, dangerouslySetInnerHTML
 //    * exec() system calls
 //    * Reports as 'security' 'error' with safer alternatives
//
//   - Missing sanitizers:
//     * Debug builds without address/undefined sanitizers
 //    * Reports as 'build' 'hint' for debug builds
//
//   - Missing content security policies:
//     * For Wasm/web projects lacking CSP headers
 //    * Reports as 'security' 'info' for web projects

// ============================================================================
// checkStyle
// ============================================================================
/**
 * checkStyle - Style and formatting analysis
 * ==========================================================================*/
/**
 * Function: private async checkStyle(): Promise<DiagnosticIssue[]>
 * 
 * Description:
 *   Checks for code style and formatting issues.
 * 
 * Checks Performed:
 * ==========================================================================*/
//   - Formatting issues:
//     * Inconsistent indentation, spacing, brace placement
 //    * Reports as 'style' 'hint' with formatter integration
//
//   - Naming convention violations:
//     * Variables/functions not following conventions
 //    * camelCase vs snake_case vs PascalCase violations
 //    * Reports as 'style' 'hint' with correction suggestions
//
//   - Documentation coverage:
//     * Public functions/missing JSDoc/comments
 //    * Reports as 'style' 'info' with documentation suggestions
//
//   - Import organization:
//     * Unused imports, alphabetical ordering violations
 //    * Reports as 'build' 'hint' with cleanup suggestions

// ============================================================================
// checkHardwareCompatibility
// ============================================================================
/**
 * checkHardwareCompatibility - Hardware compatibility analysis
 * ==========================================================================*/
/**
 * Function: private async checkHardwareCompatibility(): Promise<DiagnosticIssue[]>
 * 
 * Description:
 *   Checks for hardware compatibility issues.
 * 
 * Checks Performed:
 * ==========================================================================*/
//   - CUDA target without NVIDIA GPU:
//     * Project targets 'cuda' but no NVIDIA GPU detected
 //    * Reports as 'hardware' 'error' with target change suggestion
//
//   - WASM without emscripten:
//     * Project targets 'wasm' but no WebAssembly runtime
 //    * Reports as 'hardware' 'error' with build environment note
//
//   - Memory requirement mismatches:
//     * Project requires more RAM than available
 //    * Reports as 'hardware' 'warning' with adjustment suggestion
//
//   - SIMD feature availability:
//     * Using AVX-512 but CPU doesn't support it, or vice versa
 //    * Reports as 'hardware' 'hint' with appropriate flag adjustment

// ============================================================================
// autoFix
// ============================================================================
/**
 * autoFix - Apply automatic fixes to issues
 * ==========================================================================*/
/**
 * Function: async autoFix(issues: DiagnosticIssue[]): Promise<any[]>
 * 
 * Description:
 *   Attempts to automatically fix all fixable issues from the diagnosis.
 *   Runs all fix attempts in parallel for efficiency.
 * 
 * Parameters:
 *   issues: DiagnosticIssue[] - Issues to attempt fixing
 * 
 * Returns:
 *   Promise<any[]> - Array of fix results with structure:
 *   [
 *     { issueId: "dep-conflict-foo", description: "Unify lodash", success: true },
 *     { issueId: "dep-conflict-bar", description: "Unify react", success: false, error: "..." }
 *   ]
 * 
 * Fix Attempt Behavior:
 * ==========================================================================*/
//   - Iterates through all provided issues
 //   - For each fixable issue (fixable: true with fix object):
 //     * Calls issue.fix.apply()
 //     * On success: adds { issueId, description, success: true }
 //     * On failure: adds { issueId, description, success: false, error: Error.message }
//   - Non-fixable issues are skipped (not included in results, or included with success: false)
//   - All attempts run in parallel via implicit iteration
 //   - Returns results for all attempted fixes

 // Example Results:
//   [
//     { issueId: "dep-conflict-lodash", description: "Unify lodash to v4.17.21", success: true },
//     { issueId: "dep-conflict-react", description: "Unify react to v18.2.0", success: false, error: "Permission denied writing novium.toml" }
//   ]

// ============================================================================
// generateHardwareFingerprint
// ============================================================================
/**
 * generateHardwareFingerprint - Hardware fingerprint for lockfile reproducibility
 * ==========================================================================*/
/**
 * Function: generateHardwareFingerprint(hardware: any): string
 * 
 * Description:
 *   Generates a hardware fingerprint string from a HardwareProfile.
 *   Included in lockfile metadata for reproducible builds across machines.
 * 
 * Algorithm:
 * ==========================================================================*/
//   1. Stringify hardware profile key fields:
//      cpu.model + cpu.cores
//      gpu.model + gpu.vramGB
 //      memory.totalGB
 //      os.platform + os.arch
//
//   2. Create SHA-256 hash of the stringified data
//
//   3. Return first 16 characters of hex digest
 //        // .substring(0, 16)

// Example:
//   Input: HardwareProfile with i7-9750H, 16GB RAM, Windows x64
//   Output: "a3f7c2e9b1d4f6a8" (first 16 chars of sha256 hash)

// ============================================================================
// verifyLockfile
// ============================================================================
/**
 * verifyLockfile - Verify lockfile integrity and hardware compatibility
 * ==========================================================================*/
/**
 * Function: async verifyLockfile(lockfile: ResolvedLockfile): Promise<{ valid: boolean; errors: string[] }>
 * 
 * Description:
 *   Verifies that a lockfile is valid and compatible with the current hardware.
 *   Checks hardware fingerprint and package checksums.
 * 
 * Parameters:
 *   lockfile: ResolvedLockfile - The lockfile to verify
 * 
 * Returns:
 *   Promise<{ valid: boolean; errors: string[] }>
 //   - valid: true if all checks pass
 //   - errors: Array of error strings describing any issues
 * 
 * Checks Performed:
 * ==========================================================================*/
//   - Hardware fingerprint match:
//     * Compares lockfile.metadata.hardwareFingerprint
 //      with current hardware fingerprint
 //    * Mismatch: "Hardware fingerprint mismatch - lockfile may not be reproducible"
//
//   - Package checksum verification:
//     * Iterates through lockfile.packages and verifies SHA-256 checksums
 //    * Would query actual downloaded packages for comparison
 //    * (Simplified: checks format, may skip actual verification)
//
//   - Return structure:
//     * { valid: true, errors: [] } - Lockfile is valid
 //     * { valid: false, errors: ["Hardware fingerprint mismatch"] } - Invalid

// ============================================================================
// generateInstallScript
// ============================================================================
/**
 * generateInstallScript - Generate offline installation script
 * ==========================================================================*/
/**
 * Function: generateInstallScript(result: ResolutionResult): string
 * 
 * Description:
 *   Generates a bash script for installing packages offline or in controlled
 *   environments. Based on the resolution result from dependency resolution.
 * 
 * Output Format:
//   "#!/bin/bash\n"
//   "# Auto-generated Novium installation script\n\n"
//   
//   For each installation step:
//   "echo \"Installing ${package}@${version}...\"\n"
//   "novium-pkg install ${package}@${version}\n\n"

// Example Output:
//   "#!/bin/bash\n"
//   "# Auto-generated Novium installation script\n\n"
//   "echo \"Installing novium-runtime@0.1.0...\"\n"
//   "novium-pkg install novium-runtime@0.1.0\n\n"
//   "echo \"Installing novium-utils@0.3.1...\"\n"
//   "novium-pkg install novium-utils@0.3.1\n\n"

// ============================================================================
// computeHardwareFingerprint (Utility Function)
// ============================================================================
/**
 * computeHardwareFingerprint - Compute hardware fingerprint from profile
 * ==========================================================================*/
/**
 * Function: export function computeHardwareFingerprint(hardware: any): string
 * 
 * Description:
 *   Utility function to compute hardware fingerprint from a hardware profile.
 *   Used by DependencyResolver and other components for lockfile reproducibility.
 * 
 * Algorithm:
// ==========================================================================*/
//   1. Construct data string from key hardware fields:
//      {
//        cpu: hardware.cpu.model + hardware.cpu.cores,
//        gpu: hardware.gpu?.model + hardware.gpu?.vramGB,
//        memory: hardware.memory.totalGB,
//        os: hardware.os.platform + hardware.os.arch
//      }
//
//   2. Create SHA-256 hash of JSON-stringified data
//
//   3. Return full hex digest (64 characters)

// Example:
//   Input: { cpu: "i7-9750H8", gpu: "RTX306016", memory: 16, os: "win32x64" }
//   // Actually: JSON.stringify({ cpu: "i7-9750H8", gpu: "RTX306016", memory: 16, os: "win32x64" })
//   Output: "a3f7c2e9b1d4f6a8e5c2d1f0a3b6c7d9e8f7a6b5c4d3e2f1a0b9c8d7e6f5a4" (64 char hex)

// ============================================================================
// Integration with Other Components
// ============================================================================
/*
 * Integration Points:
// ==========================================================================*/
//   - AutoConfigurator: Uses diagnostics to inform configuration choices
 //   - BuildOptimizer: Uses diagnostic results to optimize build parallelism/flags
 //   - ErrorCorrector: Uses diagnosed issues as input for auto-fix + compile
 //   - VS Code Extension: Shows diagnostic results in UI, runs on command
 //   - Novium CLI: 'diagnose' command uses this engine
 //   - DependencyResolver: Uses fingerprint for lockfile generation
 //
 // Data Flow:
//   1. Engine.runFullDiagnosis(projectRoot) -> DiagnosticReport
 //   2. Report.issues analyzed for fixability
 //   3. Engine.autoFix(issues) -> fix results
 //   4. Fixed code re-compiled via BuildOptimizer or CLI
 //   5. Report shown to user via VS Code notification or webview

 // ============================================================================
 // Version History
 // ============================================================================
/*
 * Version 1.0.0 (2026-08-18)
 *   - Initial release
 //   - All 6 diagnostic check categories implemented
 //   - Auto-fix capability with category-specific patterns
 //   - Parallel diagnosis execution
 //   - Hardware compatibility checking
 //   - Dependency conflict detection
 //   - Security vulnerability scanning
 //   - Performance optimization analysis
 //   - Style/formatting analysis
 //   - DiagnosticReport with summary and metrics
 //   - autoFix() method with success/failure reporting
 //   - generateHardwareFingerprint() utility
 //   - verifyLockfile() integration
 //   - generateInstallScript() utility
 //   - computeHardwareFingerprint() export
 //   - Integration with AutoConfigurator, BuildOptimizer, ErrorCorrector
 //   - VS Code extension integration
 //   - Novium CLI 'diagnose' command support

 // ============================================================================
 // Known Limitations
 // ============================================================================
/*
 * - Security scanning depth configurable but may miss obscure vulnerabilities
 //   - Hardcoded secret patterns may have false positives/negatives
 //   - Vulnerability database coverage depends on API access and recency
 //   - Auto-fix patterns cover common cases but not all code patterns
 //   - Hardware compatibility checks depend on accurate hardware detection
 //   - Some style issues require human judgment for correct fixes
 //   - Parallel checks may have shared state issues (mitigated by design)

// ============================================================================
 // Requirements
 // ============================================================================
/*
 * Node.js Environment:
//   - Node.js 16+ (for async/await, crypto.createHash)
//   - Access to project files (novium.toml, source .nvm/.nvi/.nvw files)
//   - Permissions to read/write project configuration files
 //
 * Novium Project Requirements:
//   - novium.toml or project configuration file must exist
 //   - Source files accessible for parsing
 //   - Diagnostic engine works with all three language layers (.nvm, .nvi, .nvw)
 //   - Hardware detection should run prior to diagnosis for hardware-aware checks

// ============================================================================
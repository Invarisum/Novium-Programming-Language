# Novium Complete Documentation
## Full Ecosystem Reference Guide

*Generated: August 18, 2026*

---
## Table of Contents
1. [Overview](#overview)
2. [Language Layers](#language-layers)
3. [Go CLI (`novium-cli`)](#go-clinicli)
4. [VS Code Adaptive Extension](#vs-code-adaptive-extension)
5. [Error Correction Feature](#error-correction-feature)
6. [Python Package Manager](#python-package-manager)
7. [Learning Guide](#learning-guide)
8. [Example Files](#example-files)
9. [Build Targets](#build-targets)
10. [Configuration](#configuration)
11. [CLI Commands Reference](#cli-commands-reference)
12. [Hardware Detection](#hardware-detection)
12. [Troubleshooting](#troubleshooting)

---
## Overview

Novium is a complete programming language ecosystem designed for high-performance systems programming, dynamic orchestration, and modern web application development. The ecosystem consists of three primary language layers plus a comprehensive developer toolchain that auto-configures itself based on detected hardware.

**Key Features:**
- **Zero-config auto-detection** - Hardware detection drives all configuration
- **Multi-target compilation** - Native, WASM, CUDA, and GPU targets
- **Dependency conflict prevention** - SAT-based resolution with lockfile generation
- **Blazingly fast incremental builds** - Parallel compilation with optimal batch sizing
- **Self-diagnosing toolchain** - Automatic issue detection and fixing
- **Hardware-optimized builds** - Compiler flags tuned to detected CPU/GPU

The ecosystem is designed to eliminate "dependency hell" and provide optimal performance across all target platforms without manual configuration.

---
## Language Layers

### 1. Novium Compiled (`.nvm`)
**File Extension:** `.nvm`
**Paradigm:** Systems programming, compiled
**Compiler:** LLVM-based native compiler (C++17 backend)

**Features:**
- Rust-like ownership/borrowing semantics
- Static type inference
- Pattern matching with exhaustive checking
- Null-safe types (`string?`, `int?`)
- Inter-language interop with `.nvi` and `.nvw`
- SIMD optimization auto-detection
- LTO (Link-Time Optimization) support

**Hello World Example:**
```novium
fn main() void:
    print("Hello from Novium!")
    
fn fibonacci(n: int) int:
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)
```

**Compilation Targets:**
- `native` - Native x86_64/arm64 executable
- `wasm` - WebAssembly for browser/Node.js
- `cuda` - NVIDIA GPU computing
- `gpu` - General GPU computing (OpenCL/Metal)

### 2. Novium Interpreter (`.nvi`)
**File Extension:** `.nvi`
**Paradigm:** Dynamic scripting, garbage-collected
**Runtime:** Python-like virtual machine

**Features:**
- Dynamic type system with optional static typing
- Zero-overhead FFI to `.nvm` shared libraries
- Interactive REPL mode
- Full access to system Python ecosystem
- Easy learning curve for beginners

**Hello World Example:**
```nvi
def main():
    print("Hello from Novium Interpreter!")
    
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)
```

**Interop with `.nvm`:**
```novium
// .nvm can import and use .nvi functions
import "my_module.nvi"

fn main() void:
    result = fibonacci(10)
    print("fibonacci(10) = ${result}")
```

### 3. Novium Web (`.nvw`)
**File Extension:** `.nvw`
**Paradigm:** Web components, Wasm compilation
**Transpiler:** Next.js-like component syntax → WebAssembly + JavaScript

**Features:**
- Component-based architecture
- WASM output with JavaScript bindings
- DOM integration
- Reactive state management
- CSS-in-JS support
- Progressive Web App capabilities

**Hello World Example:**
```nvw
<!-- Component definition -->
@Component
fn HelloWorld():
    div {
        h1("Hello from Novium Web!")
        p("This component compiles to Wasm + JS")
    }
    
<!-- Usage in HTML -->
<hello-world></hello-world>
```

---
## Go CLI (`novium-cli`)

### Location
`novium-cli/main.go` - Complete Go source code (requires Go toolchain to compile)

### Overview
The Novium Command Line Interface provides 6 essential commands for project management, build automation, and toolchain configuration. All commands feature hardware-aware auto-configuration.

### Available Commands

#### `novium init`
**Description:** Initialize a new Novium project with hardware detection and auto-configuration.
**Usage:**
```bash
novium init
```
**What it does:**
1. Detects CPU, GPU, RAM, and OS hardware
2. Generates `novium.toml` with optimized configuration
3. Creates project directory structure (`src/`, `examples/`, `tests/`, `.novium/`)
4. Generates `main.nvm` template file
5. Creates `novium.lock` lockfile
6. Generates `CMakeLists.txt` for build system
7. Installs initial dependencies

**Generated Configuration Example:**
```toml
name = "my-project"
version = "0.1.0"

[build]
target = "auto"   # auto, native, wasm, cuda, gpu
optimizationLevel = "speed"   # speed, size, balanced
parallelJobs = 8
incremental = true
lto = "thin"
debugInfo = true
sanitizers = ["address", "undefined"]

[dependencies]
# Auto-resolved dependencies
```

#### `novium new`
**Description:** Create a new Novium project from template.
**Usage:**
```bash
novium new my-project
```
**Options:**
- `--template` - Specify template (default, cuda, wasm, minimal)
- `--force` - Overwrite existing project files

#### `novium build`
**Description:** Build Novium project for specified target.
**Usage:**
```bash
novium build --target cuda
novium build --target gpu
novium build --target native
novium build --target wasm
```
**Options:**
- `--target` - Compilation target (default: auto-detected)
- `--optimize` - Optimization level (speed|size|balanced)
- `--incremental` - Enable incremental builds
- `--cache` - Enable build caching

**Build Process:**
1. Loads project configuration from `novium.toml`
2. Detects hardware if not already configured
3. Generates optimal compiler flags for detected hardware
4. Compiles source files in parallel (optimal batch sizing)
5. Applies link-time optimization (thin LTO)
6. Generates output appropriate to target:
   - native: Native executable
   - wasm: WebAssembly module
   - cuda: GPU kernel + host wrapper
   - gpu: General GPU compute kernel

**Example: Build for CUDA**
```bash
novium build --target cuda --optimize speed
```
*Will auto-detect NVIDIA GPU and generate appropriate `-arch=sm_86` flags*

#### `novium test`
**Description:** Run tests for Novium project.
**Usage:**
```bash
novium test
novium test --target cuda
```
**Features:**
- Runs unit tests embedded in `.nvm` files
- Hardware-aware test execution
- Coverage reporting
- Performance benchmarking

#### `novium fmt`
**Description:** Format Novium source files.
**Usage:**
```bash
novium fmt
novium fmt --check  # Check format only, don't modify
```
**Supported Formats:**
- `.nvm` files - Novium source formatting
- `.nvi` files - Interpreter source formatting
- `.nvw` files - Web component formatting

#### `novium install`
**Description:** Install dependencies for Novium project.
**Usage:**
```bash
novium install
novium install --force  # Force reinstallation
```
**Features:**
- Resolves dependencies from Novium package registry
- Generates/updates `novium.lock` lockfile
- Prevents dependency version conflicts
- Offline mode supported (using existing lockfile)
- Hardware-fingerprinted lockfiles for reproducibility

---
## VS Code Adaptive Extension

### Location
`novium-adaptive-extension/` - TypeScript source files

### Overview
The Novium Adaptive VS Code Extension provides a zero-config, self-optimizing toolchain directly in the editor. It auto-detects hardware, prevents dependency conflicts, and compiles blazingly fast for any target.

### Features

#### Hardware Detection
Auto-detects and configures for:
- CPU: Cores, threads, model, frequency, SIMD extensions (SSE, AVX, AVX2, AVX-512, NEON)
- GPU: Vendor, model, VRAM, compute capability (CUDA, OpenCL, Metal)
- Memory: Total and available GB, speed, channels
- Storage: Type, free space, IO speed
- OS: Platform, architecture, distribution
- Container/virtualization status

**Detected Profile Example:**
```json
{
  "cpu": {
    "cores": 8,
    "threads": 16,
    "model": "Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz",
    "frequencyMHz": 2600,
    "simd": ["SSE", "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2", "AVX", "AVX2"],
    "architecture": "x86_64"
  },
  "gpu": {
    "vendor": "nvidia",
    "model": "GeForce GTX 1650",
    "vramGB": 4,
    "computeCapability": "sm_86"
  },
  "memory": {
    "totalGB": 16,
    "availableGB": 8,
    "speedMTs": 2666,
    "channels": 2
  },
  "os": {
    "platform": "win32",
    "arch": "x64",
    "kernelVersion": "10.0.19045"
  }
}
```

#### Auto-Configuration
The extension automatically generates optimal configuration based on detected hardware:

**Configuration Generated:**
```json
{
  "name": "novium-project",
  "version": "0.1.0",
  "target": "auto",          // auto-detected: native, wasm, cuda, or gpu
  "optimizationLevel": "speed",  // speed|size|balanced (based on RAM)
  "cudaArch": "sm_86",       // auto-detected from NVIDIA GPU
  "parallelJobs": 15,        // CPU threads - 1
  "optimizationFlags": ["-O3", "-march=native", "-mtune=native", "-flto=thin"],
  "lto": "thin",
  "debugInfo": true,
  "sanitizers": ["address", "undefined"],
  "buildCacheDir": ".novium/cache",
  "incrementalBuild": true
}
```

**Target Determination Logic:**
```typescript
if (gpu?.vendor === 'nvidia' && gpu.computeCapability) {
  return 'cuda';  // Auto-target CUDA if NVIDIA GPU detected
}
if (gpu?.vendor === 'amd' || gpu?.vendor === 'intel') {
  return 'gpu';   // OpenCL/Metal path
}
return 'native';   // Default to native compilation
```

#### Optimization Profiles

**Speed Profile** (default, for development machines with 8+GB RAM):
- Compiler: `-O3 -march=native -mtune=native`
- LTO: thin
- Defines: `NDEBUG=1`, `OPTIMIZE_SPEED=1`

**Size Profile** (for low-memory machines < 8GB RAM):
- Compiler: `-Os -march=native -mtune=native`
- LTO: full
- Defines: `OPTIMIZE_SIZE=1`
- Aggressive function inlining removal

**Balanced Profile** (default fallback):
- Compiler: `-O2 -march=native -mtune=native`
- LTO: thin
- Defines: `OPTIMIZE_BALANCED=1`

#### Commands Registered

1. **`novium-adaptive.initialize`**
   - Initializes full Novium adaptive toolchain
   - Runs hardware detection
   - Generates project config
   - Creates project structure
   - Installs dependencies
   - Generates VS Code launch configs

2. **`novium-adaptive.optimize`**
   - Re-optimizes for current hardware
   - Updates `novium.toml` with new settings
   - Regenerates build configurations

3. **`novium-adaptive.diagnose`**
   - Runs full diagnostic analysis
   - Detects issues in: dependencies, build config, performance, security, style, hardware
   - Auto-fixes fixable issues
   - Shows diagnostic report in VS Code panel

4. **`novium-adaptive.build`**
   - Builds for selected target
   - Options: native, wasm, cuda, auto
   - Progress notification during build
   - Shows success/failure message

#### Key Interface Files

| File | Purpose |
|------|---------|
| `src/hardware-detector.ts` | Auto-detects CPU, GPU, RAM, storage, OS |
| `src/auto-configurator.ts` | Generates optimal project config from hardware |
| `src/dependency-resolver.ts` | Zero-conflict dependency management |
| `src/build-optimizer.ts` | Blazingly fast incremental builds |
| `src/diagnostic-engine.ts` | Automated issue detection and fixing |
| `src/novium-cli.ts` | Wraps Novium CLI for extension |
| `src/extension.ts` | Main entry point (334 lines) |

#### Language Support
The extension supports Novium file types:
- `.nvm` - Novium Compiled source files
- `.nvi` - Novium Interpreter source files  
- `.nvw` - Novium Web source files

Configuration in `package.json`:
```json
"languages": [
  {
    "id": "novium",
    "extensions": [".nvm", ".nvi", ".nvw"],
    "aliases": ["Novium", "novium"]
  }
]
```

---
## Error Correction Feature

### Location
`novium-adaptive-extension\error-correction\` - Auto-fix and compile feature

### Overview
The Error Correction feature automatically detects, fixes, and reports critical code errors during Novium compilation. When compiling code, it:

1. **Diagnoses** source code for issues
2. **Auto-fixes** all fixable problems
3. **Compiles** the corrected code
4. **Reports** what bugs were fixed and what remains

### How It Works

#### 1. Issue Diagnosis
The error corrector analyzes source code for issues in these categories:

**Dependency Issues:**
- Version conflicts between dependencies
- Duplicate imports
- Unresolved type references

**Build Issues:**
- Missing imports or type annotations
- Syntax errors (missing semicolons, mismatched braces)
- Undefined variables
- Type mismatches

**Performance Issues:**
- Inefficient loop patterns
- Unnecessary computations
- Suboptimal algorithm usage
- Cache locality issues

**Security Issues:**
- `eval()` usage (dangerous pattern)
- Unsanitized input handling
- XSS vulnerabilities
- Hardcoded secrets/keys

**Style Issues:**
- Improper formatting
- Naming convention violations
- Missing documentation on public APIs
- Inconsistent code style

**Hardware Issues:**
- Target mismatch (CUDA without NVIDIA GPU)
- Missing hardware-specific optimizations
- Insufficient memory for build operations
- SIMD feature availability mismatches

#### 2. Automatic Fixing
For each detected issue, the corrector applies category-specific fixes:

**Dependency Fixes:**
- Unifies duplicate dependency versions
- Removes unused imports
- Resolves version conflicts using SAT solver

**Build Fixes:**
- Adds missing type annotations
- Fixes syntax errors automatically
- Removes undefined variables
- Optimizes build configuration

**Performance Fixes:**
- Simplifies inefficient loops
- Removes unnecessary computations
- Inlines small functions automatically
- Optimizes memory access patterns

**Security Fixes:**
- Replaces `eval()` with safer alternatives
- Adds input sanitization
- Removes hardcoded secrets
- Applies proper escaping

**Style Fixes:**
- Runs code formatter in check-fix mode
- Standardizes naming conventions
- Adds JSDoc documentation patterns
- Enforces code style rules

**Hardware Fixes:**
- Adjusts target based on detected hardware
- Adds/removes architecture-specific flags
- Optimizes for available memory
- Adjusts parallel job counts

#### 3. Compilation
After fixing, the corrected code is compiled with the selected target:

**Compilation Process:**
1. Write corrected code to temporary file
2. Run Novium compiler with optimal flags
3. Capture all output (errors, warnings, performance stats)
4. Analyze compilation results
5. Map compilation errors back to source issues

**Supported Targets:**
- `native` - C++ LLVM backend compilation
- `wasm` - Emscripten WebAssembly compilation
- `cuda` - NVCC CUDA compilation
- `gpu` - OpenCL/Metal GPU compilation

#### 4. Post-Compilation Reporting
After compilation, the error corrector generates a detailed report:

**Report Format:**
```
=== Error Correction Summary ===

Fixed Issues (3):
  - [FIXED] Triple semicolon statement separator detected and removed
  - [FIXED] Unused variable 'y' removed (was assigned but never used)
  - [FIXED] Missing closing parenthesis in function call fixed

Unfixable Issues (2):
  - [UNFIXED] Function 'greet' has missing return type - requires manual annotation
  - [UNFIXED] Dependency version conflict 'pkg@2.3.4' vs 'pkg@2.4.0' - version mismatch

Compilation Errors (1):
  - Triple brace nesting exceeds maximum depth - requires code restructuring

Compilation: SUCCESS
Fixed Code Length: 842 characters
```

**Bugs Displayed:**
- ✅ **Fixed bugs** - All automatically resolved issues with descriptions
- ❌ **Remaining errors** - Issues that still need manual attention
- 📋 **All encountered bugs** - Complete list of issues found during diagnosis

### Test File (`test-code.nvm`)
Contains intentionally broken Novium code demonstrating 8 bug types:

```novium
// Bug 1: Unused variable
fn main() void:
    let x: int = 42
    let y: int = 100
    print("Hello, Novium!")
    print("Value: ${x}")

// Bug 2: Triple semicolon (syntax error)
fn compute(x: int, y: int) int:
    result := x + y; ; ;
    return result

// Bug 3: Missing semicolon in return
fn factorial(n: int) int:
    if n <= 1:
        return n
    return n * factorial(n - 1)

// Bug 4: Unused variable pattern
fn unused_var() void:
    let a: int = 10
    let b: int = 20
    let c: int = a + b
    // c is never used

// Bug 5: Misplaced brace
fn test_braces() void:
    if true:
        print("Inside if")
    print("Outside if")

// Bug 6: Double definition
let x: int = 5
let x: int = 10

// Bug 7: Missing return type
fn greet(name: string) void:
    print("Hello, ${name!)

// Bug 8: Infinite loop pattern
fn maybe_infinite() void:
    while true:
        print("Looping...")
        break
```

### Usage

#### Via TypeScript/Node.js
```typescript
import { ErrorCorrectionModule } from './error-correction';

const module = new ErrorCorrectionModule();

const result = await module.correctAndCompile({
  sourceCode: `fn main() void: print("Hello")`,
  target: 'native',
  projectRoot: './my-project'
});

console.log(result.summary);
console.log('Fixed code:', result.fixedCode);
```

#### Via Terminal/CLI
```bash
node error-correction.js native test-code.nvm
```

**Sample Output:**
```
=== Error Correction Summary ===

Fixed Issues (3):
  - [FIXED] Triple semicolon statement separator detected and removed
  - [FIXED] Unused variable 'y' removed (was assigned but never used)
  - [FIXED] Missing closing parenthesis in function call fixed

Unfixable Issues (2):
  - [UNFIXED] Function 'greet' has missing return type - requires manual annotation
  - [UNFIXED] Dependency version conflict 'pkg@2.3.4' vs 'pkg@2.4.0' - version mismatch

Compilation Errors (1):
  - Triple brace nesting exceeds maximum depth - requires code restructuring

Compilation: SUCCESS
Fixed Code Length: 842 characters

--- Fixed Code ---
fn main() void:
    let x: int = 42
    print("Hello, Novium!")
    print("Value: ${x}")
```

#### Via VS Code Extension
The "Diagnose & Fix" command in the Novium Adaptive Extension runs the error corrector and displays results in a webview panel with:
- Summary of all fixes applied
- List of fixed bugs with descriptions
- List of unfixable bugs
- Remaining compilation errors
- Final fixed code display

### Configuration
Error correction can be configured via `novium.toml`:

```toml
[error_correction]
  autoFix = true        # Auto-apply fixes during compilation (default: true)
  maxAutoFixAttempts = 3  # Maximum auto-fix attempts per compilation (default: 3)
  reportStyle = "detailed"  # Summary report style: "brief" or "detailed" (default: detailed)
  enableSecurityScanning = true  # Enable security issue detection (default: true)
  enablePerformanceOptimization = true  # Enable performance issue fixing (default: true)
```

---
## Python Package Manager

### Location
`novium_pkg_manager.py` - Full Python script for dependency management

### Overview
The Novium Python Package Manager handles dependency installation, version resolution, and lockfile generation for Novium projects.

### Features

**Dependency Resolution:**
- SAT-based version conflict resolution
- Transitive dependency analysis
- Hardware-fingerprinted lockfiles
- Offline mode support

**Package Sources:**
- Novium package registry
- Git repositories
- Local directories
- Wheel/egg distributions

**Lockfile Format:**
```json
{
  "version": 1,
  "generatedAt": "2026-08-18T10:30:00Z",
  "packages": {
    "novium-runtime": {
      "name": "novium-runtime",
      "version": "1.0.0",
      "source": "registry",
      "checksum": "a1b2c3d4e5f6...",
      "dependencies": {
        "novium-base": "1.0.0"
      },
      "transitive": true
    }
  },
  "metadata": {
    "generatedBy": "novium-pkg-manager",
    "resolverVersion": "1.0.0",
    "hardwareFingerprint": "d4e5f6a7b8c9..."
  }
}
```

**Usage:**
```bash
python novium_pkg_manager.py install
python novium_pkg_manager.py resolve
python novium_pkg_manager.py lock
python novium_pkg_manager.py update
python novium_pkg_manager.py offline
```

### Dependency Conflict Prevention
The package manager explicitly prevents:
- Multiple versions of the same package
- Version range conflicts
- Removed/Deprecated package versions
- Incompatible dependency combinations

All lockfiles include hardware fingerprint for reproducibility across different machines.

---
## Learning Guide

### Location
`full-on learn guide.md` - 100+ page comprehensive guide

### Overview
The learning guide covers the entire Novium ecosystem from beginner to advanced topics across all three language layers.

### Structure

#### Part 1: Novium Compiled (`.nvm`)
**Topics Covered:**
- Syntax fundamentals (indentation-based blocks, colons, braces)
- Function definitions and parameters
- Type system (int, string, float, bool, null-safe types `type?`)
- Pattern matching (match expressions with exhaustive checking)
- Ownership and borrowing semantics (Rust-inspired)
- Memory management and lifetime rules
- Module system and import/export
- Error handling (result types, exception patterns)
- Generic functions and type parameters
- Concurrency model (channels, mutexes, atomic operations)
- FFI (Foreign Function Interface) for C/C++ integration
- LLVM backend and code generation
- Optimization flags and performance tuning
- CUDA GPU computing integration
- Build system integration (CMake, Ninja)

**Key Concepts:**
- `fn` keyword for function definitions
- `void` return type for functions without return
- `int`, `string`, `float`, `bool` primitive types
- `let` for variable declarations
- Colons `:` for block scoping
- Curly braces `{}` for inline blocks
- `print()` for output
- `import` for module inclusion

#### Part 2: Novium Interpreter (`.nvi`)
**Topics Covered:**
- Dynamic type system fundamentals
- Variable declaration with `def`
- String interpolation (`"Hello ${name}"`)
- Built-in functions (print, len, type, etc.)
- Control flow (if/else, while, for)
- Function definitions and return values
- Closures and higher-order functions
- List and dictionary data structures
- FFI to load `.nvm` shared libraries
- REPL interactive mode
- Error handling (try/except)
- Python interop features
- Dynamic dispatch and method resolution

**Key Concepts:**
- `def` keyword for function definitions
- Indentation-based block structure
- Dynamic typing with optional type annotations
- `None` equivalent for null values
- Built-in collection types (lists, dicts)
- `import` for module loading from `.nvm` files

#### Part 3: Novium Web (`.nvw`)
**Topics Covered:**
- Component syntax and definitions
- JSX-like template expressions
- WebAssembly compilation pipeline
- JavaScript bindings generation
- DOM event handling
- Reactive state management
- CSS-in-JS patterns
- Progressive Web App features
- SSR (Server-Side Rendering) support
- Hot Module Replacement (HMR)
- Build optimization for web
- Browser compatibility tables
- Performance profiling for Wasm

**Key Concepts:**
- `@Component` decorator for component classes
- `@Prop` for component properties
- `@Event` for event handlers
- Template literals for HTML output
- `useState` hook for reactive state
- `useEffect` hook for side effects
- `wasmExport` for exposing functions to JS

#### Part 4: Inter-Language Interop
**Topics Covered:**
- Calling `.nvm` functions from `.nvi`
- Calling `.nvi` functions from `.nvm`
- `.nvw` component interaction with JS
- Shared data structures across layers
- Memory management across runtimes
- FFI function signature conventions
- Name mangling and export conventions
- Bridge layer implementation details

**Interop Patterns:**
```novium
// .nvm calling .nvi
import "stats.nvi"

fn main() void:
    data = [1, 2, 3, 4, 5]
    avg = stats.mean(data)
    print("Average: ${avg}")
```

```nvi
# .nvi calling .nvm
import "math.nvm" 

def main():
    result = math.fibonacci(10)
    print("fibonacci(10) = ${result}")
```

```nvw
# .nvw interacting with JS
@Component
fn Button(props):
    onClick {
        js.alert("Button clicked!")
    }
    div {
        button("Click me")(onClick)
    }
```

#### Part 5: Developer Toolchain
**Topics Covered:**
- Go CLI commands (init, new, build, test, fmt, install)
- VS Code extension setup and configuration
- Python package manager usage
- Hardware detection and auto-configuration
- Error correction and automatic fixing
- Incremental build systems
- Dependency management and lockfiles
- CI/CD pipeline integration
- Cross-platform development
- Performance profiling and optimization

**Tool Integration:**
- Novium CLI: `novium` command
- VS Code: Adaptive extension features
- Python: Package manager scripts
- Build: CMake + Ninja for native, Emscripten for Wasm, NVCC for CUDA

#### Part 6: Advanced Topics
**Topics Covered:**
- Custom compiler passes
- Extending the language syntax
- Performance benchmarking methodologies
- Memory optimization techniques
- Parallel and concurrent programming
- GPU computing patterns
- WebAssembly performance tuning
- Cross-compilation strategies
- Custom build system integration
- Plugin architecture for the Novium ecosystem

### Getting Started

**Installation:**
1. Install the Novium compiler (C++17 + CMake + MinGW)
2. Install Go for CLI tools (`go install`)
3. Install Node.js for VS Code extension
4. Run `novium init` to initialize first project

**First Project:**
```bash
# Initialize new project
novium init

# Create new file
echo 'fn main() void: print("Hello, Novium!")' > main.nvm

# Build for native target
novium build

# Build for WASM
novium build --target wasm

# Build for CUDA (if NVIDIA GPU)
novium build --target cuda
```

**VS Code Integration:**
1. Open VS Code in project directory
2. Install "Novium Adaptive" extension from VS Code Marketplace
3. Use Command Palette (Ctrl+Shift+P) to access commands:
   - "Novium: Initialize Adaptive Toolchain"
   - "Novium: Optimize for Current Hardware"
   - "Novium: Diagnose & Fix Issues"
   - "Novium: Build for Target"

**Python Package Management:**
```bash
python novium_pkg_manager.py init
python novium_pkg_manager.py install
python novium_pkg_manager.py lock
```

---
## Example Files

### .nvm Examples (Novium Compiled)

**`hello.nvm`** - Basic hello world:
```novium
fn main() void:
    print("Hello, Novium World!")
```

**`fibonacci.nvm`** - Recursive function:
```novium
fn main() void:
    for i in range(10):
        print("fibonacci(${i}) = ${fibonacci(i)}")
    
fn fibonacci(n: int) int:
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)
```

**`matrix.nvm`** - SIMD-optimized matrix multiplication:
```novium
fn main() void:
    let a: matrix_t = load_matrix("data1.txt")
    let b: matrix_t = load_matrix("data2.txt")
    let result: matrix_t = multiply_matrices(a, b)
    save_matrix(result, "output.txt")
    
fn multiply_matrices(a: matrix_t, b: matrix_t) matrix_t:
    // SIMD-optimized implementation
    // Auto-vectorized by LLVM based on detected CPU features
    result := create_matrix(rows(a), cols(b))
    for i in range(rows(a)):
        for j in range(cols(b)):
            for k in range(cols(a)):
                result[i][j] += a[i][k] * b[k][j]
    return result
```

### .nvi Examples (Novium Interpreter)

**`hello.nvi`** - Basic hello world:
```nvi
def main():
    print("Hello from Novium Interpreter!")
    
def greeting(name):
    return "Hello, " + name + "!"
    
greeting("User")
```

**`fibonacci.nvi`** - Recursive function:
```nvi
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

def main():
    for i in range(10):
        print("fibonacci(%d) = %d" % (i, fibonacci(i)))
```

**`repl.nvi`** - Interactive REPL:
```nvi
# Run with: python -m novium.repl
print("Novium REPL started!")
print("Type 'exit()' to quit")

while True:
    try:
        user_input = input("novium> ")
        if user_input.strip().lower() == "exit()":
            break
        # Simple evaluation
        result = eval(user_input)
        print(result)
    except EOFError:
        break
    except Exception as e:
        print("Error: %s" % str(e))
```

### .nvw Examples (Novium Web)

**`component.nvw`** - Basic component:
```nvw
@Component
fn HelloWorld():
    div {
        h1("Hello from Novium Web!")
        p("This compiles to WebAssembly + JavaScript")
    }
```

**`todo.nvw`** - Todo application:
```nvw
<!-- State management -->
.state
  let items: list<string> = []
  let filter: string = "all"

<!-- Component -->
@Component
fn TodoItem({item, onDelete}):
  div.class("todo-item") {
    div.class("view") {
      input.type("checkbox")(checked=item in .state.filter)
      label(.item) { ${item} }
      button.class("delete")(onClick=onDelete) { "×" }
    }
  }

<!-- Todo list -->
@Component
fn TodoList():
  div.class("todo-list") {
    input.type("text")
      .on("input", e => {
        .state.items.push(e.target.value)
        e.target.value = ""
      })
    
    ul {
      .state.items.map(item => 
        <TodoItem item={item} onDelete={() => .state.items.remove(item)} />
      )
    }
  }

<!-- Usage -->
<todo-list></todo-list>
```

### .nvm Example: CUDA Integration

**`cuda-demo.nvm`** - NVIDIA GPU computation:
```novium
fn main() void:
    // Allocate data on GPU
    let gpu_data: gpu_buffer_t = gpu_alloc(1024 * 1024)  // 1MB
    
    // Initialize data
    gpu_init(gpu_data, 1024)
    
    // Launch CUDA kernel
    gpu_kernel(gpu_data, 1024)  // Process 1024 elements
    
    // Read back results
    let host_data: array_t = gpu_read(gpu_data)
    
    // Print first 10 results
    for i in range(10):
        print("Element ${i}: ${host_data[i]}")
    
    // Cleanup
    gpu_free(gpu_data)
```

---
## Build Targets

### Target: `native`
**Description:** Native CPU compilation via LLVM backend
**Compiler:** `clang++` with LLVM optimizations
**Output:** Native executable binary
**Flags:** Auto-generated based on detected CPU features
- `-march=native -mtune=native` (if AVX2/AVX-512 detected)
- `-O3` (speed) or `-Os` (size)
- `-flto=thin` (Link-Time Optimization)
- `-fno-exceptions -fno-rtti` (Novium runtime settings)

**Best for:** Desktop applications, server-side tools, CPU-intensive computing

### Target: `wasm`
**Description:** WebAssembly compilation via Emscripten
**Compiler:** `em++` (Emscripten clang++ wrapper)
**Output:** `.wasm` binary + `.js` glue code
**Flags:**
- `-s WASM=1` - Enable WebAssembly output
- `-s ALLOW_MEMORY_GROWTH=1` - Allow dynamic memory growth
- `-s MODULARIZE=1` - Enable modular output (single JS file)
- `-O2` optimization balance
- `-s "MODULARIZE=1 -s EXPORTED_FUNCTIONS='[\"_main\"]'"`

**Best for:** Web applications, browser-based tools, client-side computing

### Target: `cuda`
**Description:** NVIDIA GPU computing via NVCC
**Compiler:** `nvcc` (NVIDIA CUDA Compiler)
**Output:** GPU kernel binary + host wrapper
**Flags:**
- `-arch=sm_86` (auto-detected: sm_70, sm_75, sm_80, sm_86, sm_89, sm_90)
- `-std=c++17` C++ standard
- `--use_fast_math` - Fast math operations (faster, slightly less precise)
- `--fmad=true` - Fused multiply-add instruction
- `--ptxas-options=-v` - Verbose PTX assembly output
- `--compiler-options '-O3 -march=native'` - Host compiler flags

**Requirements:**
- NVIDIA GPU with compute capability support
- CUDA toolkit installed (or bundled flags)
- Supported GPU: sm_70 (Pascal) and newer

**Best for:** GPGPU computing, machine learning inference, graphics processing, parallel algorithms

### Target: `gpu`
**Description:** General GPU computing (OpenCL/Metal)
**Compiler:** Platform-specific (clang++ with OpenCL/nvcc with Metal)
**Output:** GPU compute kernel
**Flags:** Vendor-specific optimization for detected GPU

**Vendor Support:**
- **NVIDIA:** OpenCL ICD, CUDA interop
- **AMD:** ROCm, OpenCL support
- **Intel:** OpenCL, oneAPI support
- **Apple:** Metal API (macOS/iOS)

**Best for:** Cross-platform GPU computing, heterogeneous systems

---
## Configuration

### `novium.toml` Format
Project configuration file generated automatically by `novium init` and the VS Code extension:

```toml
# Project identity
name = "my-novium-project"
version = "0.1.0"

# Build configuration
[build]
# Target auto-detection based on hardware
target = "auto"   # auto, native, wasm, cuda, gpu

# Optimization priority
optimizationLevel = "speed"  # speed, size, balanced

# Parallel build configuration
parallelJobs = 15        # Auto-detected: CPU threads - 1
incremental = true       # Enable incremental builds
lto = "thin"           # LTO level: off, thin, full

# Debug and safety settings
debugInfo = true       # Include debug symbols
sanitizers = ["address", "undefined"]  # TSAN, MSAN enabled

# Hardware-optimized flags (auto-generated)
cflags = "-O3 -march=native -mtune=native -flto=thin"
lflags = "-flto=thin -Wl,--gc-sections"

# CUDA-specific (if targeting cuda target)
[build.cuda]
arch = "sm_86"         # Compute capability
debug = false          # Kernel debug mode

# Dependency configuration
[dependencies]
novium-runtime = "1.0.0"
math-utils = "2.3.1"

# Error correction settings
[error_correction]
autoFix = true
maxAutoFixAttempts = 3
reportStyle = "detailed"
```

### VS Code Settings (`settings.json`)
```json
{
  "novium.adaptive.autoDetectHardware": true,
  "novium.adaptive.target": "auto",
  "novium.adaptive.optimizationLevel": "speed",
  "novium.adaptive.strictDependencyResolution": true,
  "novium.adaptive.cudaArch": "auto",
  "novium.adaptive.parallelJobs": 0
}
```

**Settings Descriptions:**
- `novium.adaptive.autoDetectHardware`: Auto-detect and configure for CPU/GPU/RAM (default: true)
- `novium.adaptive.target`: Compilation target (auto, native, wasm, cuda, gpu) (default: auto)
- `novium.adaptive.optimizationLevel`: Optimization priority (speed, size, balanced) (default: speed)
- `novium.adaptive.strictDependencyResolution`: Fail on any version conflict (default: true)
- `novium.adaptive.cudaArch`: CUDA compute capability (auto, sm_70, sm_75, sm_80, sm_86, sm_89, sm_90) (default: auto)
- `novium.adaptive.parallelJobs`: Parallel build jobs (0 = auto-detect CPU cores) (default: 0)

### Error Correction Configuration
```toml
[error_correction]
  autoFix = true                # Auto-apply fixes during compilation
  maxAutoFixAttempts = 3        # Max auto-fix attempts per compilation
  reportStyle = "detailed"      # "brief" or "detailed" summary style
  enableSecurityScanning = true # Enable security issue detection
  enablePerformanceOptimization = true  # Enable performance issue fixing
```

---
## CLI Commands Reference

### Command: `novium init`
```
Usage: novium init [--force]
```

**Description:** Initialize Novium project with hardware detection and auto-configuration.

**Options:**
- `--force` - Overwrite existing project files

**What it does:**
1. Detects CPU, GPU, RAM, storage, OS
2. Generates `novium.toml` with optimized settings
3. Creates directory structure: `src/`, `examples/`, `tests/`, `.novium/`
4. Writes `main.nvm` template
5. Generates `novium.lock` lockfile
6. Creates `CMakeLists.txt` for build system
7. Runs initial dependency resolution

**Example:**
```bash
novium init
# Or with force:
novium init --force
```

---

### Command: `novium new`
```
Usage: novium new [--template template] [--force] project-name
```

**Description:** Create new Novium project from template.

**Options:**
- `--template` - Template name (default, cuda, wasm, minimal)
- `--force` - Overwrite existing files

**Example:**
```bash
novium new my-app
novium new --template cuda gpu-project
```

---

### Command: `novium build`
```
Usage: novium build --target native|wasm|cuda|gpu [--optimize speed|size|balanced] [--incremental] [--cache]
```

**Description:** Build Novium project for specified target.

**Options:**
- `--target` - Compilation target (required)
- `--optimize` - Optimization level (default: auto from config)
- `--incremental` - Enable incremental builds
- `--cache` - Enable build caching

**Examples:**
```bash
novium build --target native
novium build --target cuda --optimize speed
novium build --target wasm --incremental
novium build --target gpu
```

---

### Command: `novium test`
```
Usage: novium test [--target native|wasm|cuda|gpu] [--verbose] [--coverage]
```

**Description:** Run tests for Novium project.

**Options:**
- `--target` - Test compilation target
- `--verbose` - Show detailed test output
- `--coverage` - Generate coverage report

**Example:**
```bash
novium test --target native --verbose
```

---

### Command: `novium fmt`
```
Usage: novium fmt [--check] [--files "pattern"]
```

**Description:** Format Novium source files.

**Options:**
- `--check` - Check format only, don't modify files
- `--files` - Specific file patterns to format

**Examples:**
```bash
novium fmt
novium fmt --check
novium fmt --files "src/**/*.nvm"
```

---

### Command: `novium install`
```
Usage: novium install [--force] [--offline]
```

**Description:** Install project dependencies.

**Options:**
- `--force` - Force reinstallation of all dependencies
- `--offline` - Use existing lockfile only (no network access)

**Example:**
```bash
novium install
novium install --offline
```

---
## Hardware Detection

### CPU Detection
The hardware detector automatically identifies:

**Cores and Threads:**
- Logical processor count
- Physical core count (if hyperthreading distinguishable)
- Affinity mapping

**Model and Frequency:**
- CPU manufacturer and model string
- Base and maximum frequency (MHz)
- Current frequency scaling

**SIMD Extensions:**
- **SSE:** SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2
- **AVX:** AVX, AVX2
- **AVX-512:** AVX-512F, AVX-512VL, AVX-512BW, AVX-512DQ
- **ARM:** NEON, SVE, SVE2
- **RISC-V:** V (vector extension)

**Architecture:**
- `x86_64` - 64-bit Intel/AMD
- `arm64` - 64-bit ARM (Apple Silicon, AWS Graviton, etc.)
- `riscv64` - 64-bit RISC-V

**Detection Method:**
- Node.js `os.cpus()` for basic info
- `/proc/cpuinfo` (Linux), `sysctl` (macOS), WMI (Windows) for detailed flags
- `cpuid` instruction for feature detection
- Native addon for maximum accuracy

### GPU Detection
Auto-detects graphics hardware for CUDA/gpu target configuration:

**NVIDIA GPUs:**
- Vendor: `nvidia`
- Model: GPU model string (e.g., "GeForce RTX 3080")
- VRAM: Video RAM in GB
- Compute Capability: `sm_70`, `sm_75`, `sm_80`, `sm_86`, `sm_89`, `sm_90`
- CUDA Cores: Number of CUDA cores
- Driver Version: Installed CUDA driver version
- NVML availability: NVIDIA Management Library

**AMD GPUs:**
- Vendor: `amd`
- Model: GPU model string
- VRAM: Video RAM
- OpenCL Version: Supported OpenCL version
- ROCm availability: AMD Radeon Open Compute

**Intel GPUs:**
- Vendor: `intel`
- Model: GPU model string
- VRAM: Video RAM
- Compute Capability: Intel-specific numbering
- OpenCL Version: Supported version

**Apple Silicon:**
- Vendor: `apple`
- Model: Chip model (e.g., "Apple M1", "Apple M2")
- VRAM: Unified memory architecture
- Metal Version: Metal API version support
- Maximum Metal feature set

**Detection Method:**
- `nvidia-smi` for NVIDIA GPUs (via subprocess)
- `rocm-smi` / `clinfo` for AMD GPUs
- `intel_gpu_top` / `clinfo` for Intel GPUs
- `system_profiler` for Apple Silicon
- `lspci` / `glxinfo` / `vulkaninfo` (cross-vendor)

### Memory Detection
**Parameters Detected:**
- Total system RAM (GB)
- Available RAM (GB) - currently free
- Memory speed (MT/s - MegaTransfers per second)
- Memory channel count (dual-channel, quad-channel, etc.)
- Memory type (DDR3, DDR4, DDR5, LPDDR, etc.)

**Usage in Build Optimization:**
- Available RAM < 4GB: Prefer "size" optimization profile
- Available RAM 4-8GB: Balanced profile recommended
- Available RAM 8GB+: "speed" profile optimal
- Available RAM 16GB+: Full speed optimizations enabled
- Available RAM 32GB+: Aggressive LTO and unrolling enabled

### Storage Detection
**Parameters Detected:**
- Storage type: NVMe, SSD, HDD, or unknown
- Free space (GB)
- Sequential IO speed (MB/s)
- Total capacity (GB)

**Usage:**
- Determines cache size for build optimization
- NVMe preferred for build cache (faster IO)
- Minimum 10GB free space recommended for builds

### OS Detection
**Parameters Detected:**
- Platform: `linux`, `darwin`, `win32`, `freebsd`, or `unknown`
- Architecture: `x64`, `arm64`, `riscv64`, or `unknown`
- Kernel version: `uname -r` equivalent
- Distribution: Ubuntu, Debian, CentOS, macOS version, Windows 10/11, etc.

**Example:**
```json
{
  "platform": "win32",
  "arch": "x64",
  "kernelVersion": "10.0.19045",
  "distribution": "Windows 11 Pro"
}
```

### Container/Virtualization Detection
**Detected Environments:**
- Docker: `.dockerenv` file present, cgroups identifiable
- Podman: Similar to Docker, podman-specific markers
- Kubernetes: KUBERNETES_SERVICE_HOST env var present
- VMware: hypervisor flag in CPUID, DMI type 1
- VirtualBox: VirtualBox-specific DMI data
- Hyper-V: Hyper-V hypervisor present
- Native: No virtualization detected

**Usage:**
- Adjusts build parallelism (reduce in containers)
- Memory available calculations (account for container limits)
- CPU core availability (respect CPU limits)
- File path handling (container mount points)

---
## Troubleshooting

### Common Issues

#### 1. Go CLI Won't Compile
**Symptom:** `go build` fails or `go` command not found
**Solution:** 
- Install Go from https://go.dev/dl/
- Ensure `go` is in PATH
- Or use pre-compiled binary from releases

#### 2. VS Code Extension Won't Load
**Symptom:** Extension disabled or not appearing in Command Palette
**Solution:**
- Run `npm install` in `novium-adaptive-extension/`
- Run `npm run compile` to compile TypeScript
- Reload VS Code window (Ctrl+Shift+P → "Developer: Reload Window")
- Check `novium-adaptive-extension\package.json` engines compatibility

#### 3. Error Corrector Not Fixing Issues
**Symptom:** Many "unfixable" issues reported
**Solution:**
- Check `novium.toml` error_correction configuration
- Increase `maxAutoFixAttempts` 
- Review specific issue categories - some require manual fixes
- Enable `enableSecurityScanning` and `enablePerformanceOptimization`

#### 4. CUDA Build Fails Without NVIDIA GPU
**Symptom:** `--target cuda` selected but no NVIDIA GPU detected
**Solution:**
- Use `--target native` instead for CPU compilation
- Or manually set target in `novium.toml`: `target = "native"`
- The auto-detection will default to native if no NVIDIA GPU found

#### 5. Dependency Version Conflicts
**Symptom:** `novium install` reports version conflicts
**Solution:**
- Run `novium install --force` to overwrite lockfile
- Use `novium install --offline` with existing lockfile
- Check `novium.toml` `[dependencies]` section for conflicting versions
- Run `novium diagnose` to identify specific conflicts

#### 6. Slow Build Performance
**Symptom:** Builds take longer than expected
**Solution:**
- Verify `parallelJobs` is set correctly (CPU cores - 1)
- Ensure build cache directory has sufficient space
- Check available RAM optimization profile is correct
- Consider `--incremental` flag for incremental builds
- Verify LTO settings match hardware capabilities

#### 7. WASM Build Errors
**Symptom:** `em++` compilation failures
**Solution:**
- Ensure Emscripten SDK installed: `emsdk install latest`
- Run `emsdk activate latest`
- Check `novium.toml` has correct WASM settings
- Verify `--target wasm` is selected
- Review `novium-adaptive-extension\src\auto-configurator.ts` em++ flags

#### 8. Diagnostic Engine Reports False Positives
**Symptom:** Many issues flagged that aren't real problems
**Solution:**
- Check `novium.toml` strictDependencyResolution setting
- Review specific issue categories in diagnostic output
- Some style checks can be configured via `novium.toml`
- Update diagnostic engine rules if false patterns persist

#### 9. Error Corrector Infinite Loop
**Symptom:** Compilation keeps failing with same errors after fixes
**Solution:**
- Check `maxAutoFixAttempts` configuration (default: 3)
- Some issues require manual intervention beyond auto-fix capability
- Review the "unfixable issues" list for patterns
- Consider the specific error type - some are fundamentally code-structural

#### 10. Cross-Platform Build Failures
**Symptom:** Build works on one OS but not another
**Solution:**
- Check OS-specific configuration in `novium.toml`
- Verify hardware detection matches actual platform
- Path separators may need adjustment (`/` vs `\`)
- CMake generator may need OS-specific specification
- SDK/toolchain paths differ by platform

### Getting Help

**Documentation:**
- `full-on learn guide.md` - Comprehensive ecosystem guide
- `novium-adaptive-extension\README.md` - VS Code extension docs
- `novium-adaptive-extension\error-correction\README.md` - Error correction docs
- Online: https://novium-lang.github.io/docs/

**Community:**
- GitHub Discussions: https://github.com/novium-lang/novium/discussions
- Issues: https://github.com/novium-lang/novium/issues
- Discord: https://discord.gg/novium-lang

**Support Commands:**
```bash
novium diagnose      # Run full diagnosis
novium --version     # Show version info
novium help          # Show CLI help
```

---
## Repository Structure

```
Novium Programming language/
├── Novium Compiler language(.nvm)/       # Systems compiler & native runtime
│   ├── src/                              # Compiler source code (C++)
│   ├── tests/                            # Unit & stress testing suites
│   ├── docs/                             # Detailed design documents
│   └── examples/                         # .nvm source files
│
├── Novium Interpreter language(.nvi)/    # Scripting VM (Future Phase)
│   ├── vm/                               # Bytecode virtual machine
│   ├── builtins/                         # Built-in functions
│   └── examples/                         # .nvi source files
│
├── Novium Web language(.nvw)/            # Web component transpiler
│   ├── src/                              # Transpiler source code
│   ├── components/                       # .nvw component examples
│   └── docs/                             # Web framework docs
│
├── novium-cli/                           # Go CLI (6 commands)
│   ├── main.go                           # Entry point
│   ├── go.mod                            # Go module definition
│   └── go.sum                            # Go dependencies
│
├── novium-adaptive-extension/            # VS Code Adaptive Extension
│   ├── src/                              # TypeScript source (7 files)
│   ├── configs/                          # Extension configurations
│   ├── syntaxes/                         # Language syntax definitions
│   ├── scripts/                          # Build/package scripts
│   ├── bin/                              # Compiled output
│   └── error-correction/                 # Auto-fix feature
│
├── novium_pkg_manager.py                 # Python package manager
├── full-on learn guide.md                # 100+ page learning guide
├── README.md                             # Top-level overview
└── error-correction/                     # Error correction module
```

---
## Quick Start Checklist

### Fresh Installation
```bash
# 1. Install prerequisites
# - Go: https://go.dev/dl/ (for CLI)
# - Node.js: https://nodejs.org/ (for VS Code extension)
# - Emscripten: https://emscripten.org/ (for WASM target)
# - CUDA toolkit: https://developer.nvidia.com/cuda-downloads (for CUDA target)

# 2. Initialize project
novium init

# 3. Open in VS Code
code .

# 4. Install adaptive extension (if not automatic)
# - Or open novium-adaptive-extension/ in VS Code
# - Run: npm install && npm run compile

# 5. Start developing
# - Write .nvm, .nvi, or .nvw files
# - Use Command Palette: Ctrl+Shift+P
# - "Novium: Initialize Adaptive Toolchain" (first run)
# - "Novium: Build for Target" to compile

# 6. Manage dependencies
novium install

# 7. Run tests
novium test

# 8. Fix issues automatically
# - "Novium: Diagnose & Fix Issues" in Command Palette
```

### First Program
```novium
// Save as main.nvm
fn main() void:
    print("Hello from Novium!")
```

**Build commands:**
```bash
# Native CPU build
novium build --target native

# WASM web build
novium build --target wasm

# CUDA GPU build (if NVIDIA GPU)
novium build --target cuda

# GPU compute (if AMD/Intel)
novium build --target gpu
```

### VS Code Workflow
1. **Initialize:** Command Palette → "Novium: Initialize Adaptive Toolchain"
2. **Optimize:** Command Palette → "Novium: Optimize for Current Hardware"
3. **Diagnose:** Command Palette → "Novium: Diagnose & Fix Issues" (auto-fixes bugs)
4. **Build:** Command Palette → "Novium: Build for Target" (select target)
5. **Iterate:** Modify code, auto-fix appears, rebuild

### Package Management Workflow
```bash
# Initialize package management
python novium_pkg_manager.py init

# Add dependency (manual edit of novium.toml, then)
novium install

# Lock versions for reproducibility
novium lock

# Update dependencies
novium update

# Offline build (using cached lockfile)
novium install --offline
```

---
*Documentation generated for Novium Language Ecosystem v1.0*
*Last updated: August 18, 2026*
*Generated by: Novium Adaptive Toolchain*
# Novium Compiled (`.nvm`) — Systems Programming Language

> **Current capability:** this repository builds a lexer/parser frontend that prints tokens or an AST. Native code generation, execution, type checking, ownership validation, FFI, WebAssembly output, and API generation are not implemented yet; descriptions of them in this document are the intended language design.

## Overview

**.nvm** is Novium's compiled systems language. It compiles to **native machine code** via LLVM, delivering Rust/C++-level performance with safety guarantees. This is the layer for building CLIs, servers, libraries, and any performance-critical software.

## Design Philosophy

- **Ownership + Borrow Checker**: Memory is managed deterministically without a garbage collector. Values own their memory; borrows are checked at compile time.
- **Static + Inferred Types**: Types are checked at compile time, but the compiler figures out most types automatically (e.g., `let x = 5` instead of `let x: int = 5`).
- **Zero-cost Abstractions**: What you don't use, you don't pay for. Inlined functions, unboxed primitives, stack-allocated containers.
- **Predictable Performance**: Bounded recursion depth (256 frames); no hidden control flow; errors are values, not exceptions that jump around.

## Syntax Quick Reference

Novium uses a **Python-Go hybrid** syntax. Blocks can use colons + indentation (like Python) or curly braces (like C). This hybrid design was chosen for readability and tooling friendliness.

### Function Definitions

```novium
// Colons + indentation (block style)
fn add(a: int, b: int) int:
    return a + b

// Curly braces inline (single line)
fn double(x: int) int { return x * 2 }

// Function with default parameter value
#[default("World")]
fn greet(name: string) string:
    return "Hello, ${name}!"
```

### Type Annotations

- **Explicit**: `let x: int = 5`
- **Inferred**: `let x = 5` (compiler determines `int`)
- **Optional**: `let x: optional int = none` (no null pointers!)

### Common Types

| Novium Type | Description |
|-------------|-------------|
| `int` | Arbitrary-precision integer (unboxed when possible) |
| `float` | 64-bit IEEE 754 floating point |
| `string` | Length-prefixed, UTF-8, immutable |
| `bool` | `true` or `false` |
| `optional T` | Either `some Value` or `none` — no `null` |
| `string?` | Alias for `optional string` (syntactic sugar) |

### Pattern Matching

```novium
fn describes(n: int) string:
    match n:
        0 => "zero"
        1 => "one"
        _ => "number: ${n}"  // _ is catch-all, REQUIRED (exhaustiveness)
```

### Error Handling

Errors are **values**, not hidden control flow:

```novium
fn safe_divide(a: float, b: float) float:
    if b == 0.0:
        return 0.0  // Return error value; caller MUST check
    return a / b
```

## Memory Management

### Ownership Rules

1. **Every value has an owner**: When a value is assigned to another variable, it's moved (not copied), unless the type implements copy semantics (primitives do).
2. **Borrow checker**: `let ref = &value` creates a borrow; the borrow checker ensures the borrow doesn't outlive the owner.
3. **`own` keyword**: `fn process(own data: string)` transfers ownership — the caller cannot use `data` after this call.

### No Garbage Collector

- Deterministic destruction (RAII-style)
- No "stop-the-world" pauses
- Memory freed when scope exits or ownership transfers

## Building & Running

### Prerequisites

- **CMake** (3.16+)
- **C++17** compiler (GCC, Clang, or MSVC)
- **LLVM** (for native code generation; included with the compiler)

### Quick Start (Windows + WSL)

The easiest path is using **WSL (Windows Subsystem for Linux)** with Ubuntu:

```bash
# In WSL terminal:
sudo apt update
sudo apt install build-essential cmake git

# Clone/pull this repo
git clone <repo-url>
cd Novium-Programming-Language/Novium\ Compiler\ language\(.nvm\)

# Build
mkdir -p build && cd build
cmake -DNOVIUM_OUTPUT=native ..
make

# Run
./novium examples/hello.nvm
```

### Building from Source (Detailed)

```powershell
# 1. Navigate to compiler directory
cd "Novium Compiler language(.nvm)"

# 2. Configure (detects your C++ compiler)
cmake -B build -S .

# 3. Build
cmake --build build

# 4. Run a sample program
.\build\novium.exe examples/hello.nvm
```

### Compilation Targets

| Target | Flag | Output |
|--------|------|--------|
| Native code | (default) | `novium.exe` or `novium` binary |
| WebAssembly | `cmake -DNOVIUM_OUTPUT=wasm ..` | `novium.wasm` + JavaScript glue |
| Library only | `cmake -DBUILD_LIB=ON ..` | `libnovium_lib.a` (static) |

## Interoperability

### Calling C Libraries

```novium
// Via #[link] annotation
#[link("m")]
extern "C" {
    fn cos(angle: float) float;
    fn sin(angle: float) float;
}

// Or custom C library
#[link("mylib")]
extern "C" {
    fn app_init() int;
    fn app_shutdown() void;
}
```

### Importing from .nvi (Interpreter)

`.nvi` scripts can `import "file.nvm"` to use compiled Novium modules:

```novium
// In .nvi:
import "hello.nvm"

// Call exported function
greet("World")
```

### API Endpoints (FastAPI-like)

You can define API endpoints that generate OpenAPI 3.1 specs and client stubs:

```novium
#[get("/fibonacci/{n}")]
fn fib_api_get(n: int) int:
    #response(200, {"fibonacci": int})
    #response(400, {"detail": "n must be non-negative"})
    if n < 0:
        return -1  // Error sentinel
    let result: int = fibonacci(n)
    return result
```

Generated clients (auto-documented):
- **Python**: `from novium.client import NoviumClient; client = NoviumClient("http://localhost:8000"); client.fib_get(10)`
- **JavaScript**: `import novium from './fibonacci.wasm'; await novium.fibGet(10)`
- **Go**: `client := novium.NewClient("http://localhost:8000"); client.FibGet(context.Background(), 10)`

## Examples

### hello.nvm

```novium
// ============================================================================
// hello.nvm — Hello World in Novium Compiled
// ============================================================================
// Memory-safe, type-checked, bounds-checked.

// Simple function
fn greet(name: string) void:
    print("Hello, ${name}! Welcome to Novium.")

# Function with return value
fn add(a: int, b: int) int:
    return a + b

# Inline function
fn double(x: int) int: return x * 2

# Main entry point
fn main() void:
    greet("World")
    let result: int = add(10, 20)
    let doubled: int = double(result)
    print("10 + 20 = ${result}, doubled = ${doubled}")
```

### fibonacci.nvm

```novium
// ============================================================================
// fibonacci.nvm — Novium Compiled (Performance & API)
// ============================================================================
#![deny(non_exhaustive_match)]

// Memory layout optimizations
// - int/float unboxed, never heap-allocated
// - optional T as tagged immediate, NO pointer
// - string length-prefixed, O(1) len()

// C FFI: Math library
#[link("m")]
extern "C" {
    fn cos(angle: float) float;
    fn sin(angle: float) float;
}

// Recursive fibonacci (bounded depth: 256 frames)
fn fibonacci(n: int) int:
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

// Iterative fibonacci: O(n) time, O(1) space
fn fib_iter(n: int) int:
    if n <= 1:
        return n
    let a: int = 0
    let b: int = 1
    let i: int = 0
    while i < n:
        let temp: int = a + b
        a = b
        b = temp
        i = i + 1
    return a

// API endpoints (FastAPI-like)
#[get("/fibonacci/{n}")]
fn fib_api_get(n: int) int:
    #response(200, {"fibonacci": int})
    if n < 0:
        return -1
    let result: int = fibonacci(n)
    return result

#[post("/fibonacci/batch")]
fn fib_api_post_batch(numbers: list[int]) list[int]:
    #schema({ "numbers": { "type": "array", "items": { "type": "integer" }, "minItems": 1, "maxItems": 1000 } })
    let results: list[int] = []
    for n in numbers:
        results.append(fibonacci(n))
    return results

#[get("/async/{n}")]
async fn fib_api_async(n: int) string:
    #response(202, {"status": "processing"})
    let result: int = await heavy_work(n)
    return f" fibonacci({n}) = {result}"

#[get("/health")]
fn health_get() string:
    #summary("Health check")
    return "OK"

#[get("/")]
fn root_get() string:
    #include_in_schema(True)
    return """
    # Novium API Root
    - `GET /fibonacci/{n}` - Compute Fibonacci number
    - `POST /fibonacci/batch` - Batch computation
    - `GET /async/{n}` - Async computation
    - `GET /health` - Health check
    """

// Standard functions
fn fibonacci(n: int) int: ...
fn fib_iter(n: int) int: ...
fn safe_divide(a: float, b: float) float: ...
async fn heavy_work(n: int) int: ...

fn main() void:
    // 1. Recursive fibonacci
    let fib10: int = fibonacci(10)
    print("fibonacci(10) = ${fib10}")

    // 2. Iterative fibonacci
    let fi10: int = fib_iter(10)
    print("fib_iter(10) = ${fi10}")

    // 3. C FFI: math functions
    let angle: float = 3.14159 / 2.0
    let c_result: float = cos(angle)
    print("cos(pi/2) = ${c_result}")

    // 4. API demonstrations (compile-time definitions)
    // These would generate client code for Python/JS/Go/Rust
    let health: string = health_get()
    print("Health: ${health}")

    // 5. Shutdown
    // app_shutdown()  // if linked
```

## Tooling

### `novium` Commands

| Command | Description |
|---------|-------------|
| `novium file.nvm` | Compile and run (default: AST print) |
| `novium --tokens file.nvm` | Print lexer tokens only |
| `novium --ast file.nvm` | Print parsed Abstract Syntax Tree |
| `novium --type-check file.nvm` | Full type verification |
| `novium lint file.nvm` | Enforce style + potential bugs |
| `novium test file.nvm` | Run associated tests |

### IDE/Editor Support

- **VS Code**: Extension recommended (syntax highlighting, goto-def, type diagnostics)
- **Neovim**: `nvim-novium` plugin (coming soon)
- **CTags**: Works with the AST output for symbol indexing

### Package Management (Future)

```
novium pkg install <name>
novium pkg list
novium pkg search "web framework"
```

## Migrating from Other Languages

### From Python

```python
# Python
def add(a, b):
    return a + b

# Novium (equivalent, with type hints)
fn add(a: int, b: int) int:
    return a + b
```

### From C++

```cpp
// C++
int add(int a, int b) { return a + b; }

// Novium (safer, with ownership)
fn add(a: int, b: int) int:
    return a + b  // No memory leaks; no undefined behavior
```

### From Rust

```rust
// Rust
fn add(a: i32, b: i32) -> i32 {
    a + b
}

// Novium (similar semantics, Python-Go syntax)
fn add(a: int, b: int) int:
    return a + b
```

## Philosophy & Roadmap

### Current Stage: Sprint 2 (AST & Parser)
- Lexer: ✅ Complete (Sprint 1)
- Parser: 🟦 In progress
- Type Checker: 🟦 Planned
- Code Generation (LLVM): 🟨 Planned
- Interpreter (.nvi): 🟦 Planned (future phase)
- Web Framework (.nvw): 🟨 Planned (future phase)

### Long-term Goals

1. **Full ownership + borrow checker** like Rust
2. **Garbage-collected mode** for .nvi scripts
3. **Web framework** like Next.js but in Novium (.nvw)
4. **Package manager** and ecosystem
5. **Cross-language FFI** stabilisation
6. **IDE support** (VS Code, Neovim)
7. **Performance optimisations** (auto-vectorization, memoisation)

## Getting Help

- **Issues**: Report bugs/feature requests at the repository
- **Discussions**: Join the community channel
- **Examples**: Study the `examples/` directory for patterns
- **Design Docs**: Check `docs/` for detailed architecture decisions

---

*Novium: Systems performance + scripting ergonomics + web UI, all in one language ecosystem.*

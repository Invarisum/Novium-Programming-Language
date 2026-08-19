# Novium SDK Documentation

## Overview

The Novium SDK provides libraries, headers, and tooling for C/C++ interop, Moji cross-language FFI, and project scaffolding. This document covers the SDK distribution, FFI guides, and migration utilities.

## SDK Distribution

### Generated Package

Running `novium --sdk generate` produces the following artifacts:

```
novium-sdk-v0.2.0/
├── include/
│   └── novium.h     # C ABI type mappings and function declarations
├── lib/
│   └── libnovium.a  # Static runtime library
└── novium-sdk.cmake # CMake integration file
```

### C ABI Header (`include/novium.h`)

The generated header contains:

- **Type mappings**: `NOVIUM_C_TYPE(int)`, `NOVIUM_C_TYPE(float)`, etc.
- **Version info**: `NOVIUM_SDK_VERSION`
- **Build configuration**: `NOVIUM_SDK_PROFILE`, `NOVIUM_SDK_TARGET`
- **C ABI functions**: `novium_malloc`, `novium_free`, `novium_print`, `novium_add`
- **Convenience types**: `NoviumString`, `NoviumSlice`, `NOVIUM_INT`

### CMake Integration (`novium-sdk.cmake`)

```cmake
find_package(novium REQUIRED)
include_directories(${NOVIUM_INCLUDE_DIRS})
target_link_libraries(my_app PRIVATE ${NOVIUM_LIBRARY})
```

### Usage Example (C)

```c
#include <novium.h>

void example() {
    int* x = (int*)novium_malloc(sizeof(int));
    *x = novium_add(3, 4);
    novium_print("Result: ");
    novium_free(x);
}
```

## Moji Cross-Language FFI

### Importing Moji Functions

```novium
import moji

extern "Moji" fn fibonacci(n int) int

fn main() {
    let result = fibonacci(10)
    print(result)
}
```

### Exporting Novium Functions

```novium
moji_export "my_fibonacci" fibonacci_fn sig

struct MojiFnSig {
    void* ret_type
    size_t arg_count
    void** arg_types
    bool is_cdecl
}
```

### Shared FFI Arena

The shared arena enables zero-copy data exchange between Novium and Moji:

```novium
moji_bridge_init()  // Called at runtime startup

// Allocate in shared arena
let data = moji_arena_alloc(&arena, 1024)
```

### Type Mappings (Moji → Novium)

| Moji Type | Novium Type Descriptor |
|-----------|----------------------|
| `i32` / `int` | `1` (int32) |
| `i64` / `long` | `2` (int64) |
| `f32` / `float` | `3` (float) |
| `f64` / `double` | `4` (double) |
| `bool` | `5` (bool) |
| `void` | `6` (void) |

## Project Scaffold (`novium new`)

### Creating a New Project

```bash
novium --sdk new my_game
```

This creates the following directory structure:

```
my_game/
├── src/           # Source directory for .nvm files
├── include/       # Header directory for C FFI
├── novium.nvm     # Main Novium source file
├── CMakeLists.txt # CMake build configuration
└── examples/      # Example programs directory
```

### Scaffold Template (`novium.nvm`)

```novium
# Hello World in Novium
fn main() -> int:
    print("Hello, Novium!")
    return 0
```

### CMakeLists.txt Template

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_game LANGUAGES CXX)

# Build Novium compiler
add_subdirectory(novium)

# Add Novium-compiled library
target_link_libraries(my_game PRIVATE novium_lib)

# Add source files
aux_source_directory(src DIR_SRCS)

# Executable
add_executable(my_game ${DIR_SRCS})
```

## Cross-Language Testing (`novium test --cross`)

### Running Cross-Language Tests

```bash
novium --test cross
```

This runs the following test suites:

1. **Novium Unit Tests**: Core language functionality
2. **Moji FFI Interop**: Cross-language function calls
3. **C ABI Compatibility**: C header interop verification
4. **Python FFI**: Python module import and function calling
5. **React JSX**: JSX compilation and React element creation

### Test Categories

| Test Type | Description |
|-----------|-------------|
| `unit` | Novium language semantics, type checker, interpreter |
| `moji` | Moji FFI import/export, arena allocation |
| `c_abi` | C header generation, `novium_add`, memory management |
| `cross` | Novium ⇄ Moji ⇄ C combined interop |
| `python` | Python FFI integration, module import, function calls |
| `react` | React JSX compilation, React element creation |

### Example Test File (`examples/python_ffi.nvi`)

```novium
# Python FFI test
python_import("math")
let result = python_call("pi")
print("Python pi: " + result)

# Python call with arguments
let area = python_call("pow", 5, 2)
print("5^2 via Python: " + area)

# React JSX test
let jsx = <div>Hello React</div>
let compiled = jsx_compile(jsx)
print("Compiled JSX: " + compiled)

# React element creation
let element = react_create("div", {"class": "hello"})
print("React element: " + element)

# React rendering
let render_result = react_render("App", {"title": "Demo"})
print("React render: " + render_result)
```

### Example Test File (`examples/react_demo.nvi`)

```novium
# React component demo
let App = <fn() -> div>
    <h1>Novium Full-Stack</h1>
    <p>Powered by .nvi interpreter</p>
</fn>

let rendered = react_render("App", {})
print("App rendered: " + rendered)

# Component with props
let Greet = <fn(name: string) -> div>
    <h2>Hello, {name}!</h2>
</fn>

let greeter = react_create("Greet", {"name": "World"})
print("Greeting element: " + greeter)

# Python + React integration
python_import("datetime")
let now = python_call("datetime.now")
let props = {"timestamp": now, "message": "Hello from Python+React"}
let integrated = react_render("App", props)
print("Python+React integrated: " + integrated)
```

## Migration Tool (`novium migrate`)

### Migration Directions

| Direction | Command | Description |
|-----------|---------|-------------|
| Novium → C | `novium migrate --to c <file.nvm>` | Generate C equivalent |
| Novium → Moji | `novium migrate --to moji <file.nvm>` | Generate Moji equivalent |
| C → Novium | `novium migrate --to novium <file.c>` | Import C code |

### Example: Novium → C

```bash
novium migrate --to c fibonacci.nvm
# Generates: fibonacci.c with C ABI compatible code
```

### Example: C → Novium

```bash
novium migrate --to novium math.c
# Parses C code and produces Novium .nvm equivalent
```

## Runtime Memory Management

The Novium runtime provides three levels of memory management, each tailored to
the respective language variant's scope and goals:

### .nvm: Highest-Grade Memory Management (Systems Compiler)
The .nvm backend compiler features the most sophisticated memory management in
the Novium ecosystem, designed for systems-level programming where control and
predictability are paramount.

**.nvm Memory Management Features:**

- **Ownership Types**: `MemoryOwnership::NONE`, `OWN`, `BORROW`, `BORROW_MUT`
  - Full compile-time lifetime analysis possible
  - Enables zero-cost abstractions (no runtime overhead when ownership is known)
  - Supports RAII patterns and automatic deallocation

- **Size-Class Memory Pools**
  - Pre-allocated pools for objects 8-1024 bytes
  - Zero-allocation overhead for common small types
  - Automatic pool return on deallocation for reuse

- **Generation-Based Reclamation**
  - Automatic memory cleanup when generation IDs expire
  - Ownership-aware: only OWN/NONE types are reclaimed;
    BORROW/BORROW_MUT respect reference counting
  - `sweep_generation()`, `force_reclaim()` methods

- **Memory Ownality Tracking**
  - Every Value and MemoryBlock tracks its ownership type
  - `MemoryManager::get_ownership()`, `set_ownership()` queries and sets ownership
  - `transfer_ownership()` enables safe ownership migration

- **Memory Statistics & Profiling**
  - `MemoryManager::get_memory_stats()` reports:
    - total_allocated, currently_alive, peak_allocated
    - num_blocks, num_pool_allocations, pool_efficiency (0.0-1.0)
  - `force_reclaim()` for emergency memory cleanup

- **Compile-Time Memory Analysis**
  - The .nvm compiler can prove memory lifetimes
  - Eliminates runtime memory checks when ownership is statically known
  - Supports safe manual memory management when needed

**.nvm Code Example - Ownership-Aware:**

```novium
# .nvm: Ownership-aware code
fn process_buffer(data: own uint8_t*, len: int) -> own uint8_t*:
    // .nvm: Compiler proves data ownership is transferred
    // .nvm: Deallocated when function scope ends
    let result: own uint8_t* = malloc(len)
    // ... process ...
    return result  // Ownership transferred to caller

fn main() -> void:
    let buffer: own uint8_t* = malloc(1024)
    let processed: uint8_t* = process_buffer(buffer, 1024)
    // .nvm: Compiler tracks ownership - buffer is freed here
    #       - processed ownership transfers to caller
```
```

### .nvi: Full-Stack Interpreter Memory Management
The .nvi interpreter provides automatic memory management suitable for a
full-stack interpreted language.

**.nvi Memory Management Features:**

- **Generation-Based Reclamation**
  - Automatic cleanup of values no longer referenced
  - `generation_id` tracking on all Values
  - `sweep_generation()` for batch cleanup

- **Reference Counting**
  - `MemoryBlock::ref_count` for shared memory
  - `add_ref()`, `release()` for manual control when needed

- **Arena Allocation (for temporary data)**
  - `Interpreter::allocate_memory()` / `deallocate_memory()`
  - Fast allocation with generation-based reclamation
  - Ideal for temporary/short-lived data

- **No Fixed Ownership Types (Trio-Compatible)**
  - All values use `MemoryOwnership::NONE` by default for automatic GC
  - **BUT**: Can switch to .nvm-compatible ownership modes per-value
  - `.nvi` ownership flexibility: values can be created with `OWN`, `BORROW`, 
    `BORROW_MUT` modes when .nvm-style memory control is needed
  - Full compatibility with .nvm ownership types when required
  - Garbage collector handles overall lifetime by default

- **Generation-Based Reclamation (Enhanced)**
  - `generation_id` tracking on all Values (from .nvm)
  - `sweep_generation()` for batch cleanup (from .nvm)
  - Can operate in .nvm mode (ownership-aware) or .nvi mode (auto GC)

- **Arena Allocation (for temporary data)**
  - `Interpreter::allocate_memory()` / `deallocate_memory()`
  - Fast allocation with generation-based reclamation
  - Ideal for temporary/short-lived data
  - Default: `MemoryOwnership::NONE` for automatic GC
  - Optional: `MemoryOwnership::OWN` for tracked lifetime

- **Ownership Flexibility (Key Trio Feature)**
  - Per-value ownership mode switching
  - `.nvi` can emulate .nvm ownership when needed:
    ```novium
    # .nvi: Default automatic GC
    let auto_val = MemoryManager::allocate(1024)  # NONE ownership
    
    # .nvi: .nvm-style ownership when needed
    let owned_val = MemoryManager::allocate(1024, 1, MemoryOwnership::OWN)
    # .nvi: Ownership tracked, auto-reclaimed when refcount=0
    
    # .nvi: Borrowed mode for shared data
    let borrowed_val = MemoryManager::allocate(1024, 1, MemoryOwnership::BORROW)
    ```
  - Full backward compatibility: code using `NONE` works unchanged
  - Optional .nvm-style ownership when control is needed

**.nvi Code Example - Ownership-Enabled Trio Compatibility:**

```novium
# .nvi: Ownership-enabled automatic memory
# .nvi: Can use .nvm-style ownership when needed, or automatic GC

fn with_ownership(data: uint8_t*, len: int) -> uint8_t*:
    # .nvi: Use .nvm-style OWN ownership for tracked lifetime
    let result: uint8_t* = MemoryManager::allocate(len, 1, MemoryOwnership::OWN)
    # .nvi: Ownership tracked, will be freed when refcount reaches 0
    # .nvi: Or omit ownership for automatic GC:
    # let result2 = MemoryManager::allocate(len, 1, MemoryOwnership::NONE)
    # ... process ...
    return result  # Auto-reclaimed when done, or tracked ownership

fn auto_reclaimed() -> string:
    # .nvi: Default automatic GC (NONE ownership)
    let result: string = "expensive computation"
    return result  # Auto-reclaimed when no longer referenced

fn main() -> void:
    # .nvi: Mixed ownership modes - same code, different behavior
    let owned_buf = MemoryManager::allocate(1024, 1, MemoryOwnership::OWN)
    let auto_buf = auto_reclaimed()
    print(owned_buf)
    # .nvi: owned_buf tracked, auto_buf auto-reclaimed
    # .nvi: Same API, different ownership modes - trio compatible
```
```
```

### .nvw: Frontend-Focused Memory Management (Trio-Compatibility Layer)
The .nvw web frontend compiler relies on the JavaScript Wasm runtime's garbage
collector, but adapts .nvm concepts for **full-stack trio compatibility**.

**.nvw Memory Management Features (Trio-Compatibility):**

- **JavaScript GC Integration (Base)**
  - All Novium values compile to Wasm/JS
  - JavaScript garbage collector handles lifetime
  - No explicit deallocation needed

- **DataPacket Generation ID (Trio Bridge)**
  - **Consistent generation_id tracking across .nvm ↔ .nvi ↔ .nvw**
  - Enables cross-variant memory lifetime awareness
  - `DataPacket.generation_id` flows between all three variants
  - No ownership complexity - JS GC manages, but generation_id provides tracking

- **Conceptual .nvm Ownership Adaptation**
  - .nvw cannot use OWN/BORROW/BORROW_MUT directly (JS GC incompatible)
  - But **conceptually adapts** .nvm ownership ideas:
    - Generation ID provides lifetime tracking equivalent
    - DataPacket is_shared flag mirrors ownership concepts
    - TypeMapper serialization format enables type compatibility

- **Wasm Memory Model**
  - Linear memory within Wasm module bounds
  - No ownership types needed (JS GC manages)
  - Predictable memory bounds (Wasm linear memory size)

- **Compile-Time Size-Class Optimizations (Trio Bridge)**
  - .nvw compiler can apply .nvm-style size-class thinking
  - Small objects inlined during Wasm/JS codegen
  - Zero-overhead for common patterns (conceptual adaptation)

- **Shared Memory via DataPacket (Trio-Compatible)**
  - Cross-variant data transfer uses DataPacket with generation_id
  - Enables .nvm ↔ .nvi ↔ .nvw memory lifetime awareness
  - No ownership complexity for web use cases

**.nvw Code Example - Trio-Compatibility:**

```novium
# .nvw: Frontend memory with trio compatibility
# .nvw: JS GC handles lifetime, but DataPacket generation_id enables interop

fn render_greeting(name: string, shared_with: string = "none") -> div:
    # .nvw: Values compile to JS, GC handles lifetime
    # .nvw: DataPacket generation_id tracks cross-variant compatibility
    let dp_data = {"msg": "Hello, " + name}
    let dp = DataPacket{"novium/string", json_encode(dp_data), generation_id: 3}
    
    # .nvw: generation_id enables .nvm/.nvi awareness without ownership overhead
    <div>Hello, {name}!</div>
    # .nvw: generation_id=3 means this data could be shared with .nvi/.nvm
    # .nvw: if needed via DataPacket interop, but JS GC still manages lifetime

fn main() -> void:
    # .nvw: No manual memory management
    # .nvw: JS GC reclaims when component unmounts
    # .nvw: generation_id tracked for trio interop if needed
    let component = render_greeting("World", "nvm")
    # ... component auto-unmounts
    # .nvw: generation_id=3 remains tracked for trio compatibility
```
```
```
```

### Summary: .nvi vs .nvw Python/React

| Feature | .nvi (Full-Stack) | .nvw (Frontend) |
|---------|-------------------|------------------|
| Python FFI | Full runtime support | Compiles to JS, no runtime |
| React JSX | Full bridge with runtime | Compiles to virtual DOM |
| Memory Management | Generation + ref counting | JS GC only |
| API Access | Python → C/FFI/Bridge | Python → Browser JS APIs |
| Standard Library | Full Python subset | Limited frontend subset |
| Use Case | Full-stack applications | Web frontend / Wasm compilation |

### Example Test Files for Memory Management

The `novium test --cross` now includes memory management test categories:

| Test Type | Description |
|-----------|-------------|
| `memory` | .nvm ownership and pool tests |
| `gc` | .nvi generation-based GC tests |
| `nvw_memory` | .nvw JS GC integration tests |

### Example Test File (`examples/nvm_memory_ownership.nvm`)

```novium
# .nvm Memory ownership test
# Verifies compile-time ownership tracking

fn take_ownership(data: own uint8_t*, len: int) -> own uint8_t*:
    // Compiler tracks: data ownership transferred here
    // Function returns ownership to caller
    return data  // Ownership passed through

fn release_ownership(data: own uint8_t*) -> void:
    // .nvm: Compiler knows data can be freed here
    // (Or passed to another function with ownership transfer)
    // No runtime cost - ownership info is compile-time

fn main() -> void:
    let buffer: own uint8_t* = malloc(256)
    let processed: own uint8_t* = take_ownership(buffer, 256)
    # .nvm: Compiler ensures:
    #   - buffer is NOT accessed after transfer
    #   - processed ownership is tracked
    #   - No runtime overhead for ownership tracking
```
```

## Python & React Compatibility (`novium python` and `novium react`)

### .nvi Full-Stack Interpreter Python/React (Previously Covered)

The .nvi interpreter provides full Python FFI and React JSX bridge with runtime support. See the .nvi section in the SDK docs.

### .nvw Frontend Web Compiler Python/React

The .nvw frontend compiler provides **frontend-focused** Python FFI and React JSX compatibility. Unlike .nvi, .nvw does not include backend runtime features (no memory management, no garbage collection). Instead, Python code compiles to JavaScript, and React JSX compiles to virtual DOM descriptions for Wasm/JS output.

#### .nvw Python FFI (Frontend-Oriented)

The .nvw compiler can import Python modules that transpile to JavaScript equivalents. This enables:

- **Browser API access** - `window`, `document`, `fetch`, `localStorage` via Python-style calls
- **Math operations** - `math.pi`, `math.pow`, `random.randint` compile to JS equivalents
- **JSON handling** - `json.dumps`, `json.loads` for data serialization

**.nvw Python FFI Example:**

```novium
# python_bridge.nvw
python_import("browser")
let width = python_call("window.innerWidth", 0)
print("Browser width: " + width)

python_import("math")
let area = python_call("math.pow", 5, 2)
print("5^2 via Python: " + area)

python_import("json")
let data = python_call("json.dumps", {"status": "active"})
```

#### .nvw Python FFI Limitations (Expected for Frontend)

- No Python runtime or memory management
- Python functions compile to JavaScript equivalents
- Only a subset of Python standard library is supported
- No Python garbage collection - JS garbage collector handles memory

#### .nvw Python + React Integration Example:

```novium
python_import("json")
let json_data = python_call("json.dumps", {"count": 42})
let react_props = {"data": json_data}
let rendered = react_render("App", react_props)
print("Python+React: " + rendered)
```

#### .nvw Python FFI Built-ins

| Built-in | Description |
|----------|-------------|
| `python_import("module")` | Import a Python module (compiles to JS) |
| `python_call("function", args...)` | Call a Python function (compiles to JS call) |
| `python_module("module")` | Get a Python module object (frontend only) |

### .nvw React JSX Bridge

The .nvw compiler includes a React JSX bridge that enables JSX syntax for component definitions. JSX is natively supported since .nvw is a web/frontend language.

**.nvw JSX Example:**

```novium
# JSX in .nvw
let basic = <div>Hello, Novium!</div>
let compiled = jsx_compile(basic)
print("JSX compiled: " + compiled)

let with_props = <div class="greeting" id="title">Content</div>
let compiled_props = jsx_compile(with_props)
print("JSX with props: " + compiled_props)

# React element creation
let element = react_create("div", {"class": "container"})
print("React element: " + element)

# React rendering
let render_result = react_render("App", {"title": "Demo"})
print("React render: " + render_result)

# Component composition
let App = <fn() -> div>
    <h1>App Title</h1>
    <p>Novium .nvw React integration</p>
</fn>

let app_element = react_create("App", {})
let app_render = react_render("App", {})
print("App rendered: " + app_render)
```

**.nvw JSX Built-ins**

| Built-in | Description |
|----------|-------------|
| `jsx_compile(source)` | Compile JSX source to Novium representation |
| `react_create("component", props)` | Create a React element |
| `react_render("component", props)` | Render a React component |

**.nvw JSX + Python Integration Example:**

```novium
python_import("math")
let pi_val = python_call("pi")

let MathDisplay = <fn() -> div>
    <p>π = {pi_val}</p>
</fn>

let math_element = react_create("MathDisplay", {})
let math_render = react_render("App", {"showMath": True})
print("Math display with Python data: " + math_render)
```

#### .nvw React JSX Limitations (Frontend-Focused)

- JSX compiles to virtual DOM descriptions, not real DOM rendering
- No React runtime or reconciliation algorithm (handled by Wasm/JS output)
- No hooks (`useState`, `useEffect`) - basic component system only
- Component props are static (no reactivity system in .nvw itself)
- CSS-in-JS is static styling (no runtime processing)

#### .nvw CSS-in-JS Example:

```novium
# CSS-in-JS styles (static, compiled to class names)
css: "body { font-family: sans-serif; margin: 0; padding: 20px; }"
css: ".button { padding: 10px; background: #0066cc; color: white; }"
css: ".container { max-width: 800px; margin: 0 auto; }"

# Component using CSS classes
let Button = <fn(text: string) -> div>
    <button class="button">{text}</button>
end

let save_btn = react_create("Button", {"text": "Save"})
print("Styled button: " + save_btn)
```

### Summary: .nvi vs .nvw Python/React

| Feature | .nvi (Full-Stack) | .nvw (Frontend) |
|---------|-------------------|------------------|
| Python FFI | Full runtime support | Compiles to JS, no runtime |
| React JSX | Full bridge with runtime | Compiles to virtual DOM |
| Memory Management | Garbage collected runtime | JS garbage collector only |
| API Access | Python → C/FFI/Bridge | Python → Browser JS APIs |
| Standard Library | Full Python subset | Limited frontend subset |
| Use Case | Full-stack applications | Web frontend / Wasm compilation |

### Example Test Files for .nvw Python/React

The `novium test --cross` now includes .nvw Python/React test categories:

| Test Type | Description |
|-----------|-------------|
| `python` | .nvi Python FFI integration |
| `react` | .nvi React JSX bridge |
| `nvw_python` | .nvw Python FFI (frontend) |
| `nvw_react` | .nvw React JSX (frontend) |

### Example Test File (`examples/nvw_python_react.nvw`)

```novium
# .nvw Python FFI + React test
python_import("browser")
let width = python_call("window.innerWidth", 0)
print("Browser width via Python: " + width)

python_import("math")
let area = python_call("math.pow", 10, 2)
print("10^2 via Python: " + area)

# JSX and React
let jsx = <div>Hello from .nvw!</div>
let compiled = jsx_compile(jsx)
print("JSX compiled: " + compiled)

let element = react_create("div", {"class": "hello"})
print("React element: " + element)

let rendered = react_render("App", {"title": ".nvw React"})
print("React render: " + rendered)
```

## Python & React Compatibility (`novium python` and `novium react`)

### Python FFI Integration

The .nvi full-stack interpreter provides seamless Python FFI integration, allowing Novium code to import and call Python modules:

```bash
# Enable Python support
novium --runtime python

# Import and use Python modules from .nvi
novium run python_ffi.nvi
```

#### Python FFI Built-ins

| Built-in | Description |
|----------|-------------|
| `python_import("module")` | Import a Python module |
| `python_call("function", args...)` | Call a Python function with arguments |
| `python_module("module")` | Get a Python module object |

#### Example: Python FFI Usage

```novium
# python_ffi.nvi
python_import("math")
let pi_val = python_call("pi")
print("Python pi: " + pi_val)

let area = python_call("pow", 5, 2)
print("5^2 via Python: " + area)

python_import("datetime")
let now = python_call("datetime.now")
print("Current time: " + now)
```

#### Example: Python + React Integration

```novium
python_import("json")
let data = python_call("json.dumps", {"status": "active"})
let rendered = react_render("App", {"json_data": data})
print("Python+React: " + rendered)
```

### React JSX Bridge

The .nvi interpreter includes a React JSX bridge, enabling JSX syntax compilation and React element creation:

```bash
# Enable React support  
novium --runtime react

# Use JSX and React built-ins from .nvi
novium run react_demo.nvi
```

#### React JSX Built-ins

| Built-in | Description |
|----------|-------------|
| `jsx_compile(source)` | Compile JSX source to Novium representation |
| `react_create("component", props)` | Create a React element |
| `react_render("component", props)` | Render a React component |

#### Example: React JSX Usage

```novium
# react_demo.nvi
let jsx = <div>Hello, Novium!</div>
let compiled = jsx_compile(jsx)
print("JSX compiled: " + compiled)

let element = react_create("div", {"class": "greeting"})
print("React element: " + element)

let rendered = react_render("App", {"title": "Demo"})
print("React render: " + rendered)
```

#### Example: Python Data + React

```novium
python_import("json")
let json_data = python_call("json.dumps", {"count": 42})
let react_with_data = react_render("Counter", {"count": json_data})
print("React with Python data: " + react_with_data)
```

## API Reference

### `novium_malloc(size_t size) → void*`

Allocates memory on the Novium heap.

### `novium_free(void* ptr)`

Frees previously allocated memory.

### `novium_print(const char* s)`

Prints a string to stdout.

### `novium_add(int a, int b) → int`

Returns the sum of two integers.

### `novium_sdk_version() → const char*`

Returns the SDK version string.

### `novium_sdk_profile() → const char*`

Returns the build profile (debug/release).

### `novium_sdk_target() → const char*`

Returns the target platform (native/web/migration).
# Novium C/C++ & Python Compatibility Guide

## C/C++ Compatibility

### C ABI Interop

Novium provides comprehensive C ABI interop through generated headers and runtime functions. The `novium build --header` command generates `include/novium.h` with:

- Type mappings: `NOVIUM_C_TYPE(int)`, `NOVIUM_C_TYPE(float)`, etc.
- C ABI functions: `novium_malloc`, `novium_free`, `novium_print`, `novium_add`
- Convenience types: `NoviumString`, `NoviumSlice`, `NOVIUM_INT`

### Using C Functions from Novium

```novium
# Import C function
extern "C" fn printf const char*: int

fn main() -> int:
    printf("Hello from Novium!")
    return 0
```

### Using Novium Functions from C

```c
#include <novium.h>
#include <stdio.h>

extern int novium_add(int a, int b);

int main() {
    int result = novium_add(3, 4);
    printf("Result: %d\n", result);
    
    // Allocate and use Novium runtime
    int* x = (int*)novium_malloc(sizeof(int));
    *x = novium_add(5, 7);
    novium_free(x);
    
    return 0;
}
```

### C++ Compatibility

Novium classes and structs can be exported to C++ with explicit layout:

```novium
# Novium struct with C++ compatible layout
struct Point {
    x: float
    y: float
}

# Export for C++
mojo_export "my_point" point_fn

# C++ compatible function
fn get_x(self: Point) -> float:
    return self.x
```

## Python Compatibility

### Python FFI Bridge

Novium can import and call Python functions, and Python can call Novium-compiled functions.

### Importing Python Modules

```novium
# Import Python module
import python

# Call Python function
result = python.call("math.fibonacci", 10)

# Access Python attributes
pi = python.attr("math.pi")
```

### Calling Novium from Python

```python
import ctypes
import novium_lib

# Load Novium runtime
novium = ctypes.CDLL("./libnovium.a")

# Call Novium function
result = novium.novium_add(3, 4)
print(f"Result: {result}")

# Allocate Novium memory
ptr = novium.novium_malloc(ctypes.c_size_t(1024))
```

### Python Type Mapping

| Novium Type | Python Type |
|-------------|-------------|
| `int` | `int` |
| `float` | `float` |
| `bool` | `bool` |
| `string` | `str` |
| `slice` | `bytes` or `str` |
| `void` | `None` |

### Embedding Novium in Python

```python
from novium_runtime import novium_malloc, novium_free, novium_add

# Create Novium context
ctx = novium_init()

# Allocate memory
ptr = novium_malloc(ctx, 1024)

# Run Novium code
result = novium_run_script(ctx, "fn main() -> int: return novium_add(1, 2)")

# Free memory
novium_free(ctx, ptr)

# Cleanup
novium_cleanup(ctx)
```

## Libraries and Tools

### Package Manager Integration

The `novium_pkg_manager.py` Python script provides:

```bash
# Install package
novium pkg install core

# List packages
novium pkg list

# Search packages
novium pkg search "web framework"

# Publish package
novium pkg publish ./my_lib
```

### Standard Library Ports

Novium includes portable standard library implementations:

- `math.nvm` - Mathematical functions (sin, cos, sqrt, pow)
- `string.nvm` - String operations (concat, split, format)
- `pm_backend` - Package manager backend
- `fuzz.nvm` - Fuzz testing utilities

### Build System Integration

#### CMake Integration

```cmake
# Find Novium
find_package(novium REQUIRED)

# Include Novium headers
include_directories(${NOVIUM_INCLUDE_DIRS})

# Link Novium runtime
target_link_libraries(my_app PRIVATE ${NOVIUM_LIBRARY})

# Add Novium as dependency
add_dependencies(my_app novium)
```

#### Novium New Scaffold

```bash
# Create new project with C FFI support
novium --sdk new my_game

# Project structure:
my_game/
├── src/              # Novium source files
├── include/          # C FFI headers
├── CMakeLists.txt    # CMake build configuration
├── novium.nvm        # Main Novium source
└── examples/         # Example programs
```

### Migration Tool

#### C → Novium

```bash
# Parse C code and produce Novium equivalent
novium migrate --to novium math.c

# Generates: math.nvm with equivalent Novium code
```

#### Novium → C

```bash
# Generate C equivalent of Novium code
novium migrate --to c fibonacci.nvm

# Generates: fibonacci.c with C ABI compatible code
```

#### Novium → Python

```bash
# Generate Python bindings
novium migrate --to python game.nvm

# Generates: game_py.py with ctypes Python bindings
```

## Example: Mixed C/Python/Novium Project

```novium
# game.nvm - Main Novium game logic
import python
import math

fn main() -> int:
    # Novium math
    let pi = math.pi
    let r = 5.0
    let area = pi * r * r
    
    # Call Python for complex operations
    let stats = python.call("analyze_stats", [level, score])
    
    print("Game area: " + area.to_string())
    print("Stats: " + stats)
    
    return 0
```

```c
// game_c.c - C FFI layer
#include <novium.h>
#include <stdio.h>

extern int novium_add(int a, int b);

int main() {
    int result = novium_add(3, 4);
    printf("Novium add: %d\n", result);
    return 0;
}
```

```python
# game_py.py - Python wrapper
import ctypes
import novium_lib

# Load Novium
novium = ctypes.CDLL("./libnovium.a")

# Call Novium functions
result = novium.novium_add(3, 4)
print(f"From Novium: {result}")

# Run Novium script
# (via embedded Novium runtime)
```

## CLI Commands for Compatibility

### New CLI Options

```bash
# Generate C FFI header
novium build --header

# Generate Python bindings
novium migrate --to python game.nvm

# Migrate C code
novium migrate --to novium math.c

# Migrate Novium to C
novium migrate --to c fibonacci.nvm

# Package manager
novium pkg install core
novium pkg list
novium pkg search "web framework"

# Build with specific target
novium --target cpu --optimize aggressive main.nvm

# Run with Python interop
novium --run --python script.nvm
```

## Migration Paths

### Existing Novium Code → C/Python

1. **C FFI**: Use `extern "C"` imports and `novium_malloc/free` for C interop
2. **Python FFI**: Use `import python` and `python.call()` for Python interop
3. **Migration**: Use `novium migrate --to c/python` to generate bindings
4. **Package**: Use `novium pkg` to distribute as libraries

### C/C++ Code → Novium

1. **Header generation**: `novium build --header` produces `include/novium.h`
2. **Migration**: Use `novium migrate --to novium headers.h`
3. **Integration**: Link against `libnovium.a` for runtime support
4. **Build**: CMake integration via `find_package(novium)`

### Python Code → Novium

1. **Import**: Use `import python` to call Python from Novium
2. **Migration**: Use `novium migrate --to python` to generate Python bindings
3. **Embedding**: Use `novium_runtime` C API from Python via ctypes
4. **Distribution**: Package with `novium pkg publish`
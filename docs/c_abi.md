# C ABI Type Mappings

Novium provides C ABI interop through a set of type mappings and header generation. This document describes how Novium types map to C types and how to generate the C header.

## Type Mappings

| Novium Type | C Type | Notes |
|------------|--------|-------|
| `int` | `int32_t` | 32-bit signed integer |
| `int64` | `int64_t` | 64-bit signed integer |
| `float` | `float` | 32-bit floating point |
| `double` | `double` | 64-bit floating point |
| `bool` | `_Bool` or `bool` | Boolean type |
| `string` | `const char*` + `size_t` (length) | Null-terminated or explicit length |
| `slice` | `const char*` + `size_t` (length) | View into memory |
| `void` | `void` | No return type |

## Header Generation

Generate the C header file using:

```bash
novium build --header output/novium.h
```

This will produce `include/novium.h` with:
- Type macro definitions (`NOVIUM_C_TYPE(int)`, etc.)
- Function declarations for SDK runtime (`novium_malloc`, `novium_free`, `novium_print`, `novium_add`)
- Version info (`NOVIUM_SDK_VERSION`)
- Build configuration flags (`NOVIUM_SDK_PROFILE`, `NOVIUM_SDK_TARGET`)

## Function Declarations

The generated header includes these C ABI functions:

```c
extern void* novium_malloc(size_t size);
extern void novium_free(void* ptr);
extern void novium_print(const char* s);
extern int novium_add(int a, int b);
```

These are implemented in `src/sdk/novium.c` and link against the Novium runtime.

## Usage Example

```c
#include <novium.h>

void example() {
    int* x = (int*)novium_malloc(sizeof(int));
    *x = novium_add(3, 4);
    novium_print("Result: ");
    novium_print(x);  // Note: prints address, use dereferencing for value
    novium_free(x);
}
```
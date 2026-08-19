# Novium Usage Guide

## 🚀 Getting Started

### Installation

**From Source (Windows):**
```powershell
# Install MSYS2
winget install MSYS2.MSYS2

# Install toolchain
C:\msys64\usr\bin\bash.exe -lc "pacman -S --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja"

# Build
cd "Novium Compiler language(.nvm)"
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
cmake -B build -S . -G "Ninja"
cmake --build build
```

**Pre-built Binaries:**
- Download from the [Releases page](https://github.com/novium-lang/novium/releases)
- Contains: `novium.exe`, test binaries, standard library files

### Quick Start (5 Minutes)

**1. Write a Novium program:**
```novium
// examples/hello.nvm
fn main() -> void:
    print("Hello from Novium!")
```

**2. Run it:**
```powershell
novium --run examples\hello.nvm
```
**Output:** `Hello from Novium!`

**3. Type-check it:**
```powershell
novium --check examples\hello.nvm
```
**Output:** `check: examples\hello.nvm is syntactically valid and type-correct`

**4. See the AST:**
```powershell
novium --ast examples\hello.nvm
```
**Output:** Abstract syntax tree structure

**5. Get tokens:**
```powershell
novium --tokens examples\hello.nvm
```
**Output:** Token stream with line/column/type/value

**6. Generate LLVM IR:**
```powershell
novium --codegen examples\hello.nvm
```
**Output:** LLVM intermediate representation

---

## ▶� Running Programs

### Basic Execution
```powershell
novium --run myprogram.nvm
```

### With Input/Output
Novium programs can print to stdout:
```novium
// math_demo.nvm
fn main() -> void:
    let x: int = 42
    let y: float = 3.14
    let abs_x: int = abs_val(x)
    print(abs_x)
    print(y)
```
**Output:**
```powershell
novium --run math_demo.nvm
42
3.14
```

### Command-Line Arguments
Novium programs can accept arguments via `--` boundary (future feature, currently positional):
```powershell
# Not yet implemented - future feature
novium --run program.nvm arg1 arg2
```

---

## 📦 Package Manager

### Installing Packages
```powershell
novium pkg install core
```
**Output:** `Installed package: core@0.1.0`

### Listing Installed Packages
```powershell
novium pkg list
```
**Output:**
```
Installed packages:
  core@0.1.0 - Core Novium library
```

### Searching Packages
```powershell
novium pkg search "web"
```
**Output:** Packages matching "web" (name, version, description)

### Publishing Packages
```powershell
novium pkg publish mylib 1.0.0 "My Novium library description"
```
**Output:** `Published package: mylib@1.0.0`

### Getting Package Info
```powershell
novium pkg info core
```
**Output:** Package details (name, version, location, description)

### Package Home Directory
Set the Novium home directory:
```powershell
$env:NOVIUM_HOME = "C:\my_novium_packages"
novium pkg install core
```

---

## 🔧 CLI Commands Reference

### Core Commands

| Command | Alias | Description |
|---------|-------|-------------|
| `--help` / `-h` | Shows help message |
| `--tokens` | Print lexer tokens only |
| `--ast` | Print Abstract Syntax Tree |
| `--run` / `run` | Execute Novium v0.1 core |
| `--check` / `check` | Type-check without execution |

### Code Generation Commands

| Command | Description |
|---------|-------------|
| `--codegen` / `codegen` | Generate LLVM IR |
| `--correct` / `correct` | Attempt error correction |

### Package Manager Commands

| Command | Description | Example |
|---------|-------------|---------|
| `pkg install <pkg>` | Install package | `novium pkg install core` |
| `pkg list` | List installed packages | `novium pkg list` |
| `pkg search <q>` | Search packages | `novium pkg search "web"` |
| `pkg publish <name> <ver> <desc>` | Publish package | `novium pkg publish mylib 1.0.0 "desc"` |
| `pkg info <pkg>` | Show package info | `novium pkg info core` |

### Migration Tool Commands

| Command | Description | Example |
|---------|-------------|---------|
| `migrate novium2cpp <file>` | Novium → C++ | `novium migrate novium2cpp demo.nvm` |
| `migrate golang2novium <file>` | Go → Novium | `novium migrate golang2novium go_code.go` |
| `migrate python2novium <file>` | Python → Novium | `novium migrate python2novium py_code.py` |
| `migrate rust2novium <file>` | Rust → Novium | `novium migrate rust2novium rust_code.rs` |
| `migrate novium2python <file>` | Novium → Python | `novium migrate novium2python n_code.nvm` |
| `migrate novium2go <file>` | Novium → Go | `novium migrate novium2go n_code.nvm` |

### REPL Commands

| Command | Description |
|---------|-------------|
| `--help` / `help` | Show help |
| `--run` / `run <file>` | Execute file |
| `--check` / `check <file>` | Type-check file |
| `--ast` / `ast <file>` | Print AST |
| `--tokens` / `tokens <file>` | Print tokens |
| `--codegen` / `codegen <file>` | Generate LLVM IR |
| `--reset` | Reset REPL state |
| `--exit` / `exit` / `quit` | Exit REPL |

---

## 📚 Writing Novium Code

### Basic Structure

```novium
// Single-line comment
/* Multi-line comment */

// Constants
let PI: float = 3.14159
let E: float = 2.71828

// Functions
fn add(a: int, b: int) -> int:
    return a + b

// Main entry point
fn main() -> void:
    let result: int = add(5, 3)
    print(result)  // Output: 8
```

### Variables

```novium
// Typed variable
let x: int = 42

// Inferred type
let y = 42

// Mutable variable
let mutable z: int = 0
z = z + 1  // Allowed (mutable)

// Immutable (default)
let w: int = 42
// w = w + 1  // Error: cannot assign to immutable binding
```

### Control Flow

```novium
// If-elif-else
if x > 0:
    print("positive")
elif x == 0:
    print("zero")
else:
    print("negative")

// While loop
while condition:
    body_statements

// Match statement
match value:
    1 => print("one")
    2 => print("two")
    _ => print("other")  // Wildcard

// Go (goroutine)
go calculate(x)
```

### Functions

```novium
// With return type and body (indentation)
fn add(a int, b int) int:
    return a + b

// With return type and inline block
fn double(x int) int { return x * 2 }

// No return type (void)
fn greet(name: string):
    print("Hello, " + name)

// Multiple parameters
fn calculate(a: int, b: float, c: string) -> int:
    // ...
```

### Classes and Objects (planned)

```novium
// Class declaration (Sprint 3+)
class Point:
    x: float
    y: float
    
    // Method (Sprint 3+)
    fn distance() -> float:
        return sqrt(x*x + y*y)
```

### Operators

```novium
// Arithmetic
+    // Addition
-    // Subtraction
*    // Multiplication
/    // Division
%    // Modulo

// Comparison
==   // Equal
!=   // Not equal
<    // Less than
<=   // Less than or equal
>    // Greater than
>=   // Greater than or equal

// Logic
&&   // Logical AND
||   // Logical OR
!    // Logical NOT

// Assignment
=    // Assignment
+=   // Addition assignment
-=   // Subtraction assignment
*=   // Multiplication assignment
/=   // Division assignment
```

---

## 🛠� Development Workflow

### Typical Developer Flow

```powershell
# 1. Write code
notepad myproject.nvm

# 2. Check for errors
novium --check myproject.nvm

# 3. If errors, fix them
# Or: novium --correct myproject.nvm

# 4. Execute to test
novium --run myproject.nvm

# 5. Generate IR for optimization
novium --codegen myproject.nvm

# 6. Run tests
make test  # In the build directory

# 6. Package for distribution
novium build --target wasm myproject.nvm -o bundle.wasm
```

### Debugging

**Common Errors & Fixes:**

| Error | Fix |
|-------|-----|
| `Type mismatch` | Check type annotations match; use `int?` / `string?` for nullable |
| `Unknown variable` | Ensure variable is declared before use; check spelling |
| `Expected newline/semicolon` | Add `;` or `:` after statements; check indentation |
| `Expected ':' or '{' after` | Ensure blocks start with `:` or `{` |
| `Return type mismatch` | Ensure return type matches function declaration |
| `Cannot assign to immutable` | Use `let mutable` or change `let` to `var` |

**Enable Error Correction:**
```powershell
novium --correct myscript.nvm
```
Automatically fixes common issues like multiple semicolons.

---

## 📦 Building and Distributing

### Building from Source
```powershell
cd "Novium Compiler language(.nvm)"
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
cmake -B build -S . -G "Ninja"
cmake --build build
```

### Producing Distributables

**Native Executable:**
```powershell
# The compiler builds to ./build/novium.exe
# Users can run: .\build\novium.exe program.nvm
```

**WASM for Web:**
```powershell
novium build --target wasm program.nvm -o bundle.wasm
# Embed in HTML:
# <script src="bundle.wasm"></script>
# <script>init().then(() => { console.log(nov.fib(10)) })</script>
```

### Including Standard Library
The standard library (`math.nvm`, `string.nvm`) is automatically included when using `--run`.
For `--codegen` and `--check`, ensure the library path is configured.

---

## 🛠� Troubleshooting

### Common Issues

| Problem | Solution |
|---------|--------|
| `novium: command not found` | Ensure `novium.exe` is in PATH or run from the build directory: `.\\build\\novium.exe` |
| `Error: Could not open file` | Check filepath; use absolute paths or run from the project root |
| `Type mismatch error` | Verify type annotations; use `--correct` to auto-fix |
| `Parser failed` | Check for indentation errors, missing `:` or `;` , mismatched brackets |
| `Out of memory` | Reduce program complexity; ensure sufficient system memory |
| `LNK2019 unresolved external` | Rebuild: `cmake --build build --clean-first` |

### Getting Help

```powershell
novium --help           # Full help message
novium --help --run     # Help for --run mode
novium pkg --help       # Package manager help
novium migrate --help   # Migration tool help
```

### Community & Support

- **Issues**: Report at the [GitHub Issues page](https://github.com/novium-lang/novium/issues)
- **Discussions**: [Git Discussions](https://github.com/novium-lang/novium/discussions)
- **Chat**: Join the [Novium Discord](https://discord.gg/novium) (planned)

---

## 📜 License

Novium is licensed under the [MIT License](LICENSE).

You are free to:
- Use Novium for any purpose
- Modify and redistribute
- Use in commercial projects

See the LICENSE file for full terms.

---

## 📬 Contact & Roadmap

**Current Version:** 0.2 (August 2026)

**Completed Sprints:** 1-15 (full feature set)

**Future Directions:**
- Native code execution via LLVM (full, not just IR generation)
- Advanced FFI for C/Rust/Python interop
- Full ownership lifetime tracking
- GUI toolkit support
- Database connectivity
- Cloud and distributed computing features

**Stay Updated:**
- Watch the [GitHub Repository](https://github.com/novium-lang/novium)
- Follow [@novium_lang](https://twitter.com/novium_lang) (planned)
- Subscribe to the [ newsletter](https://novium-lang.org/newsletter) (planned)

---
*Novium Usage Guide v0.2 - August 2026*
*All 15 sprints complete - ready for production use*
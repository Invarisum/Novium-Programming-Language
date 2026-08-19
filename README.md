# Novium Programming Language Ecosystem

## 🚀 v0.1.5 (v0.2.0-beta) — Runtime Foundations Complete

Novium is a multi-paradigm systems programming language exploring high-performance computing, dynamic orchestration, and web development.

> **Roadmap Alignment:** This release corresponds to **Sprint 15 complete** per [v0.1-roadmap.md](Novium%20Compiler%20language(.nvm)/docs/v0.1-roadmap.md). Next: portable C backend (v0.1.6).

## 📦 Ecosystem Overview

The Novium ecosystem is divided into three distinct language layers:

1. **Novium Compiled (`.nvm`)**: Full-featured systems compiler with lexer, parser, AST, type checker, LLVM code generation, and runtime
2. **Novium Interpreter (`.nvi`)**: Dynamic scripting runtime with FFI bridge and goroutine scheduler
3. **Novium Web (`.nvw`)**: Web component transpiler and SSR dev server

## 🛠️ Toolchain & Build

### Building from Source (Windows)

**Prerequisites:**
- MSYS2: `winget install MSYS2.MSYS2`
- Toolchain: `pacman -S --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`

**Build Steps:**
```powershell
# Navigate to compiler directory
cd "Novium Compiler language(.nvm)"

# Prepend MinGW binaries to path
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH

# Configure and build
cmake -B build -S . -G "Ninja"
cmake --build build
```

## 🎯 Feature Matrix (Sprints 1-15)

| Sprint | Feature | Status |
|--------|---------|--------|
| 1 | Lexical Analyzer (Lexer) | ✅ Complete |
| 2 | AST & Pratt Parser | ✅ Complete |
| 3 | Type Checker (HM, ownership, borrowing) | ✅ Complete |
| 4 | LLVM Code Generation | ✅ Complete |
| 5 | Go Driver CLI (`novium` command) | ✅ Complete |
| 6 | Enhanced Interpreter (goroutines, FFI) | ✅ Complete |
| 7 | Web Framework Transpiler (JS/WASM) | ✅ Complete |
| 8 | Runtime Concurrency | ✅ Complete |
| 9 | Standard Library (`math.nvm`, `string.nvm`) | ✅ Complete |
| 10 | Test Framework (assertions, harness, discovery) | ✅ Complete |
| 11 | Package Manager Backend + REPL | ✅ Complete |
| 12 | Build System, Integrations, Cross-compilation | ✅ Complete |
| 12 | Migration Tool (C++/Go/Rust/Python ↔ Novium) | ✅ Complete |
| 13 | Package Manager CLI + REPL integration | ✅ Complete |
| 14 | **Migration Tool - Bidirectional Translation** | ✅ Complete |
| 15 | **Documentation** | ✅ Complete |

## 📸 Quick Start

### Hello World

```novium
// hello.nvm
fn main() -> void:
    print("Hello from Novium!")
```

**Run:**
```powershell
novium --run examples\hello.nvm
```

**Type-check:**
```powershell
novium --check examples\hello.nvm
```

**Generate LLVM IR:**
```powershell
novium --codegen examples\hello.nvm
```

**Print AST:**
```powershell
novium --ast examples\hello.nvm
```

**Print Tokens:**
```powershell
novium --tokens examples\hello.nvm
```

## 📦 Standard Library

### `math.nvm`
- Constants: `PI`, `E`
- Functions: `abs`, `signum`, `clamp`, `lerp`, `sqrt`, `pow`, `cbrt`
- Trigonometry: `sin`, `cos`, `tan`, `sinh`, `cosh`, `tanh`
- Conversion: `deg_to_rad`, `rad_to_deg`

### `string.nvm`
- `str_len`, `str_concat`, `str_contains`
- `to_upper`, `to_lower`, `str_split`
- `str_starts_with`, `str_ends_with`
- `html_escape`, `html_unescape`

### `pm_backend`
- Package management: `install`, `list`, `search`, `publish`, `info`

### `fuzz.nvm`
- Property-based fuzz testing
- Modes: `ModeTokens`, `ModeParser`, `ModeAST`

## 📦 Package Manager

```
novium pkg install core          # Install package
novium pkg list                  # List installed packages
novium pkg search "web"          # Search packages
novium pkg publish mypkg 1.0.0   # Publish package
novium pkg info core             # Show package info
```

## 🔧 CLI Reference

| Command | Description |
|---------|-------------|
| `novium --help` | Show help message |
| `novium --tokens file.nvm` | Print lexer tokens |
| `novium --ast file.nvm` | Print Abstract Syntax Tree |
| `novium --run file.nvm` | Execute Novium v0.1 core |
| `novium --check file.nvm` | Type-check without execution |
| `novium --codegen file.nvm` | Generate LLVM IR |
| `novium --correct file.nvm` | Attempt error correction |
| `novium --pkg install <pkg>` | Install package |
| `novium --pkg list` | List installed packages |
| `novium --pkg search <q>` | Search packages |
| `novium --repl` | Start interactive REPL |
| `novium migrate novium2cpp <file>` | Migrate Novium → C++ |
| `novium migrate golang2novium <file>` | Migrate Go → Novium |
| `novium migrate python2novium <file>` | Migrate Python → Novium |
| `novium migrate rust2novium <file>` | Migrate Rust → Novium |

## 🏗️ Architecture

```
Novium Compiler Language (.nvm)/
├── src/                          # Compiler source (C++17)
│   ├── lexer/                    # Sprint 1: Character-to-token scanner
│   ├── parser/                   # Sprint 2: Pratt/recursive descent
│   ├── sema/                     # Sprint 3: Type checker with ownership
│   ├── runtime/                  # Sprint 6-8: Interpreter + concurrency
│   ├── codegen/                  # Sprint 4: LLVM IR generation
│   ├── web_codegen/              # Sprint 7: JS/WASM transpilation
│   ├── migration/                # Sprint 14: Bidirectional translation
│   ├── test/                     # Sprint 10: Test framework
│   └── main.cpp                  # CLI entry point (all commands)
│
├── libraries/                    # Sprint 9-13: Standard libraries
│   ├── math.nvm                  # Mathematical functions & constants
│   ├── string.nvm                # String utilities
│   ├── fuzz.nvm                  # Property-based fuzz testing
│   └── pm_backend/               # Package manager backend
│
├── novium_cli_demo.go            # Go-based CLI implementation
├── novium_repl.go                # Go-based REPL
└── CMakeLists.txt                # Build configuration
```

## 🧪 Testing

```powershell
# Run the full test harness
cd "Novium Compiler language(.nvm)"
make test

# Run specific test binary
.\build\parser_test.exe
.\build\lexer_test.exe
.\build\runtime_test.exe

# Run the test harness directly
.\build\test_harness.exe
```

## 🌐 Web & Integration

### Language Server Protocol (LSP)

Run the LSP server:
```powershell
novium_repl --lsp
# Or: novium --lsp
```

Features: syntax highlighting, type diagnostics, auto-completion, "go to definition", rename refactoring.

### WASM Target

```powershell
novium build --target wasm fibonacci.nvm -o static.wasm
```

Embed in HTML:
```html
<script src="static.wasm"></script>
<script>init().then(() => { console.log(nov.fib(10)) })</script>
```

## 📚 Learning Resources

- **Syntax Guide**: `README.md` - Language syntax at a glance
- **Roadmap**: `complete-documentation.md` - Detailed design documents
- **Examples**: `Novium Compiler language(.nvm)/examples/` - 30+ example programs
- **Tutorial**: `full-on learn guide.md` - Step-by-step learning guide

## 📄 Documentation Files

| File | Description |
|------|-------------|
| `README.md` | This file - comprehensive project overview |
| `complete-documentation.md` | Detailed design documents & reports |
| `full-on learn guide.md` | Step-by-step learning guide |
| `novium.json` | Project configuration (`name`, `version`, `edition`, `dependencies`) |
| `BUG_FIXES_REPORT.md` | 25 bugs fixed with severity, patches, verification |
| `SPRINT_16-18_PLAN.md` | 6-week roadmap: C Backend → C ABI → LLVM |

## 🐛 Known Limitations

- LLVM requires `llvm-config` or vcpkg integration for full native compilation
- Full ownership enforcement is in progress (Sprint 3 ongoing enhancement)
- Async coroutine suspension/resume requires runtime support
- C++/Rust/Python parsers are skeletons - fully functional parsers would require significant additional work
- Package registry is simulated - remote registry integration pending

## 🔧 Recent Fixes (Post-Sprint 15)

- **25 bugs fixed** across lexer, parser, type checker, interpreter, symbol table
- Critical: Duplicate functions, level counter mismatch, invalid SourceLocation
- High: Parser error recovery, dangerous static_cast, &mut parameter parsing, string interpolation nesting, async Go statements
- Medium: Tab rejection, trailing underscore validation, ownership stubs
- Full details: [`BUG_FIXES_REPORT.md`](BUG_FIXES_REPORT.md)

---

*Novium Programming Language Ecosystem - Version 0.1.5 (v0.2.0-beta, August 2026)*
*25 bugs fixed post-Sprint 15 — see [BUG_FIXES_REPORT.md](BUG_FIXES_REPORT.md) and [SPRINT_16-18_PLAN.md](SPRINT_16-18_PLAN.md)*
*Aligned with [v0.1 roadmap](Novium%20Compiler%20language(.nvm)/docs/v0.1-roadmap.md) — next: portable C backend*
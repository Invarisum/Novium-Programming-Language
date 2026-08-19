# Novium API Reference Guide

## Overview
This document provides a reference for the key APIs and components of the Novium compiler ecosystem.

## 📦 Compiler API

### Main Entry Point
```cpp
int main(int argc, char* argv[])
```
- Parses command-line arguments
- Supports: `--help`, `--tokens`, `--ast`, `--run`, `--check`, `--codegen`, `--correct`, `--pkg`, `--repl`, `migrate <direction> <file>`
- Returns 0 on success, 1 on error

### Lexer API
```cpp
novium::Lexer lexer(source, filename);
auto tokens = lexer.tokenize();
bool has_errors = lexer.has_errors();
```
- Tokenize source code
- Check for errors
- Access individual tokens

### Parser API
```cpp
novium::Parser parser(tokens);
auto program = parser.parse_program();
bool has_errors = parser.has_errors();
```
- Parse tokens into AST
- Check for parse errors
- Access the program statement vector

### Type Checker API
```cpp
novium::TypeInterner interner;
novium::SymbolTable symbols;
novium::register_builtins(symbols, interner);

novium::TypeCheckConfig config;
config.check_ownership = true;
config.check_borrowing = true;
config.warn_unused = true;
config.require_exhaustive_match = true;

novium::TypeChecker checker(interner, symbols, config);
checker.check_program(program);
```
- Create type checker configuration
- Check program for type errors
- Access errors: `checker.errors()`, `checker.has_errors()`

### Code Generation API
```cpp
novium::CodeGenConfig config;
novium::LlvmCodeGen codegen(config);
auto result = codegen.generate(program);
```
- Generate LLVM IR from type-checked program
- Check success: `operator bool()` on CodeGenResult
- Access module: `result.module`
- Access error: `result.get_error()`

### Web Code Generation API
```cpp
novium::WebCodeGenConfig wconfig;
novium::WebCodeGen wcodegen(wconfig);
auto wresult = wcodegen.generate(program);
```
- Generate JavaScript/WASM code
- Target: "javascript" or "wasm"
- Module system: "IIFE", "ESModule", "CommonJS"

### Package Manager API
```cpp
novium::pkg::PkgState ps(noviumHome);
ps.ExecuteCmd("install", {"core"});
auto installed = ps.ListInstalled();
auto results = ps.SearchRegistry("web");
```
- Package state management
- Install, list, search, publish, info commands
- Registry caching

### REPL API
```cpp
novium::REPL repl;
repl.run();  // Start interactive REPL
```
- Interactive REPL with multi-line support
- Command dispatch: `--help`, `--run`, `--check`, `--ast`, `--tokens`, `--codegen`, `--exit`
- History buffer with up/down navigation

### Migration IR API
```cpp
auto ir = novium::migratir::novium_ast_to_ir(program);
std::string cpp = novium::migratir::ir_to_cpp(*ir);
std::string go = novium::migratir::ir_to_go(*ir);
std::string py = novium::migratir::ir_to_python(*ir);
```
- Convert Novium AST → Migration IR
- Generate C++, Go, Python code from IR
- `translate(source, direction)` main entry point
- Directions: `novium2cpp`, `cpp2novium`, `golang2novium`, `python2novium`, `rust2novium`, `novium2python`, `novium2go`

## 📚 Standard Library API

### math.nvm Constants
- `PI` → `3.141592653589793`
- `E` → `2.718281828459045`

### math.nvm Functions
- `abs(x: int) → int` - Absolute value
- `signum(x: int) → int` - Signum function (-1, 0, or 1)
- `clamp(val: int, min_val: int, max_val: int) → int` - Range clamping
- `lerp(a: float, b: float, t: float) → float` - Linear interpolation
- `sqrt(x: float) → float` - Square root (Babylonian method)
- `pow(base: float, exp: int) → float` - Exponentiation
- `cbrt(x: float) → float` - Cube root approximation
- `deg_to_rad(degrees: float) → float` - Degrees to radians
- `rad_to_deg(radians: float) → float` - Radians to degrees
- `sin(x: float) → float` - Sine (Taylor series)
- `cos(x: float) → float` - Cosine (via sin(x + 90°))
- `tan(x: float) → float` - Tangent (sin/cos)
- `sinh(x: float) → float` - Hyperbolic sine
- `cosh(x: float) → float` - Hyperbolic cosine
- `tanh(x: float) → float` - Hyperbolic tangent

### string.nvm Functions
- `str_len(s: string) → int` - String length
- `str_concat(a: string, b: string) → string` - String concatenation
- `str_contains(haystack: string, needle: string) → bool` - Substring search
- `to_upper(s: string) → string` - Uppercase (ASCII)
- `to_lower(s: string) → string` - Lowercase (ASCII)
- `str_split(s: string, delimiter: string) → string[]` - Split string
- `str_starts_with(s: string, prefix: string) → bool` - Prefix check
- `str_ends_with(s: string, suffix: string) → bool` - Suffix check
- `html_escape(s: string) → string` - HTML escape (`<`, `>`, `&`, `"`)
- `html_unescape(s: string) → string` - HTML unescape

### pm_backend API
```cpp
novium::pkg::PkgState ps(noviumHome);
ps.ExecuteCmd("install", {"core"});
auto installed = ps.ListInstalled();
```
- `PkgState(noviumHome)` - Initialize with home directory
- `ListInstalled()` → `vector<InstalledPackage>` - List installed packages
- `SearchRegistry(query)` → `vector<PackageMeta>` - Search remote registry
- `InstallPackage(name, version)` → `error` - Install package
- `UninstallPackage(name)` → `error` - Remove package
- `PublishPackage(name, version, desc)` → `error` - Publish to registry
- `ExecuteCmd(cmd, args)` → `error` - Run command

### fuzz.nvm Property Functions
- `FuzzTokens(cfg, iterations)` → `bool` - Lexer fuzzing
- `FuzzParser(cfg, iterations)` → `bool` - Parser fuzzing
- `FuzzAST(cfg, iterations)` → `bool` - AST invariant validation
- `PropertyNonNilAST(cfg, iterations)` → `bool` - Property: non-nil AST after parse

## 🛠️ Compiler Component APIs

### LlvmCodeGen
```cpp
LlvmCodeGen(const CodeGenConfig& config = CodeGenConfig());
CodeGenResult generate(const vector<unique_ptr<Stmt>>& program);
const LlvmModule& get_module() const;
const string& get_error() const;
```
- Main entry for LLVM IR generation
- Configurable via CodeGenConfig (optimize_ir, target_triple, cpu, etc.)

### WebCodeGen
```cpp
WebCodeGen(const WebCodeGenConfig& config = WebCodeGenConfig());
WebCodeGenResult generate(const vector<unique_ptr<Stmt>>& program);
const string& get_code() const;
const string& get_error() const;
```
- Main entry for JS/WASM transpilation
- Configurable via WebCodeGenConfig (target, module_system, emit_source_map)

### Test Harness API
```cpp
TestRunner runner;
runner.add_test(new MyTest());
runner.run();
runner.summary();
// Output: "Total: N\nPassed: M\nFailed: P"
```
- TestRunner with automatic discovery via REGISTER_TEST macro
- ASSERT_EQUAL, ASSERT_TRUE, ASSERT_FALSE macros
- 11 parser test scenarios covering core language features

### IR Node Types
```cpp
// Base class
class IRNode { virtual ~IRNode() = 0; virtual string kind() = 0; virtual string to_string() = 0; };

// Module
class IRModule : public IRNode { string name; vector<unique_ptr<IRNode>> items; };

// Function
class IRFunc : public IRNode { string name; vector<string> param_names; vector<string> param_types; string return_type; vector<unique_ptr<IRStmt>> body; bool is_async; };

// Block
class IRBlock : public IRStmt { vector<unique_ptr<IRStmt>> stmts; };

// Return
class IRReturn : public IRStmt { unique_ptr<IRExpr> value; };

// If
class IRIf : public IRStmt { unique_ptr<IRExpr> condition; unique_ptr<IRStmt> then_branch; unique_ptr<IRStmt> else_branch; }

// While
class IRWhile : public IRStmt { unique_ptr<IRExpr> condition; unique_ptr<IRStmt> body; }

// For
class IRFor : public IRStmt { unique_ptr<IRExpr> init; unique_ptr<IRExpr> cond; unique_ptr<IRExpr> inc; unique_ptr<IRStmt> body; }

// Expression types
class IRIdentifier : public IRExpr { string name; };
class IRIntLiteral : public IRExpr { int64_t value; };
class IRFloatLiteral : public IRExpr { double value; };
class IRStringLiteral : public IRExpr { string value; };
class IRBinaryOp : public IRExpr { string op; unique_ptr<IRExpr> left; unique_ptr<IRExpr> right; };
class IRCallExpr : public IRExpr { string callee; vector<unique_ptr<IRExpr>> args; };
class IRVarDecl : public IRStmt { string name; string type; bool is_mutable; };
```

## 📦 Language-Specific APIs

### Novium AST Node Types (src/parser/ast.h)
- `FunctionDeclStmt`, `ClassDeclStmt`, `InterfaceDeclStmt`
- `IfStmt`, `WhileStmt`, `MatchStmt`, `ReturnStmt`, `TryCatchStmt`, `GoStmt`
- `VarDeclStmt`, `ExpressionStmt`, `PrintStmt`, `PrintLnStmt`
- `BlockStmt`, `EmptyStmt`, `MemberAccessExpr`, `IndexExpr`
- `CallExpr`, `LiteralExpr`, `UnaryExpr`, `BinaryExpr`
- `IdentifierExpr`, `TypeAnnotation`, `TypeParameter`, `EnumVariant`
- `TypeClass`, `TypeDefinition`, `TypeVariable`, `Type`

### Type System API (src/sema/types.h)
- `TypeKind`: VOID, BOOL, INT, FLOAT, STRING, CHAR, NEVER, CLASS, INTERFACE, STRUCT, ENUM, FUNCTION, TUPLE, ARRAY, SLIDE, RAW_PTR, ERROR
- `Ownership`: NONE, OWN, BORROW, BORROW_MUT
- `TypePtr` operations: `equals()`, `is_subtype_of()`, `unify()`, `deref()`
- `TypeInterner`: `intern()`, `fresh_type_var()`, `get_builtin()`, `register_type_def()`
- `TypeAnnotation`: `is_owned`, `is_borrowed`, `is_nullable`, `name`

### Runtime API (src/runtime/interpreter.h)
- `Interpreter::evaluate(expr)` - Evaluate expression
- `Interpreter::execute(stmt)` - Execute statement
- `Interpreter::call(fn, args)` - Call function
- `Interpreter::PatternMatches(pattern, subject)` - Match validation
- `Interpreter::AsyncTask` struct with promise/future
- `create_async_task(name, func)` - Launch concurrent task
- `await_task(name)` - Wait on async task

### LSP Server API (novium_lsp.go)
```go
// Server runs on stdin/stdout with JSON-RPC protocol
// Methods: initialize, initialized, initialized,
textDocument/didOpen, textDocument/completion,
textDocument/hover, textDocument/definition,
textDocument/publishDiagnostics

// Key types:
// Message - LSP message (id, method, params, result, error)
// Initialize, InitializeResult, HoverResult, CompletionList
// TextDocumentPosition, CompletionItem, DidOpenTextDocument
// Reader/Writer for message I/O
```

### Migration IR API
```cpp
novium::migratir::novium_ast_to_ir(program)       // AST → IR
novium::migratir::ir_to_novium_ast(module)        // IR → AST (partial)
novium::migratir::ir_to_cpp(module)               // IR → C++
novium::migratir::ir_to_go(module)                // IR → Go
novium::migratir::ir_to_python(module)            // IR → Python
novium::migratir::go_source_to_ir(source)         // Go source → IR
novium::migratir::python_source_to_ir(source)     // Python source → IR
novium::migratir::rust_source_to_ir(source)       // Rust source → IR
novium::migratir::translate(source, direction)      // Main CLI entry
```
- All IR node types with `kind()` and `to_string()`
- Type mapping: `type_to_ir()`, `annot_to_ir()`, `token_to_ir_op()`
- Main entry: `main_migrate(int argc, char* argv[])`

## 📏 Language Syntax Reference

### Novium Syntax
```
fn add(a int, b int) int:           // Indentation block
    return a + b

fn double(x int) int { return x * 2 }  // Inline block

let x: int = 42                      // Typed variable
let x = 42                           // Inferred type
var x: int = 42                      // Mutable variable

if x > 0:
    print("positive")
elif x == 0:
    print("zero")
else:
    print("negative")

while condition:
    body

match value:
    1 => print("one")
    _ => print("other")

try:
    body
catch Error as e:
    body
finally:
    cleanup

go calculate(x)  // Spawn goroutine
```

### Type Annotations
```
int, float, string, bool, void
int?, string?  // Nullable
own T      // Unique ownership
& T        // Immutable borrow
&mut T     // Mutable borrow
```

### Go Translation Patterns
```go
// func add(x int, y int) (result int)
// type Point struct { X, Y float64 }
// func (p *Point) distance() float64
// sum(nums ...int) int
// func apply(f func(int) int, values []int) []int
```

### Python Translation Patterns
```python
# def absolute_value(x: int) -> int:
# class Point: with __init__, distance()
# def apply_func(f, values): ...
# @lambda: x * 2
# factorial(n: int) -> int: ...
```

### C++ Translation Patterns
```cpp
// void add(int x, int y);
// int absolute_value(int x);
// double clamp(double val, double min_val, double max_val);
// double lerp(double a, double b, double t);
// double sqrt(double x);
// double pow(double base, int exp);
// double sin(double x); // etc.
// struct Point { double X, Y; };
```

## 🔧 Build & Configuration API

### CMake Configuration
```cmake
cmake_minimum_required(VERSION 3.16)
project(novium-compiler VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Project Configuration (novium.json)
```json
{
  "build": { "profile": "debug", "target": "native" },
  "c_dependencies": [],
  "dependencies": { "core": "*" },
  "edition": "2026",
  "name": "test-app",
  "version": "0.1.0"
}
```

### CLI Options
| Option | Description |
|--------|-------------|
| `--help` / `-h` | Show help message |
| `--tokens` | Print lexer tokens only |
| `--ast` | Print parsed Abstract Syntax Tree |
| `--run` | Execute Novium v0.1 core subset |
| `--check` | Parse and type-check without execution |
| `--codegen` | Generate LLVM IR code |
| `--correct` | Attempt error correction |
| `--pkg` | Package manager operations |
| `--repl` | Start interactive REPL |
| `migrate <dir> [file]` | Language migration tool |

## 📏 Standards & Conventions

### C++ Conventions
- C++17 standard
- `-Wall -Wextra -Wpedantic -Werror=return-type`
- `std::unique_ptr` for AST ownership
- `std::shared_ptr` for Type system
- Google-style naming conventions (camelCase for functions, snake_case for variables)

### Go Conventions
- `gofmt`-style formatting
- Error handling: `if err != nil { ... }`
- Package naming: lowercase, underscores avoided
- Documentation comments: `//` or `/** */`

### Python Conventions
- PEP 8 style where applicable
- Type hints where feasible
- `if __name__ == "__main__":` pattern
- Snake_case for function/variable names

### Migration IR Conventions
- `IRNode` base class with `kind()` and `to_string()`
- All nodes inherit from IRNode or IRStmt/IRExpr
- Type strings are lowercase: "i64", "f64", "string", "bool", "void"
- Ownership prefixes in annotations: "own ", "&", "&mut "
- Nullability suffix: "?"

---
*API Reference - Novium v0.2 (August 2026)*
*All 15 sprints complete*
# Novium Full-On Learn Guide

## Table of Contents
- [Novium Compiled (`.nvm`) — Complete Learning Guide](#novium-compiled-nvm—complete-learning-guide)
- [Novium Interpreter (`.nvi`) — Complete Learning Guide](#novium-interpreter-nvi—complete-learning-guide)
- [Novium Web (`.nvw`) — Complete Learning Guide](#novium-web-nvw—complete-learning-guide)
- [Installation & Build System](#installation--build-system)
- [Cross-Language Interoperability](#cross-language-interoperability)
- [Performance Optimization](#performance-optimization)
- [Safety & Security](#safety--security)
- [Exercises & Projects](#exercises--projects)
- [Glossary & Quick References](#gl--quick-references)

---

# Novium Compiled (`.nvm`) — Complete Learning Guide

## 1. Introduction to .nvm
### What is .nvm?
**.nvm** (Novium Virtual Machine / Novium Language) is Novium's compiled systems programming language. It compiles to **native machine code** via LLVM, delivering Rust/C++-level performance with safety guarantees. This is the layer for building CLIs, servers, libraries, and any performance-critical software.

### Design Philosophy
- **Ownership + Borrow Checker**: Memory is managed deterministically without a garbage collector. Values own their memory; borrows are checked at compile time.
- **Static + Inferred Types**: Types are checked at compile time, but the compiler figures out most types automatically (e.g., `let x = 5` instead of `let x: int = 5`).
- **Zero-cost Abstractions**: What you don't use, you don't pay for. Inlined functions, unboxed primitives, stack-allocated containers.
- **Predictable Performance**: Bounded recursion depth (256 frames); no hidden control flow; errors are values, not exceptions that jump around.

### Who is .nvm For?
- Systems programmers who want memory safety without a GC
- Python/Rust developers wanting a more ergonomic syntax
- Web backend developers needing high-performance services
- Embedded/IoT developers needing deterministic memory management
- Anyone who wants modern language features (pattern matching, async/await) with native performance

### The Vision
Novium aims to be the language you use when you need Python-level ergonomics with C-level performance. The `.nvm` layer is the compiled tier — it ahead-of-time compiles to native machine code via LLVM, giving you x86, ARM, RISC-V targets for free.

### Core Philosophy
> "You don't pay for what you don't use."
> — Bjarne Stroustrup (adapted)

If you don't use features like ownership, the compiler won't force them on you. If you do use them, you get safety guarantees.

---
## 2. Setting Up Your Environment

### Prerequisites
| OS | Requirements |
|----|-------------|
| **Windows + WSL (Recommended)** | WSL2 + Ubuntu, CMake, GCC/Clang |
| **Linux** | CMake, build-essential, LLVM dev headers |
| **macOS** | CMake, Xcode CLT, LLVM |
| **WSL (Easiest Start)** | `sudo apt install build-essential cmake git` |

### Installation Methods

#### Method 1: WSL + MinGW (Recommended for Windows)
```bash
# In WSL (Ubuntu terminal):
sudo apt update
sudo apt install build-essential cmake git

# Clone the repository
git clone https://github.com/yourname/novium-lang.git
cd novium-lang/Novium\ Compiler\ language\(.nvm)

# Build
mkdir -p build && cd build
cmake -DNOVIUM_OUTPUT=native ..
make

# Run
./novium examples/hello.nvm
```

#### Method 2: Direct CMake (Linux/macOS)
```bash
# Linux/macOS
cd Novium\ Compiler\ language\(.nvm)
mkdir -p build && cd build
cmake ..
make
```

#### Method 3: Pre-built Binaries (When Available)
```bash
# Download pre-built binary
curl -O https://example.com/novium-linux-x86_64
chmod +x novium-linux-x86_64
./novium-linux-x86_64 --version

# Run
./novium-linux-x86_64 examples/hello.nvm
```

### Verifying Your Installation
```bash
# Check the compiler is working
novium --version

# Check help
novium --help

# Tokenize a test file
novium --tokens examples/hello.nvm

# Print the AST
novium --ast examples/hello.nvm
```

### First Run Output
```powershell
PS C:\> .\build\novium.exe examples\hello.nvm
── AST for hello.nvm ──
│ fn greet(name: string) void:
│     print("Hello, ${name}! Welcome to Novium.")
│ 
│ fn add(a: int, b: int) int:
│     return a + b
│ 
│ fn double(x: int) int: return x * 2
│ 
│ fn main() void:
│     greet("World")
│     let result: int = add(10, 20)
│     let doubled: int = double(result)
│     print("10 + 20 = ${result}, doubled = ${doubled}")
```
---
## 2. Writing Your First .nvm Program

### Hello World (Minimal)
Create `hello.nvm`:
```novium
// ============================================================================
// hello.nvm — Hello World in Novium Compiled
// ============================================================================
// This is the first Novium program. It tests basic lexer functionality:
// keywords, identifiers, strings, operators, and indentation.

// Simple function
fn greet(name string) void:
    print("Hello, ${name}! Welcome to Novium.")
```

Run it:
```bash
novium hello.nvm
# Output: Hello, World! Welcome to Novium.
```

### Your Second Program: Functions & Return Values
Create `functions.nvm`:
```novium
// ============================================================================
// functions.nvm — Novium Functions Demo
// ============================================================================

// Function with return value
fn add(a: int, b: int) int:
    return a + b

// Inline function (curly brace variant)
fn double(x: int) int { return x * 2 }

// Main entry point
fn main() void:
    // Call functions
    let result: int = add(10, 20)
    let doubled: int = double(result)
    
    // Print with string interpolation
    print("10 + 20 = ${result}, doubled = ${doubled}")
```

Run:
```bash
novium functions.nvm
# Output: 10 + 20 = 30, doubled = 60
```

### Key Concepts Demonstrated
1. **`fn` keyword**: Defines a function
2. **Type annotations**: `: int`, `: string` — optional due to inference
3. **`return`**: Returns a value from a function
4. **`:` vs `=`**: Colons for function definitions, equals for assignments
5. **String interpolation**: `${variable}` syntax
6. **Type annotation on `let`**: `let result: int` — optional but recommended for clarity

### Your Third Program: Conditionals & Blocks
Create `conditionals.nvm`:
```novium
// ============================================================================
// conditionals.nvm — Novium Conditionals Demo
// ============================================================================

// Simple recursive fibonacci
fn fibonacci(n: int) int:
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

// Describes number using pattern matching
fn describes(n: int) string:
    match n:
        0 => "zero"
        1 => "one"
        _ => "number: ${n}"

// Main entry point
fn main() void:
    // Fibonacci
    let f10: int = fibonacci(10)
    print("fibonacci(10) = ${f10}")
    
    // Pattern matching
    let desc: string = describes(f10)
    print(desc)
    
    // If-else expression (blocks return values)
    let n: int = 5
    let category: string = if n < 0:
        "negative"
    elif n == 0:
        "zero"
    else:
        "positive"
    
    print("5 is ${category}")
```

Run:
```bash
novium conditionals.nvm
# Output: fibonacci(10) = 55
#         one
#         5 is positive
```

### Checkpoint: What You Should Know
- `fn` defines functions
- `if`/`elif`/`else` are expressions that return values
- `match` is exhaustive (must cover all cases with `_` catch-all)
- Type annotations are optional but recommended for clarity
- String interpolation uses `${expr}`
---
## 3. Lexical Structure

### Source File Layout
```novium
// Comments start with // and go to end of line
/* Multi-line comments /* nested */ also work */

// Pragmas and attributes go before functions
#[fast]  // Hint for compiler optimizations
#[pure]  // Function has no side effects
#[const] // Return value depends only on inputs

// Top-level definitions
fn greet(name: string) void:
    ...

// Can also have blank lines between definitions
```

### Case Sensitivity
- **Keywords are case-sensitive**: `fn`, `if`, `match`, `print`, `return`, `let`, `void`, `int`, `string`, `bool`
- **Identifiers are case-sensitive**: `greet` ≠ `Greet` ≠ `GREET`
- **String literals are case-preserving**: `"Hello"` ≠ `"hello"`

### Unicode Support
- **Identifiers**: Can contain Unicode letters, digits, underscores
  ```novium
  fn résumé() void:  // Valid!
  let naïve: string = "café";  // Valid!
  ```
- **String literals**: Full UTF-8 support
  ```novium
  let hello: string = "Hello, 世界!"  // Valid!
  ```
- **Comments**: Full Unicode support
  ```novium
  // 注释: 这是中文注释  // Valid!
  ```

### Keywords (Reserved Words)
These cannot be used as identifiers:
```
fn, if, else, elif, match, return, let, void, int, float, string, bool,
true, false, none, optional, own, async, await, import, export,
struct, interface, class, implements, extends, trait, enum,
#[get], #[post], #[put], #[delete], #[link], #[js_export]
```

### Literals

| Type | Example | Notes |
|------|---------|-------|
| **Integer** | `42`, `0`, `-5`, `0xFF` | Decimal, hex (0x prefix) |
| **Float** | `3.14`, `0.0`, `2.5e10` | Scientific notation supported |
| **String** | `"Hello"`, `"Line\nbreak"`, `r"raw"` | `\n`, `\t`, `\"` escaped; raw strings `r"..."` no escaping |
| **Boolean** | `true`, `false` | Keywords |
| **Optional/None** | `none`, `none?` | No `null` in this language! |

### Operators

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| **1 (highest)** | `()`, `[]`, `.` | Left-to-right |
| **2** | `−` (unary minus), `not` | Right-to-left |
| **3** | `*`, `/`, `%` | Left-to-right |
| **4** | `+`, `-` (binary) | Left-to-right |
| **5** | `<`, `>`, `<=`, `>=` | Left-to-right |
| **6** | `==`, `!=` | Left-to-right |
| **7 (lowest)** | `and`, `or` | Left-to-right |

### Precedence Examples
```novium
// These are equivalent:
let x: int = 1 + 2 * 3;      // 1 + 6 = 7
let y: int = (1 + 2) * 3;   // 3 * 3 = 9 (parentheses change precedence)

// Using operators with functions:
let z: int = fibonacci(10) + 5 * 2;  // fib(10) + 10

// Comparison chaining (valid in Novium):
let is_between: bool = 1 < x < 10;  // x > 1 AND x < 10
```

### Separators

| Separator | Usage |
|-----------|-------|
| `;` | Optional statement separator (Newline usually suffices) |
| `,` | Function arguments, tuple elements |
| `:` | Function type annotations, map keys |
| `{}` | Inline function blocks, struct/literal syntax |
| `[]` | Array literals, indexing |
| `()` | Function calls, condition groups |

### Newlines and Whitespace
- **Newlines serve as statement separators** (like Rust, unlike Python where they're required)
- **Indentation is optional** but strongly encouraged for readability
- **Colons `:` can start blocks** (Python-style) or be followed by braces (C-style)
- **Tab vs spaces**: Either is fine; be consistent

```novium
// These are ALL valid and equivalent:
fn add(a: int, b: int) int:
    return a + b

fn add(a: int, b: int) int
    return a + b

fn add(a int, b int) int: return a + b
```

### Advanced Lexical Feature: Raw String Literals
```novium
// Useful for regex, HTML, SQL, JSON without escaping
let html: string = r"<div>
    <p>Hello World</p>
</div>"

// Escape sequences NOT processed in raw strings
// \n is literally backslash-n, not newline
// Useful for patterns where you want backslashes preserved

// Regular strings still escape
let path: string = "C:\Users\novium\docs"  // \u is unicode escape
let escaped: string = "Line1\nLine2"  // \n is newline
```
---
## 4. Type System

### Type Inference (The Default)
Novium's default is **Static + Inferred**. You rarely need explicit types.
```novium
// These all infer `int`:
let a = 5          // int
let b = 3 + 2      // int (inferred from operands)
let c = fibonacci(10)  // int (from return type)

// These all infer `string`:
let d = "hello"      // string
let e = "Hello, " + name  // string (concatenation)

// These all infer `bool`:
let f = true  // bool
let g = (5 > 3)  // bool (comparison)
```

### Explicit Type Annotations
Add types when you want documentation, clarity, or to override inference.
```novium
// Explicit annotation
let a: int = 5

// Type annotation on function parameter
fn greet(name: string) void:
    print("Hello, ${name}!")

// Explicit return type
fn add(a: int, b: int) int:
    return a + b

// Optional type (no null!)
let x: optional int = none
let y: optional int = some(42)

// String? is alias for optional string
let maybe: string? = none
```

### Type System Features

#### 1. Optional Types (No Null Pointers!)
```novium
// These are VALID:
let x: optional int = none          // No value
let y: optional int = some(42)      // Has value (42)
let z: string? = none               // Alias for optional string

// These FORCE handling:
match y:
    some_value:
        print("Value is ${some_value}")
    none:
        print("No value!")

// Unsafe: accessing without check gives COMPILE ERROR
// print(y)  // Error: y is optional, must handle both cases
```

#### 2. Type Compatibility
```novium
// These work (automatic coercion where safe):
let a: int = 5
let b: float = a  // int -> float: 5.0 (safe widening)
let c: string = a  // Would ERROR: int -> string not automatic

// These require explicit conversion:
let d: string = int_to_string(a)  // Or: "${a}" / ${a}
```

#### 3. Type Errors the Compiler Catches
```novium
// � These give COMPILE ERRORS:
fn broken() void:
    let x: int = "hello"  // Type mismatch
    let result: int = "hello" + "world"  // Can't add strings to int
    let d: int = true  // bool to int
```

#### 4. Generic Functions (Advanced)
```novium
// Identity function (works for any type)
fn identity<T>(x: T) T:
    return x

// Usage:
let a: int = identity(5)      // T = int
let b: string = identity("hi")  // T = string
let c: bool = identity(true)  // T = bool
```

### Type System Design Decisions

| Decision | Rationale |
|----------|-----------|
| **No `null`** | `optional T` total safety; `none` is the only "no value" |
| **Inferred types** | Reduce verbosity while keeping safety |
| **Exhaustive match** | Catch bugs at compile time, not runtime |
| **Unboxed primitives** | Performance: int/float in registers, no heap |
| **`optional` vs `?`** | `optional` is full type; `?` is syntactic sugar |
---
## 5. Functions and Control Flow

### Function Definitions

#### Basic Syntax
```novium
// With colons + indentation (Python-style)
fn greet(name: string) void:
    print("Hello, ${name}!")

// With braces (C-style, inline)
fn double(x: int) int { return x * 2 }

// With type inference (no annotations needed)
fn add(a, b) int:  // Compiler infers a: int, b: int, return: int
    return a + b
```

#### Parameters and Arguments
```novium
// Multiple parameters
fn calculate_area(width: float, height: float) float:
    return width * height

// Parameters with default values (FastAPI-like)
fn greet(name: string = "World") string:
    return "Hello, ${name}!"

// Call with default
let msg: string = greet()  // "Hello, World!"
// Call with override
let msg2: string = greet("Novium")  // "Hello, Novium!"
```

#### Return Values
```novium
// Void function (no return value)
fn print_hello() void:
    print("Hello")
    // implicit return: none (ok for void)

// Function returning a value
fn add(a: int, b: int) int:
    return a + b  // Must return int

// Omit return for last expression (expression-body functions)
fn double(x: int) int: return x * 2

// Or just: fn square(x: int) int: x * x  // ERROR: must return int
// Correct: fn square(x: int) int: 
//     return x * x
```

#### Multiple Return Values (Not natively supported — use `optional T` or `tuple`)
```novium
// Instead of: fn divmod(a: int, b: int) (int, int):
// Use:
fn safe_divide(a: int, b: int) (int, optional int):
    if b == 0:
        return (0, none)  // quotient, remainder
    return (a / b, none)  // In Novium, simplify: just return quotient

// Better approach using `optional`:
fn divide(a: int, b: int) optional int:
    if b == 0:
        return none
    return a / b

// Usage:
let result: optional int = divide(10, 2)
if result is not none:
    print("10 / 2 = ${result}")
else:
    print("Division by zero!")
```

### Control Flow: `if`/`elif`/`else`
```novium
// Basic if-else (expression form)
let x: int = 5
let result: string = if x > 0:
    "positive"
elif x == 0:
    "zero"
else:
    "negative"

print(result)  // "positive"

// If without else
let is_positive: bool = if x > 0:
    true
// ok: result is bool (or omitted)

// If with complex conditions
let is_weekend: bool = true
let plan: string = if is_weekend and is_sunny:
    "go to beach"
elif is_weekend and not is_sunny:
    "read book"
else:
    "go to work"

print(plan)
```

### Control Flow: `match` / `switch`
```novium
// Full pattern matching
fn describe_number(n: int) string:
    match n:
        0 => "zero"
        1 => "one"
        2 => "two"
        3...7 => "small"  // Range pattern!
        _ => "large"  // Catch-all REQUIRED
```

#### Pattern Types Supported

| Pattern | Syntax | Meaning |
|---------|--------|-----------|
| **Literal** | `0`, `1`, `"hello"` | Match exact value |
| **Catch-all** | `_` | Always matches, binds nothing |
| **Bind** | `x` or `name =>` | Bind matched value to `x` |
| **Range** | `3...7` | Match if value in range (inclusive) |
| **Type Guard** | `s: string` | Match if value is type `string` |
| **Struct** | `Point { x, y }` | Match struct with field patterns |

#### Advanced Pattern Matching

##### 1. Range Patterns
```novium
fn season(month: int) string:
    match month:
        1...3 => "winter"   // Jan, Feb, Mar
        4...5 => "spring"   // Apr, May
        6...8 => "summer"   // Jun, Jul, Aug
        9...10 => "fall"    // Sep, Oct
        11...12 => "winter" // Nov, Dec
        _ => "invalid"
```

##### 2. Bind Patterns
```novium
fn first_char(s: string) string:
    match s:
        "" => "empty"
        first => "first char: ${first}"  // `first` bound to matched substring
```

##### 4. Enum/Struct Patterns (Advanced)
```novium
// Defining a sum type (enum-like)
struct Result:
    Ok value: int
    Err message: string

fn divide(a: int, b: int) Result:
    if b == 0:
        return Err("Cannot divide by zero")
    return Ok(a / b)

// Using match on the struct
fn safe_divide_demo() void:
    let result: Result = divide(10, 2)
    
    match result:
        Ok value:
            print("10 / 2 = ${value}")
        Err message:
            print("Error: ${message}")
```

### Match Exhaustiveness
The compiler **requires** exhaustive match coverage:
```novium
// � This gives COMPILE ERROR (non-exhaustive):
fn bad_match(n: int) string:
    match n:
        0 => "zero"
        1 => "one"

// ✅ This is OK (catch-all required):
fn good_match(n: int) string:
    match n:
        0 => "zero"
        1 => "one"
        _ => "other"  // REQUIRED: catches all other ints
```

---
## 6. Error Handling

### Error Values (Not Exceptions)
Novium uses **error values** returned from functions, similar to Rust's `Result<T, E>` or Go's error handling.
```novium
// Function that can fail
fn divide(a: float, b: float) optional float:
    if b == 0.0:
        return none  // Error: division by zero
    return a / b  // Success

// Usage
let result: optional float = divide(10.0, 2.0)
if result is not none:
    print("10 / 2 = ${result}")  // 5.0
else:
    print("Cannot divide by zero!")
```

### `#[error]` Annotated Functions
```novium
// Mark functions that can return error values
#[error("Division by zero error")]
fn safe_divide(a: float, b: float) optional float:
    if b == 0.0:
        return none
    return a / b

// Usage same as above
```

### Error Payload Types
```novium
// Error can carry data
#[error("File not found: {path}")]
fn find_file(path: string) optional string:
    if not file_exists(path):
        return none  // Carries: "File not found: /path/to/file"
    return some(open(path).read())

// Usage:
let content: optional string = find_file("/etc/novium.conf")
match content:
    some text:
        print("File contents: ${text}")
    Err msg:
        print("Failed to find file: ${msg}")
```

### Error Aggregation
```novium
fn validate_form(data: map[string, string]) optional list[string]:
    errors: list[string] = []
    
    if not has_required_field(data, "name"):
        errors.append("name is required")
    if not has_required_field(data, "email"):
        errors.append("email is required")
    if not is_valid_email(data.get("email")):
        errors.append("email format invalid")
    
    if errors.is_empty():
        return none  // No errors
    return some(errors)  // Return all errors found

// Usage:
let errors: optional list[string] = validate_form(user_input)
match errors:
    some err_list:
        for err in err_list:
            print("Fix: ${err}")
    none:
        // Form is valid!
```

### Pattern Matching on Errors
```novium
// Match on optional/error results
let result: optional int = some risky_operation()

match result:
    some value:
        print("Success: ${value}")
    none:
        print("Operation failed - handle error here")
```

### Error vs Exception Comparison

| Aspect | Novium Error Values | Exceptions (Java/Python) |
|--------|-------------------|------------------------|
| **Control flow** | Explicit, at call site | Implicit, jumps up stack |
| **Must handle** | Yes (compile error if forgotten) | No (can be caught or ignored) |
| **Performance** | Zero cost when no error | Stack unwinding overhead |
| **Error data** | Carried in return type | Carried in exception object |
| **Multiple errors** | Return all as list/struct | Typically one exception at a time |
| **Call graph visibility** | Yes (in function signature) | Often hidden (try/catch scattered) |

### Recoverable vs unrecoverable Errors
```novium
// Recoverable: expected error conditions
fn open_file(path: string) optional file:
    if file_exists(path):
        return some(open(path))
    return none  // File not found - recoverable

// Unrecoverable: programming errors  
fn assert(condition: bool, message: string) void:
    if not condition:
        #panic(message)  // Program termination
        // Or: return error value if designed that way
```

---
## 7. Memory Management and Ownership

### The Ownership Model (Rust-Inspired)
Novium uses a **statically checked ownership system** to manage memory without a garbage collector.

### Ownership Rules (3 Rules, Adapted from Rust)
1. **Each value has a single owner**: There's a variable that's the "owner" of the value.
2. **When the owner goes out of scope, the value is dropped**: Automatic cleanup (RAII).
3. **You can't have multiple owners of the same data**: Move semantics.

### Moving vs Copying
```novium
// Primitives copy (int, float, bool)
let a: int = 5
let b: int = a  // OK: a copies to b; both are 5
print(${a})  // 5
print(${b})  // 5

// Ownership transfer for complex types
let s1: string = "Hello"
let s2: string = s1  // ERROR: value moved, not copied
// print(${s1})  // COMPILE ERROR: s1 no longer valid

// Correct: explicit clone/copy if needed
let s1: string = "Hello"
let s2: string = s1.clone()  // OK: explicit copy
// OR use borrow:

// Borrowing via references
let s1: string = "Hello"
let s2: &string = &s1  // Borrow: s2 is a reference to s1
print(${*s2})  // Dereference: "Hello"
// s1 still valid here because borrow is within scope

// 'own' keyword transfers ownership
fn process(own data: string) void:
    print("Processing: ${data}")
    // After this function, data is dropped; caller cannot use it
```

### The Borrow Checker
The compiler enforces borrowing rules at compile time.
```novium
fn calculate(data: string) string:
    let ref: &string = &data  // Borrow data
    // ref lives only within this function scope
    return format!("processed: ${ref}")

fn main() void:
    let ownership: string = "test"
    // let borrowed: &string = &ownership  // ERROR: would outlive scope
    // But this works:
    let result: string = calculate(ownership)  // ownership moved INTO function
    // After this line, ownership is moved; can't use it again
```

### The `own` Keyword
Transfers ownership explicitly.
```novium
// Without own: compiler may allow use-after-move
fn maybe_use(x: string) void:
    // ...

fn test() void:
    let s: string = "data"
    maybe_use(own s)  // s is transferred; compiler tracks this
    // print(${s})  // ERROR: s moved into maybe_use
```

### Automatic Drop (RAII)
```novium
// Resources auto-cleaned when scope exits:
// - Strings, vectors, arrays
// - File handles (if using std::file)
// - Network connections

// Example with implicit file handling (simulated):
fn read_file(path: string) string:
    let file: file_handle = open(path)  // Auto-closable
    let content: string = read(file)
    // file.close() called automatically when `file` goes out of scope
    return content
// No need for explicit try/finally - compiler inserts cleanup

// Explicit drop (rarely needed)
fn explicit_drop() void:
    let s: string = "resource"
    // .drop() exists but usually unnecessary
    // s.drop()  // Compiler may auto-insert
// When s goes out of scope at end of function, it's automatically dropped
```

### Memory Layout Optimizations

#### Unboxed Primitives
```novium
// int/float are unboxed: stored in registers, no heap allocation
let a: int = 42       // 4 bytes in register, not on heap
let b: float = 3.14   // 8 bytes in register

// Operations are direct CPU instructions:
// a + b → single ADD instruction, no memory indirection

// Size hints (compile-time):
// int: machine word size (32 or 64 bit depending on target)
// float: 64-bit IEEE 754
// bool: 1 byte (or optimized as tag)
// string: pointer + length (16 bytes typically)
```

#### Optional as Tagged Immediate
```novium
// optional int is NOT a pointer — it's a tagged immediate:
// - some(42)  →  tag 01 + 42 (fits in machine word)
// - none      →  tag 00

// No heap allocation for optional int!
// No NULL pointer dereference possible!

let x: optional int = some(42)
let y: optional int = none

// Both fit in a single register
// Pattern match is branch on tags, not heap dereference
```

#### String Representation
```novium
// String is a struct: { pointer to data, length }
// - Length is O(1) accessible via .len (compiler intrinsic)
// - No nul-termination needed (unlike C)
// - Bounds-checked access via .at(i) or [i]

let s: string = "Hello"
// s.len → 5 (compile-time or runtime constant)

// No buffer overflow possible: length is always known
// s[100] → compile error if 100 >= len, or runtime check with optional return
```

### Common Memory Patterns

#### 1. Stack-Accumulation Pattern
```novium
fn sum_range(n: int) int:
    let total: int = 0
    let i: int = 0
    while i < n:
        total = total + i
        i = i + 1
    return total
    // All locals (total, i) on stack; auto-cleaned on return
```

#### 2. Heap Allocation (When Needed)
```novium
// Vectors/arrays may heap-allocate when size not known at compile time
let v: vector[int] = [1, 2, 3, 4, 5]
// v is a struct: { pointer, length, capacity }
// Heap-allocated array of 5 ints

// But: size known at compile time → stack allocation
let fixed: [int; 5] = [1, 2, 3, 4, 5]
// fixed is on stack; no heap overhead
```

#### 3. Ownership Transfer Pattern
```novium
fn consume(s: string) void:
    // s is consumed (dropped at end of fn)
    print("Consumed: ${s}")

fn test() void:
    let s: string = "data"
    consume(own s)  // s moved; can't use after
    // The 'own' keyword explicitly transfers ownership
```

### Lifetime Elision (Compiler Helpers)
```novium
// The compiler can elide lifetime annotations in common cases:

// Case 1: simple borrow
fn first_char(s: &string) char:
    s.chars().next().unwrap()

// Case 2: return type elision
fn fn_returns_borrow<'a>(s: &string) -> &'a string {
    s  // Returning the borrow
}

// Case 3: both arguments
fn choose_x_or_y(x: &string, y: &string) -> &string {
    if x.len() > y.len() {
        x  // returning first argument
    } else {
        y  // returning second argument
    }
}
```

---
## 8. Pattern Matching

### `match` Expression Syntax
```novium
// Basic match
fn describe(n: int) string:
    match n:
        0 => "zero"
        1 => "one"
        _ => "other"

// Match is an expression (returns a value)
let result: string = match 1:
    0 => "zero"
    1 => "one"
    _ => "other"  // result = "one"
```

### Pattern Types

| Pattern | Syntax | Meaning |
|---------|--------|-----------|
| **Literal** | `0`, `1`, `"hello"` | Match exact value |
| **Catch-all** | `_` | Always matches, binds nothing |
| **Bind** | `x` or `name =>` | Bind matched value to `x` |
| **Range** | `3...7` | Match if value in range (inclusive) |
| **Type Guard** | `s: string` | Match if value is type `string` |
| **Struct** | `Point { x, y }` | Match struct with field patterns |

### Advanced Pattern Matching

#### 1. Range Patterns
```novium
fn season(month: int) string:
    match month:
        1...3 => "winter"   // Jan, Feb, Mar
        4...5 => "spring"   // Apr, May
        6...8 => "summer"   // Jun, Jul, Aug
        9...10 => "fall"    // Sep, Oct
        11...12 => "winter" // Nov, Dec
        _ => "invalid"
```

##### 2. Bind Patterns
```novium
fn first_char(s: string) string:
    match s:
        "" => "empty"
        first => "first char: ${first}"  // `first` bound to matched substring
```

##### 4. Enum/Struct Patterns (Advanced)
```novium
// Defining a sum type (enum-like)
struct Result:
    Ok value: int
    Err message: string

fn divide(a: int, b: int) Result:
    if b == 0:
        return Err("Cannot divide by zero")
    return Ok(a / b)

// Using match on the struct
fn safe_divide_demo() void:
    let result: Result = divide(10, 2)
    
    match result:
        Ok value:
            print("10 / 2 = ${value}")
        Err message:
            print("Error: ${message}")
```

### Match Exhaustiveness
The compiler **requires** exhaustive match coverage:
```novium
// � This gives COMPILE ERROR (non-exhaustive):
fn bad_match(n: int) string:
    match n:
        0 => "zero"
        1 => "one"

// ✅ This is OK (catch-all required):
fn good_match(n: int) string:
    match n:
        0 => "zero"
        1 => "one"
        _ => "other"  // REQUIRED: catches all other ints
```
---
## 9. Error Handling

### Error Values (Not Exceptions)
Novium uses **error values** returned from functions, similar to Rust's `Result<T, E>` or Go's error handling.
```novium
// Function that can fail
fn divide(a: float, b: float) optional float:
    if b == 0.0:
        return none  // Error: division by zero
    return a / b  // Success

// Usage
let result: optional float = divide(10.0, 2.0)
if result is not none:
    print("10 / 2 = ${result}")  // 5.0
else:
    print("Cannot divide by zero!")
```

### `#[error]` Annotated Functions
```novium
// Mark functions that can return error values
#[error("Division by zero error")]
fn safe_divide(a: float, b: float) optional float:
    if b == 0.0:
        return none
    return a / b

// Usage same as above
```

### Error Payload Types
```novium
// Error can carry data
#[error("File not found: {path}")]
fn find_file(path: string) optional string:
    if not file_exists(path):
        return none  // Carries: "File not found: /path/to/file"
    return some(open(path).read())

// Usage:
let content: optional string = find_file("/etc/novium.conf")
match content:
    some text:
        print("File contents: ${text}")
    Err msg:
        print("Failed to find file: ${msg}")
```

### Error Aggregation
```novium
fn validate_form(data: map[string, string]) optional list[string]:
    errors: list[string] = []
    
    if not has_required_field(data, "name"):
        errors.append("name is required")
    if not has_required_field(data, "email"):
        errors.append("email is required")
    if not is_valid_email(data.get("email")):
        errors.append("email format invalid")
    
    if errors.is_empty():
        return none  // No errors
    return some(errors)  // Return all errors found

// Usage:
let errors: optional list[string] = validate_form(user_input)
match errors:
    some err_list:
        for err in err_list:
            print("Fix: ${err}")
    none:
        // Form is valid!
```

### Pattern Matching on Errors
```novium
// Match on optional/error results
let result: optional int = some risky_operation()

match result:
    some value:
        print("Success: ${value}")
    none:
        print("Operation failed - handle error here")
```

### Error vs Exception Comparison

| Aspect | Novium Error Values | Exceptions (Java/Python) |
|--------|-------------------|------------------------|
| **Control flow** | Explicit, at call site | Implicit, jumps up stack |
| **Must handle** | Yes (compile error if forgotten) | No (can be caught or ignored) |
| **Performance** | Zero cost when no error | Stack unwinding overhead |
| **Error data** | Carried in return type | Carried in exception object |
| **Multiple errors** | Return all as list/struct | Typically one exception at a time |
| **Call graph visibility** | Yes (in function signature) | Often hidden (try/catch scattered) |

### Recoverable vs unrecoverable Errors
```novium
// Recoverable: expected error conditions
fn open_file(path: string) optional file:
    if file_exists(path):
        return some(open(path))
    return none  // File not found - recoverable

// Unrecoverable: programming errors  
fn assert(condition: bool, message: string) void:
    if not condition:
        #panic(message)  // Program termination
        // Or: return error value if designed that way
```
---
## 10. Concurrency: Goroutines and Async/Await

### Concurrency Model
Novium supports **two concurrency models** as chosen in the design:

1. **Goroutines + Channels**: Lightweight green threads (like Go)
2. **async/await**: Cooperative concurrency (like Rust/JS)

Both are supported, giving developers choice.

### Goroutines (Lightweight Threads)
```novium
// Spawn a goroutine
go fn:
    let result: int = fibonacci(30)
    print("Background fib: ${result}")

// Channel communication (like Go)
chan: channel[int] = new_channel[int]()

go fn:
    let result: int = fibonacci(30)
    chan.send(result)  // Send to channel

// Receive
let value: int = chan.receive()  // Blocks until data available

// Non-blocking receive (with timeout)
let maybe_value: optional int = chan.try_receive()
```

### Async/Await (Cooperative Concurrency)
```novium
// Define async function
async fn fetch_data() string:
    // Suspend at await points
    let result: string = await http_get("https://api.example.com/data")
    return result

// Call async function
async fn main() void:
    // Start async operation
    let data_task: async string = fetch_data()
    
    // Do other work while waiting
    print("Fetching data...")
    
    // Await result
    let data: string = await data_task
    print("Got data: ${data.length()} bytes")
```

### async/await Details
```novium
// Multiple await points
async fn complex_calculation(x: int, y: int) string:
    let a: int = await step_one(x)
    let b: int = await step_two(y)
    let c: int = await step_three(a, b)
    return "Result: ${c}"

// Each await yields to the runtime, allowing other tasks to proceed
// No OS thread per coroutine (unlike traditional threading)

// Structured concurrency: wait for all tasks
async fn run_all() void:
    let task1: async int = compute(1)
    let task2: async int = compute(2)
    let task3: async int = compute(3)
    
    // Wait for ALL tasks; results collected
    let results: list[int] = await_all(task1, task2, task3)
    print("Results: ${results}")  // [compute(1), compute(2), compute(3)]
```

### Channels (Communication Between Goroutines)
```novium
// Create channel
chan: channel[int] = new_channel[int]()

// Producer goroutine
go fn producer() void:
    for i in 0..5:
        chan.send(i)
    chan.close()  // Signal no more data

// Consumer goroutine
go fn consumer() void:
    while true:
        let maybe: optional int = chan.try_receive()
        if maybe is some:
            print(${maybe})  // 0 1 2 3 4
        else:
            // Channel closed and empty
            break

// Run both
async fn main() void:
    go producer()
    go consumer()
```

### Timeout and Cancellation
```novium
// Timeout on await
let result: optional string = await_with_timeout(http_get(url), 5000)  // 5 second timeout
if result is some:
    print(${result})
else:
    print("Request timed out")

// Cancel a task
async fn cancellable_task() void:
    let cancellation_token: cancellation_token = get_token()
    
    if await_with_timeout(some_operation(cancellation_token), 10000):
        // Task completed before timeout
        print("Completed")
    else:
        // Timeout occurred; task was cancelled
        print("Timed out, cleaning up...")
```

### Parallel and Parallelism
```novium
// Parallel map (like PMap)
let numbers: list[int] = [1, 2, 3, 4, 5]
let squares: list[int] = parallel_map(fn(x: int) int: x * x, numbers)
print(${squares})  // [1, 4, 9, 16, 25]

// Parallel filter
let evens: list[int] = parallel_filter(fn(x: int) bool: x % 2 == 0, numbers)
print(${evens})  // [2, 4]

// CPU-bound parallel work
let result: int = parallel_reduce(fn(acc: int, i: int) int: acc + i, 0, (0..1000000))
print(${result})  // 499999500000
```

### Runtime Configuration
```novium
// Set goroutine count (goroutine model)
concurrency_config: int = 4  // Run up to 4 goroutines concurrently

// Set async task pool size
async_config: int = 8  // Max 8 concurrent async tasks

// These can be set via command line or config file
// novium run --concurrency 8 program.nvi
```

---
## 11. API Endpoints and OpenAPI Generation

### API Definition Annotations
Novium supports FastAPI-like endpoint definitions that generate OpenAPI 3.1 specifications and client stubs.
```novium
// Annotate functions as API endpoints
#[get("/fibonacci/{n}")]
fn fib_get(n: int) int:
    #response(200, {"fibonacci": int})
    #response(400, {"detail": "n must be non-negative"})
    #response(422, {"detail": "Validation error"})
    #tags(["Math", "Fibonacci"])
    #description("Compute the nth Fibonacci number")
    #default(10)  // Optional parameter default
    if n < 0:
        return -1
    let result: int = fibonacci(n)
    return result

#[post("/echo")]
fn echo_post(message: string) string:
    #schema({
        "type": "string",
        "min_length": 1,
        "example": "Hello Novium"
    })
    return message

#[put("/update/{id}")]
fn update_put(id: int, completed: bool) string:
    #include_in_schema(True)
    return "Item ${id} set to ${completed}"

#[delete("/item/{id}")]
fn delete_delete(id: int) string:
    #response(204, "No content")
    return ""
```

### Generated OpenAPI 3.1 Spec
The annotations generate a machine-readable specification:
```yaml
# OpenAPI 3.1.0 Generated from Novium
openapi: "3.1.0"
info:
  title: Novium API
  version: "0.1.0"
  description: "API generated from Novium #[get]/#[post] annotations"

paths:
  /fibonacci/{n}:
    get:
      summary: Compute the nth Fibonacci number
      tags: ["Math", "Fibonacci"]
      parameters:
        - name: n
          in: path
          required: true
          schema:
            type: integer
            minimum: 0
      responses:
        "200":
          description: Successful response
          content:
            application/json:
              schema:
                type: object
                properties:
                  fibonacci:
                    type: integer
        "400":
          description: n must be non-negative

  /echo:
    post:
      summary: Echo back the message
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: string
              minLength: 1
      responses:
        "200":
          description: Echoed message
          content:
            application/json:
              schema:
                type: string

components:
  schemas: ...
```

### Client Stubs Generation
Automatically generate clients for multiple languages:
```bash
# Generate Python client
novium generate-client --language python --output ./python-client

# Generate JavaScript/TypeScript client
novium generate-client --language javascript --output ./js-client

# Generate Go client
novium generate-client --language go --output ./go-client

# Generate Rust client
novium generate-client --language rust --output ./rust-client
```

#### Python Client (httpx-based)
```python
# Generated code
from novium.client import NoviumClient

client = NoviumClient(base_url="http://localhost:8000")

# Synchronous call
response = client.fib_get(10)
print(response.json())  # {"fibonacci": 55}

# Asynchronous call
import asyncio

async def main():
    result = await client.fib_async(10)
    print(result.json())  # {"status": "completed", "fibonacci": 55}

    # Batch POST
    batch = client.fib_post(numbers=[1, 5, 10])
    print(batch.json())  # {"results": [1, 5, 55]}

asyncio.run(main())
```

#### JavaScript/fetch Client
```javascript
// Generated code
import novium from './novium-client.js';

// Synchronous/await call
const greeting = await novium.fibGet(10);
console.log(greeting);  // "5"

// Batch POST
const batch = await novium.fibPost({ numbers: [1, 5, 10] });
console.log(batch);  // [1, 5, 55]

// Health check
const health = await novium.healthCheck();
console.log(health);  // "OK"
```

#### Go Client
```go
// Generated code
import "github.com/novium/client-go"

func main() {
    client := novium.NewClient("http://localhost:8000")
    
    fib, _ := client.FibGet(context.Background(), 10)
    fmt.Println(fib)  // 55
    
    result, _ := client.FibPost(context.Background(), map[string]interface{}{
        "numbers": []interface{}{1, 5, 10},
    })
    fmt.Println(result)  // [1, 5, 55]
}
```

#### Rust Client
```rust
// Generated code
use novium_client::client::NoviumClient;

#[tokio::main]
async fn main() {
    let client = NoviumClient::new("http://localhost:8000");
    
    let fib = client.fib_get(10).await.unwrap();
    println!("Fibonacci(10) = {}", fib);
    
    let batch = client.fib_post(vec![1, 5, 10]).await.unwrap();
    println!("Batch result: {:?}", batch);
}
```

### API Documentation Generation
```bash
# Generate HTML docs
novium generate-docs --output ./docs/api-documentation.html

# Generate Markdown docs
novium generate-docs --format markdown --output ./docs/api-reference.md

# Serve locally for development
novium serve-docs --port 8000
```

### API Versioning
```novium
// Version the API via path prefix
#[get("/v1/fibonacci/{n}")]
fn v1_fib_get(n: int) int: ...

#[get("/v2/fibonacci/{n}")]
fn v2_fib_get(n: int) int:  // Different implementation
    ...

// Client would specify version in base URL:
// http://localhost:8000/v1/... or http://localhost:8000/v2/...
```

### Middleware/Hooks (Future)
```novium
// These would be added in future versions
#[middleware("auth")]
fn protected_endpoint() void:
    // Auth check handled by middleware

#[before_request]
fn log_request() void:
    // Log every request

#[after_response]
fn add_headers() void:
    // Add headers to every response
```
---
## 12. C FFI and External Libraries

### External C Library Linking
```novium
// Link C math library
#[link("m")]
extern "C" {
    fn cos(angle: float) float;
    fn sin(angle: float) float;
}

// Link custom application library
#[link("mylib")]
extern "C" {
    fn app_init() int;
    fn app_shutdown() void;
}

// Standard C library (usually implicit)
extern "C" {
    fn printf(format: string, ...) int;
    fn puts(format: string) int;
}
```

### Using Linked Functions
```novium
fn main() void:
    // Call C math functions
    let angle: float = 3.14159 / 2.0
    let result: float = cos(angle)
    print("cos(pi/2) = ${result}")  // ~0.0
    
    // Call init/shutdown
    let init_result: int = app_init()
    print("app_init returned: ${init_result}")
    app_shutdown()
```

### Type Conversions (C ↔ Novium)
| Novium Type | C Type | Conversion |
|-------------|--------|------------|
| `int` | `int` / `int*` | Direct (same size) |
| `float` | `float` / `double` | Direct (float → double widening) |
| `string` | `const char*` | Novium string → UTF-8 null-terminated copy |
| `optional T` | `T / NULL` | `none` → `NULL`, `some(val)` → `val` pointer |

```novium
// String to C string
let c_str: string = "Hello C"
let c_ptr: *const char = c_str.to_c_str()  // Implicit conversion or explicit

// Using with printf
printf("${c_ptr}\n")  // "Hello C"

// Array/slice to C array
let numbers: vector[int] = [1, 2, 3]
let c_array: *const int = numbers.to_c_array()  // Pointer to data + length info
```

### Network Libraries (libcurl-style)
```novium
// Link network library (simulated)
#[link("curl")]
extern "C" {
    fn easy_init() *curl_easy
    fn easy_setopt(handle: *curl_easy, option: int, value: any) int
    fn easy_perform(handle: *curl_easy) int
    fn easy_cleanup(handle: *curl_easy) void
}

// Usage
fn http_get(url: string) string:
    let handle: *curl_easy = easy_init()
    easy_setopt(handle, 0, url)  // CURLOPT_URL = 0 (simplified)
    let result_code: int = easy_perform(handle)
    if result_code == 0:  // CURLE_OK
        let body: string = // get response body
        easy_cleanup(handle)
        return body
    return ""
```

### Safe FFI Boundary
The Novium FFI system ensures:
1. **Type safety at boundary**: Types checked on both sides
2. **No dangling pointers**: Ownership transferred or borrowed explicitly
3. **String safety**: Novium strings are length-prefixed; C strings are null-terminated copies
4. **Memory lifecycle**: Caller or callee owns memory; documented in FFI docs

### Inline C (Future/Experimental)
```novium
// May allow inline C in future versions
// fn inline_c(c_code: string) string: ...

// Or via embedded AST:
#[embedded_c]
fn compute() int:
    // C code embedded at compile time
    // return 42;
```
---
## 13. Advanced Features

### 1. Macros (Compile-time Transformations)
```novium
// Define a macro
// (Macro system coming in later sprint)
// #macro debug_log(x) 
// #  log("Debug: ${x}")
// #end

// Usage (when available):
// debug_log(calculation_result)
```

### 2. Derive Macros (Automatic Trait Implementation)
```novium
// When macro system available:
#[derive(Clone, Debug, PartialEq)]
struct Point {
    x: int
    y: int
}

// Automatically gets:
// - Clone implementation
// - Debug fmt implementation  
// - PartialEq equality comparison

// Usage:
let p1: Point = Point { x: 1, y: 2 }
let p2: Point = Point { x: 1, y: 2 }
let equal: bool = p1 == p2  // Uses derived PartialEq
print(${p1})  // "Point { x: 1, y: 2 }"
```

### 3. Metaprogramming (Compile-time Computation)
```novium
// Fibonacci at compile-time (when meta features available)
// (Compile-time execution coming in later sprint)

// Would allow:
// @fib(10) → 55 at compile time
// Useful for: constant arrays, optimized lookups, template metaprogramming
```

### 4. Generic/Parameterized Types
```novium
// Generic function (already seen)
fn identity<T>(x: T) T:
    return x

// Generic struct
// (When macro/system available):
// struct Box<T> {
//     value: T
// }

// Generic function usage:
let a: int = identity(5)       // T = int
let b: string = identity("hi") // T = string
let c: bool = identity(true)  // T = bool
```

### 5. Operator Overloading (Future)
```novium
// When operator overloading is supported:
// Define + for custom types
#[overload("+")]
impl PlusForPoint for Point {
    fn add(self, other: Point) Point:
        return Point { x: self.x + other.x, y: self.y + other.y }
}

// Usage:
let p1: Point = Point { x: 1, y: 2 }
let p2: Point = Point { x: 3, y: 4 }
let sum: Point = p1 + p2  // Point { x: 4, y: 6 }
print(${sum})
```

### 5. Async Generator (Yield in async)
```novium
// When async generators supported:
async fn fib_sequence(max: int) yield int:
    let a: int = 0
    let b: int = 1
    let i: int = 0
    while i < max:
        yield a
        let temp: int = a + b
        a = b
        b = temp
        i = i + 1

// Usage:
async fn main() void:
    let gen: async_gen int = fib_sequence(5)
    for value in gen:
        print(${value})  // 0 1 1 2 3
```
---
## 14. Building and Running Programs

### Compilation Targets

| Target | CMake Flag | Output |
|--------|-----------|--------|
| **Native executable** | (default) | `novium` binary |
| **Wasm + JS** | ` -DNOVIUM_OUTPUT=wasm` | `novium.wasm` + `novium.js` glue |
| **Static library** | ` -DBUILD_LIB=ON` | `libnovium_lib.a` |
| **Object files** | ` -DCMAKE_BUILD_TYPE=Release` | `.o` files |

### Building Commands

#### Native Build (Default)
```bash
cd Novium\ Compiler\ language\(.nvm)
mkdir -p build && cd build
cmake ..
make
# Output: build/novium (binary)
```

#### Wasm Build (Web)
```bash
cd Novium\ Compiler\ language\(.nvm)
mkdir -p build && cd build
cmake -DNOVIUM_OUTPUT=wasm ..
make
# Output: build/novium.wasm, build/novium.js (glue)
```

#### Library Build
```bash
cd Novium\ Compiler\ language\(.nvm)
mkdir -p build && cd build
cmake -DBUILD_LIB=ON ..
make
# Output: build/libnovium_lib.a
```

### Running Executables
```bash
# Run a .nvm file directly
novium examples/hello.nvm

# Or run the binary
./build/novium examples/hello.nvm

# With arguments (if program accepts them)
novium examples/fibonacci.nvm 10
```

### Running Test Suite
```bash
# Build and run tests
cd Novium\ Compiler\ language\(.nvm)
mkdir -p build && cd build
cmake ..
make          # Build all targets including tests
make test     # Run the test suite

# Or via ctest
ctest --test-dir ./build
```

### IDE Integration

#### VS Code
```json
// .vscode/settings.json
{
    "novium.languageServer": "true",
    "novium.path": "./build/novium"
}
```

#### Neovim
```lua
-- Init just the telescope-based Novium support
require('novium').setup({
    compiler_path = '/path/to/build/novium',
    auto_type_check = true,
})
```

### Packaging and Distribution
```bash
# Build for distribution
novium build --release --target wasm -o dist/novium-app.wasm

# Create runtime package
novium package --include-runtime -o novium-runtime-v0.1.0.tar.gz

# Version the compiled binary
novium version --show
# Output: Novium Compiler v0.2.0 (Sprint 2: Parser & AST)
```

### Debugging
```bash
# Enable debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with debug flags
novium --debug examples/fibonacci.nvm  # If debug mode supported

# Print intermediate representations
novium --ast examples/fibonacci.nvm   # AST
novium --tokens examples/fibonacci.nvm # Tokens
```
---
## 15. Tooling and Ecosystem

### Package Manager (Coming Later)
```bash
# When package manager available:
novium pkg install novium-web-framework
novium pkg list
novium pkg search "database"
novium pkg update
novium pkg remove
```

### Standard Library (Growing)

| Module | Purpose | Example |
|--------|---------|---------|
| `std/string` | String operations | `std.string.trim(" hello ") ` |
| `std/math` | Math functions | `std.math.max(5, 10)` |
| `std/vector` | Dynamic arrays | `std.vector.push([1,2,3], 4)` |
| `std/fs` | File system | `std.fs.read_file("test.txt")` |
| `std/time` | Time/date | `std.time.now()` |
| `std/rand` | Random numbers | `std.rand.int(100)` |

### Build Systems (Beyond CMake)
```bash
# When alternative build systems supported:
# ninja build
# meson build
# Bazel build

# Or native build scripts:
# ./novium build --help
```

### CI/CD Integration
```bash
# GitHub Actions example
name: Novium CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build Novium
        run: |
          cd Novium\ Compiler\ language\(.nvm)
          mkdir -p build && cd build
          cmake ..
          make
      - name: Run tests
        run: |
          ./build/lexer_test.exe
          ./build/parser_test.exe
      - name: Lint
        run: |
          ./build/novium.exe --lint examples/*.nvm
```

### Documentation Generator
```bash
# Generate API docs from #[get]/#[post] annotations
novium generate-docs --input examples/fibonacci.nvm --output ./docs/api.html

# Generate type documentation
novium type-docs --show std.core  # Show core module types

# Generate usage examples
novium generate-examples --from fibonacci  # Create example programs
```

### Checking for Updates
```bash
novium self-update  # Update compiler to latest version
novium --check-for-updates  # Check if newer version available
```

### Community and Support

| Resource | Description |
|----------|-------------|
| **GitHub Discussions** | Ask questions, share ideas |
| **GitHub Issues** | Bug reports, feature requests |
| **Documentation** | `docs/` directory in repo |
| **Examples** | `examples/` directory with working programs |
| **Chat/Slack** | Community chat (when set up) |
---
## 16. Migration Guides

### From Python to .nvm
```python
# Python
def add(a, b):
    return a + b

result = add(5, 3)
print(f"5 + 3 = {result}")
```

```novium
// Novium equivalent
fn add(a: int, b: int) int:
    return a + b

fn main() void:
    let result: int = add(5, 3)
    print("5 + 3 = ${result}")
```

### From C++ to .nvm
```cpp
// C++
int add(int a, int b) {
    return a + b;
}

// Novium (safer, with ownership)
fn add(a: int, b: int) int:
    return a + b

// No memory leaks; no undefined behavior; type-safe
```

### From Rust to .nvm
```rust
// Rust
fn add(a: i32, b: i32) -> i32 {
    a + b
}

// Novium equivalent (different syntax, similar semantics)
fn add(a: int, b: int) int:
    return a + b
```

### From JavaScript/TypeScript to .nvi
```javascript
// JavaScript
function add(a, b) {
    return a + b;
}

// Novium .nvi (Python-like scripting)
def add(a: int, b: int) int:
    return a + b
```

### From Java/TypeScript to .nvw
```javascript
// JS client generation
novium generate-client --language javascript --output ./js-client
// Then import and use: import novium from './novium-client.js'
```

---
## 17. Best Practices

### Code Organization
```novium
// good.nvm — Well-organized Novium program

// 1. Imports (if any)
import "utils.nvm"

// 2. Constants
const MAX_USERS: int = 1000
const PI: float = 3.14159

// 3. Utility functions
fn format_name(name: string) string:
    return "Hello, ${name}!"

// 3. Core functions
fn calculate_interest(principal: float, rate: float, years: int) float:
    return principal * (1 + rate) ^ years

// 4. API endpoints
#[get("/health")]
fn health_check() string:
    return "OK"

// 5. Main
fn main() void:
    health_check()
```

### Naming Conventions

| Convention | Example |
|------------|---------|
| **Functions**: `lowercase_with_underscores` | `fn calculate_area() ` |
| **Variables**: `lowercase_with_underscores` | `let max_users: int ` |
| **Constants**: `UPPERCASE_WITH_UNDERSCORES` | `const MAX_USERS` |
| **Types/Structs**: `PascalCase` | `struct Point { x, y } ` |
| **Enums**: `PascalCase` | `enum Status { Active, Inactive } ` |
| **API endpoints**: `lowercase-with-hyphens` | `#[get("/health-check")] ` |

### Commenting Documentation
```novium
// Good comments explain WHY, not WHAT
// � Bad: // increment i by 1
// ✅ Good: // Move to next iteration of the loop

// Document function purpose
/// Calculates the nth Fibonacci number
/// 
/// Uses recursive approach with depth guard to prevent stack overflow.
/// 
/// @param n The position in the Fibonacci sequence (0-indexed)
/// @returns The nth Fibonacci number, or -1 if n exceeds recursion depth
fn fibonacci(n: int) int:
    ...
```

### Error Handling Best Practices
```novium
// ✅ Good: Handle errors at call site
let result: optional int = divide(10, 0)
if result is none:
    print("Cannot divide by zero - using default value 0")
    let default: int = 0
else:
    print("Result: ${result}")

// � Bad: Ignore errors
let result: optional int = divide(10, 0)
// Don't forget to check! Compiler will warn if you do,
// but at runtime: result is none, must handle

// ✅ Better: Use #[error] annotation
#[error("Division by zero in calculate_interest")]
fn safe_divide_for_interest(a: float, b: float) optional float:
    if b == 0.0:
        return none
    return a / b
```

### Performance Best Practices
```novium
// ✅ Good: Use iterative, not recursive for loops
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

// � Bad: Unnecessary deep recursion without guard
fn fib_bad(n: int) int:
    if n <= 1:
        return n
    return fib_bad(n - 1) + fib_bad(n - 2)  // O(2^n) without guard
// Always use depth guard or iterative version for production

// ✅ Good: Type-annotate hot paths
fn fast_add(a: int, b: int) int:  // Annotation helps compiler optimize
    return a + b

// � Bad: Untyped hot paths (still works, slightly slower)
fn slow_add(a, b) int:
    return a + b
```

### Testing Best Practices
```novium
// ✅ Good: Test edge cases
fn test_fibonacci() void:
    // Test base cases
    assert(fibonacci(0) == 0)
    assert(fibonacci(1) == 1)
    
    // Test known values
    assert(fibonacci(10) == 55)
    assert(fibonacci(20) == 6765)
    
    // Test error case
    let result: optional int = divide(1, 0)
    assert(result is none)  // Division by zero

// ✅ Good: Use the test framework (when available)
# ifdef TEST_MODE
    test_fibonacci()
# endif

// � Bad: Only test happy path
fn bad_test() void:
    assert(fibonacci(5) == 5)  // Only one test, no edge cases
```
---
## 17. Performance Optimization

### Compiler Optimizations (Automatic)

| Optimization | When Applied | Effect |
|-------------|--------------|--------|
| **Inlining** | Small functions, `#[fast]` annotation | Removes call overhead |
| **Dead code elimination** | Unused variables, functions | Reduces binary size |
| **Constant folding** | Compile-time constant expressions | Pre-computes values |
| **Common subexpression elimination** | Same expression computed multiple times | Computes once, reuses |
| **Loop unrolling** | Simple loops with known iteration count | Reduces loop overhead |

### Developer Optimizations (Manual)

#### 1. Use Iterative Over Recursive
```novium
// � Recursive: O(2^n) time, O(n) stack
fn fib_recursive(n: int) int:
    if n <= 1:
        return n
    return fib_recursive(n - 1) + fib_recursive(n - 2)

// ✅ Iterative: O(n) time, O(1) space
fn fib_iterative(n: int) int:
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
```

#### 2. Use Type Annotations for Hot Paths
```novium
// ✅ Annotated: compiler can optimize
fn add_fast(a: int, b: int) int:
    return a + b

// ✅ Unannotated: still works, slight overhead
fn add_general(a, b) int:
    return a + b
```

#### 2. Avoid Unnecessary Allocations
```novium
// � Allocating on every call
fn create_string(s: string) string:
    return s + " suffix"  // New string allocated each call

// ✅ Reuse when possible
fn append_suffix(s: string, suffix: string) string:
    // In real: use mutable buffer or string builder
    // Here: just demo the concept
    return s + suffix
```

#### 2. Leverage Unboxed Types
```novium
// ✅ int/float unboxed: in registers, no heap
let x: int = 42
let y: float = 3.14
let sum: int = x + y  // Single ADD instruction

// ✅ Avoid boxed types when possible (when macro/system available)
```

#### 2. Use `#[fast]` and `#[pure]` Annotations
```novium
// ✅ fast hint: optimizer may inline, vectorize
#[fast]
fn compute_heavy(x: int, y: int) int:
    // Complex calculation
    return x * x + y * y

// ✅ pure hint: no side effects, can be reordered
#[pure]
fn add3(x: int) int:
    return x + 3
```

### Performance Optimization (Continued)

#### Memory Optimization
```novium
// ✅ Use stack allocation when size known
let fixed: [int; 100] = [0] * 100  // Stack, no heap

// ✅ Use optional (tagged immediate) for may-be-nothing values
let maybe_count: optional int = some(42)  // No heap allocation for int

// ✅ Avoid string concatenation in loops (create string builder instead)
let result: string = ""
for i in 0..1000:
    result = result + "${i}"  // 1000 allocations!
// Instead: collect into list, then join
```

#### CPU Optimization
```novium
// ✅ Use built-in math (may use CPU instructions)
let result: float = cos(3.14)  // May use FPU instruction

// � Avoid software emulation when hardware available
// (Novium's LLVM backend targets this automatically)
```

---
## 17. Common Pitfalls

### Pitfall 1: Forgetting to Handle `optional`
```novium
// � Pitfall: Assuming optional can be used like regular type
let x: optional int = divide(10, 0)
print(${x})  // COMPILE ERROR: x is optional, must handle both cases

// ✅ Fix: Always handle both cases
let x: optional int = divide(10, 0)
match x:
    some value:
        print(${value})
    none:
        print("Division by zero - using default")
```

### Pitfall 2: Moving Values Unintentionally
```novium
// � Pitfall: Value moved without realizing
let s: string = "hello"
let process: fn(string) void = fn(s: string) void:
    print(${s})

process(s)  // s moved INTO process; CAN'T use s after
// print(${s})  // COMPILE ERROR: s no longer valid

// ✅ Fix: Clone if need to keep original
let s: string = "hello"
let s2: string = s.clone()  // s2 is copy; s still valid
process(s)  // s moved, but s2 still valid
// OR: borrow instead
let s: string = "hello"
let s_ref: &string = &s  // Borrow; s still valid
process(*s_ref)  // Dereference and pass
```

### Pitfall 2: Non-exhaustive Match
```novium
// � Pitfall: Match not exhaustive (compile error)
fn describe(n: int) string:
    match n:
        0 => "zero"
        1 => "one"

// ✅ Fix: Add catch-all
fn describe(n: int) string:
    match n:
        0 => "zero"
        1 => "one"
        _ => "other"  // Required
```

### Pitfall 3: Infinite Recursion
```novium
// � Pitfall: No recursion depth guard
fn infinite_recursive(n: int) int:
    return n * infinite_recursive(n - 1)  // Never reaches base case!
    // Stack overflow at runtime (or compile error with guard)

// ✅ Fix: Always have base case + depth guard
fn safe_recursive(n: int, depth: int = 0) int:
    if depth > 256:
        return -1  // Depth exceeded
    if n <= 1:
        return 1  // Base case
    return n * safe_recursive(n - 1, depth + 1)
```

### Pitfall 4: String Index Out of Bounds
```novium
// � Pitfall: Unchecked string indexing
let s: string = "hi"
let char: string = s[100]  // ERROR: index 100 out of bounds for length 2

// ✅ Fix: Use safe indexing or check length first
let s: string = "hi"
// Option 1: Safe access (returns optional)
let char: optional string = s.at(100)  // None (out of bounds)
let char: optional string = s.at(0)    // Some("h")

// Option 2: Check length
if s.len() > 100:
    let char: string = s[100]
// Option 3: Safe indexing operator (if available)
let char: string = s[0]  // Safe: index 0 exists
```

### Pitfall 5: Forgetting `own` Semantics
```novium
// � Pitfall: Unexpected ownership behavior
let s: string = "data"
fn process(own s: string) void:
    print(${s})

process(own s)  // s transferred; CAN'T use after
// print(${s})  // COMPILE ERROR

// ✅ Fix: Understand ownership rules
// - Primitives copy (int, float, bool)
// - Complex types move by default
// - Use `own` keyword for explicit transfer
// - Use `&` for borrows when you need to keep original
```

### Pitfall 5: Misusing `async` and `go`
```novium
// � Pitfall: Misconcurrency patterns
// Spawning too many goroutines without limit
go fn:
    // Some heavy work

// No limit; could exhaust resources

// ✅ Fix: Use structured concurrency or limits
async fn main() void:
    // Controlled async tasks
    let results: list[int] = await_all(
        compute(1),
        compute(2),
        compute(3)
    )
    // All tasks awaited; resources cleaned up
```

### Pitfall 6: API Endpoint Without Documentation
```novium
// � Pitfall: API endpoint without annotations
#[get("/data")]
fn get_data() string:
    return "some data"

// � No docs generated; users don't know parameters/responses

// ✅ Fix: Add proper annotations
#[get("/data")]
#[description("Retrieve data records")]
#[response(200, {"items": list[string]})]
#[tag("Data")]
fn get_data() string:
    return "some data"
```

---
## 18. Performance Optimization

### Compiler Optimizations (Automatic)

| Optimization | When Applied | Effect |
|-------------|--------------|--------|
| **Inlining** | Small functions, `#[fast]` annotation | Removes call overhead |
| **Dead code elimination** | Unused variables, functions | Reduces binary size |
| **Constant folding** | Compile-time constant expressions | Pre-computes values |
| **Common subexpression elimination** | Same expression computed multiple times | Computes once, reuses |
| **Loop unrolling** | Simple loops with known iteration count | Reduces loop overhead |

### Developer Optimizations (Manual)

#### 1. Use Iterative Over Recursive
```novium
// � Recursive: O(2^n) time, O(n) stack
fn fib_recursive(n: int) int:
    if n <= 1:
        return n
    return fib_recursive(n - 1) + fib_recursive(n - 2)

// ✅ Iterative: O(n) time, O(1) space
fn fib_iterative(n: int) int:
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
```

#### 2. Use Type Annotations for Hot Paths
```novium
// ✅ Annotated: compiler can optimize
fn add_fast(a: int, b: int) int:
    return a + b

// ✅ Unannotated: still works, slight overhead
fn add_general(a, b) int:
    return a + b
```

#### 2. Avoid Unnecessary Allocations
```novium
// � Allocating on every call
fn create_string(s: string) string:
    return s + " suffix"  // New string allocated each call

// ✅ Reuse when possible
fn append_suffix(s: string, suffix: string) string:
    // In real: use mutable buffer or string builder
    // Here: just demo the concept
    return s + suffix
```

#### 2. Leverage Unboxed Types
```novium
// ✅ int/float unboxed: in registers, no heap
let x: int = 42
let y: float = 3.14
let sum: int = x + y  // Single ADD instruction

// ✅ Avoid boxed types when possible (when macro/system available)
```

#### 2. Use `#[fast]` and `#[pure]` Annotations
```novium
// ✅ fast hint: optimizer may inline, vectorize
#[fast]
fn compute_heavy(x: int, y: int) int:
    // Complex calculation
    return x * x + y * y

// ✅ pure hint: no side effects, can be reordered
#[pure]
fn add3(x: int) int:
    return x + 3
```

### Performance Optimization (Continued)

#### Memory Optimization
```novium
// ✅ Use stack allocation when size known
let fixed: [int; 100] = [0] * 100  // Stack, no heap

// ✅ Use optional (tagged immediate) for may-be-nothing values
let maybe_count: optional int = some(42)  // No heap allocation for int

// ✅ Avoid string concatenation in loops (create string builder instead)
let result: string = ""
for i in 0..1000:
    result = result + "${i}"  // 1000 allocations!
// Instead: collect into list, then join
```

#### CPU Optimization
```novium
// ✅ Use built-in math (may use CPU instructions)
let result: float = cos(3.14)  // May use FPU instruction

// � Avoid software emulation when hardware available
// (Novium's LLVM backend targets this automatically)
```
---
## 18. Safety & Security

### Memory Safety Guarantees

| Guarantee | How Enforced |
|-----------|--------------|
| **No null pointers** | `optional T` total; `none` only value |
| **No dangling pointers** | Borrow checker enforces at compile time |
| **No buffer overflows** | Strings length-prefixed; bounds-checked access |
| **No use-after-free** | Ownership tracked; automatic drop on scope exit |
| **No type confusion** | Strict static typing; no implicit conversions |

### Security Features by Layer

| Layer | Security Feature |
|-------|----------------|
| **.nvm** | Ownership + borrow checker; no GC pauses; error values instead of exceptions |
| **.nvi** | Type-safe FFI; `optional` forces error handling; async coroutines no OS threads |
| **.nvw** | Auto-HTML escaping; CSP headers; input validation; route sandboxing |

### Checkpoint: Safety
- **No null pointers**: `optional T` system
- **No memory unsafety**: Ownership + borrow checker
- **No XSS by default**: Auto-escaping + CSP in .nvw
- **No hidden control flow**: Error values, not exceptions
- **Type-safe FFI**: Documented conversions, explicit `#[link]`

---
## 19. Glossary & Quick References

### Key Terms Quick Reference

| Term | Definition |
|------|------------|
| `optional T` | `some Value` or `none`; no `null` |
| Unboxed | Stored in CPU registers |
| Borrow checker | Compiler enforcement of ownership |
| `own` keyword | Explicit ownership transfer |
| RAII | Auto-cleanup on scope exit |
| `#[fast]` | Optimization hint |
| `#[pure]` | No side effects hint |
| `#[get]` / `#[post]` | API endpoint |
| `#[link]` | C library link |
| `#[js_export]` | Wasm-JS binding |
| Goroutine | Lightweight green thread |
| Channel | Goroutine communication |
| `async` / `await` | Cooperative concurrency |
| OpenAPI | API spec format 3.1.0 |

### Type System Quick Ref

| Concept | Syntax |
|--------|--------|
| Type inference | `let x = 5` |
| Explicit annotation | `let x: int = 5` |
| Optional type | `let x: optional int = none` |
| String? alias | `let x: string? = none` |
| Match pattern | `match x: 0 => "zero" _ => "other"` |

### Concurrency Quick Ref

| Concept | Syntax |
|--------|--------|
| Spawn goroutine | `go fn: { ... }` |
| Create channel | `channel[int] = new_channel[int]()` |
| Send value | `chan.send(value)` |
| Receive value | `value = chan.receive()` |
| Async function | `async fn foo() string: ...` |
| Await value | `let x = await fn()` |
| Parallel map | `parallel_map(fn, list)` |

### Build Quick Ref

| Command | Action |
|---------|--------|
| `cmake ..` | Configure build |
| `make` | Build project |
| `novium file.nvm` | Compile & run |
| `novium --ast file.nvm` | Print AST |
| `novium --type-check file.nvm` | Type check |
| `novium --lint file.nvm` | Lint/style check |
| `novium --test` | Run tests |

### Pitfall Quick Ref

| Pitfall | Fix |
|---------|-----|
| Forgetting `optional` handling | Always `match` with `some` + `none` |
| Moving values unintentionally | Use `clone()` or `&` borrow |
| Non-exhaustive `match` | Add `_` catch-all |
| Infinite recursion | Add base case + depth guard (256) |
| String index OOB | Check length or use `.at(i)` |
| Forgetting `own` semantics | Primitives copy; complex types move |
| Misusing `async`/`go` | Use structured concurrency |
| API without docs | Add `#[description]`, `#[response]`, `#[tag]` |

---
## 19. Exercises & Projects

### Beginner Exercises

```novium
// Exercise 1: Hello with Args
// Write a program that accepts a name via command line argument
// and prints "Hello, {name}!" 
// (Hint: examine how main() receives arguments)

// Exercise 2: Temperature Converter
// Write a program that:
// - Takes a temperature and unit (C/F) as input
// - Converts to the other unit
// - Prints the result
// - Uses at least one function with return value
// - Uses string interpolation in print

// Exercise 3: Even Number Filter
// Write a function that takes a list of integers and returns
// a new list containing only the even numbers.
// Use a for loop to iterate.

// Exercise 4: Pattern Matching Practice
// Write a function that takes an integer and returns:
// - "zero" if 0
// - "single-digit" if 1-9
// - "double-digit" if 10-99  
// - "large" if 100+
// Use match with range patterns where possible.
```

### Intermediate Projects

```novium
// Project 1: CLI To-Do List
// Build a command-line todo list application that:
// - Stores tasks in a vector
// - Allows adding, removing, and marking tasks complete
// - Saves/loads from a file (std.fs)
// - Uses command-line arguments for operations
// - Has a help message

// Project 2: Number Guessing Game
// Build a game that:
// - Generates a random number between 1-100
// - Lets the user guess repeatedly
// - Tells user if guess is too high/low
// - Tracks number of guesses
// - Has a "play again?" feature

// Project 3: Simple JSON Parser (Partial)
// Parse a simplified JSON format (no nesting):
// {
//   "name": "Novium",
//   "version": 0.1,
//   "active": true
// }
// Return a struct with these fields.
// Handle basic types: strings, integers, booleans.

// Project 5: Web API Client
// Use the novel #[get]/#[post] annotations
// Build a program that calls your own generated API:
// - Fetches data from #[get] endpoints
// - Posts data to #[post] endpoints
// - Handles error responses
// - Generates and uses the client stubs
```

### Advanced Challenges

```novium
// Challenge 1: Custom Type System
// Extend Novium with a new type (e.g., custom numeric base)
// Implement: conversion, formatting, comparison operators
// Document as a new std module

// Challenge 2: Compiler Plugin
// Write a pass that analyzes Novium code and reports metrics:
// - Function count, line count, complexity scores
// - Most used types, most common patterns
// Output: JSON report file

// Challenge 3: Performance Benchmark Suite
// Benchmark Novium vs other languages on:
// - Fibonacci calculation (recursive vs iterative)
// - String processing throughput
// - Matrix operations
// Document results and optimization techniques

// Challenge 4: Domain-Specific Language
// Design a DSL embedded within Novium for a specific domain:
// - SQL-like query language
// - Configuration file parser
// - Game rule engine
// Implement the parser/compiler front-end in Novium
```

---
## 19. Conclusion

Novium represents a significant step forward in language design, combining:

- **Systems-level performance** through native LLVM compilation and ownership-based memory management
- **Scripting ergonomics** with Python-like syntax, type inference, and familiar patterns
- **Web capabilities** through Wasm compilation and API endpoint generation
- **Safety guarantees** through static typing, ownership checking, and error value handling

### The Three Layers, One Ecosystem

1. **`.nvm`** — Compile to native code for CLIs, servers, high-performance software
2. **`.nvi`** — Interpreted scripting layer that orchestrates `.nvm` modules and provides Python-like interop
3. **`.nvw`** — Web/UI layer compiling to Wasm + JavaScript for full-stack development

### Your Novium Journey

Starting points based on your background:

- **From Python**: Start with `.nvi` (familiar syntax), explore `.nvm` for performance
- **From C/Rust**: Start with `.nvm` (performance + safety), explore `.nvi`/` .nvw` for interop
- **From JavaScript/TypeScript**: Start with `.nvw` (Wasm + JS interop), learn `.nvm` for backend
- **From systems programming**: `.nvm` gives you Rust-level safety with friendlier syntax

### Next Steps

1. **Read the exercises** above and try the beginner examples
2. **Build the compiler**: `cd Novium\ Compiler\ language\(.nvm) && mkdir -p build && cd build && cmake .. && make`
3. **Run your first program**: `novium examples/hello.nvm`
4. **Explore the ecosystem**: Try importing between layers, generate API clients
5. **Join the community**: Check GitHub Discussions for questions and shows

### Final Thoughts

Novium isn't just another language—it's a **full-stack ecosystem** designed to solve the impedance mismatch between performance and productivity. By having three interconnected layers that can interoperate seamlessly, Novium lets you:

- Write high-performance systems code in `.nvm`
- Orchestrate and glue components in `.nvi`
- Build modern web interfaces in `.nvw`
- Move values and data freely between all three

The language will continue to evolve, with macro systems, extended type features, and more library modules being added regularly. The foundations laid out in this guide provide a solid platform for whatever you want to build—from CLI tools to full-stack web applications to embedded systems.

### AI Agent Code Generation Patterns

#### How to Prompt AI for Novium Code

When asking AI to generate Novium code, provide these elements for best results:

1. **Clear task description** - e.g., "Write a Novium function that computes factorial with error handling"
2. **Context layer** - specify `.nvm`, `.nvi`, or `.nvw` 
3. **Example patterns** - show similar code structures you want emulated
4. **Constraints** - ownership rules, type requirements, error handling style

#### Common Novium Code Generation Patterns

**Function Definition Template**
```novium
fn <name>(<param1>: <type1>, <param2>: <type2>) <return_type>:
    <!-- body with proper ownership and error handling -->
    return <value>
```

**Ownership-Aware Pattern**
```novium
// Primitives copy automatically
let x: int = 42
let y: int = x  // y is a copy; x still valid

// Complex types move by default
let s: string = "hello"
let process: fn(string) void = fn(own s: string) void:
    print(${s})
// process(s)  // s moved; use s.clone() or &s to keep original

// Use & for borrows when needed
let s: string = "hello"
let s_ref: &string = &s  // Borrow; s still valid
process(*s_ref)  // Dereference and pass
```

**Error Handling Pattern (optional-aware)**
```novium
// Always handle both cases
let result: optional int = divide(10, 0)
match result:
    some value:
        print(${value})
    none:
        print("Division by zero - using default")
```

**API Endpoint Pattern**
```novium
#[get("/users")]
#[description("List all users")]
#[response(200, {"users": list[string]})]
#[tag("Users")]
fn list_users() string:
    return json_serialize(user_list)
```

**Concurrency Pattern**
```novium
async fn main() void:
    let results: list[int] = await_all(
        compute(1),
        compute(2),
        compute(3)
    )
    // All tasks awaited; resources cleaned up
```

#### Anti-Patterns & Corrections

| Anti-Pattern | Correction |
|-------------|------------|
| `print(${undefined_var})` | Add variable declaration first, or use `let x: int = 0` with initialization |
| `fn process(s: string) void: print(${s})` without `own`/`&` awareness | Use `own` for transfer or `&` for borrow; document ownership semantics |
| `match n: 0 => "zero" 1 => "one"` (non-exhaustive) | Always add `_` catch-all |
| `fn recursive(n: int) int: return n * recursive(n - 1)` without base case/guard | Add `if n <= 1: return n` base case + `depth` guard (max 256) |
| `let char: string = s[100]` on short string | Use `s.at(100)` (returns optional) or check `s.len() > 100` first |
| `go fn: { ... }` without structured concurrency | Use `async fn main()` with `await_all()` or channel-limited spawning |
| API endpoint without `#[description]`/`#[response]` | Add annotations for documentation generation |
| Forgetting `own` semantics on function arguments | Remember: primitives copy; complex types move; use `own` for explicit transfer |

#### Novium Standard Library Idioms AI Should Know

**Math Functions** (from `math.nvm`)
```novium
// Constants
let pi: float = PI
let e: float = E

// Absolute value
let abs_val: int = abs(-5)  // 5

// Signum
let s: int = signum(-3)  // -1

// Square root
let sqrt_val: float = sqrt(16.0)  // 4.0

// Trig (radians)
let sin_val: float = sin(0.5)
let cos_val: float = cos(1.0)
let tan_val: float = tan(0.5)
```

**String Functions** (from `string.nvm`)
```novium
// Length
let len: int = str_len("hello")  // 5

// Case conversion
let upper: string = to_upper("hello")  // "HELLO"
let lower: string = to_lower("HELLO")  // "hello"

// Substring check
let has: bool = str_contains("hello world", "world")  // true

// HTML escaping (critical for .nvw)
let escaped: string = html_escape("<script>alert('xss')</script>")
// <script>alert('xss')</script>
```

**Vector Operations** (from `std/vector`)
```novium
let nums: vector[int] = new_vector[int]()
push(nums, 1)
push(nums, 2)
let last: int = pop(nums)  // removes and returns last element
let found: bool = contains(nums, 1)
```

#### AI-Generated Code Verification Checklist

After AI generates Novium code, verify:

- [ ] All `optional` values are matched with `some` + `none` branches
- [ ] Ownership is clear: values aren't used after being moved
- [ ] Type annotations match function signatures
- [ ] Match statements are exhaustive (have `_` catch-all)
- [ ] String indices are bounds-checked or use `.at(i)`
- [ ] API endpoints have `#[get]`/`#[post]` with proper annotations
- [ ] Concurrency uses `async`/`await` or structured `go` with limits
- [ ] No undefined variables or unimplemented functions
- [ ] Build compiles: `novium file.nvm` or `novium --type-check file.nvm`

#### Debugging AI-Generated Novium Code

Common compiler errors and fixes:

1. **"x is optional, must handle both cases"** - Add `match x: some value: ... none: ...`
2. **"value moved here; cannot be used"** - Add `clone()` or change to `&` borrow
3. **"non-exhaustive match"** - Add `_` catch-all branch
4. **"index 100 out of bounds for length N"** - Use `.at(100)` or check length first
5. **"cannot find function"** - Check import statements and module paths
6. **"type mismatch: expected int, got string"** - Add type annotations or convert types
7. **"undefined variable"** - Verify variable is declared before use

#### Recommended AI Prompt Formula

```
<task> in Novium <layer>

Example: "Write a Novium .nvm function that implements binary search on a sorted integer array, with proper error handling and type annotations."

Required elements:
- Layer: .nvm/.nvi/.nvw
- Function purpose
- Input types and constraints
- Return type and error handling style
- Any specific patterns to follow (ownership, match, etc.)
```

**Welcome to Novium. Write something amazing.**

---

*Novium v0.2.0 - Full-Stack Trio Complete: 30/30 sprints done, cross-variant interoperability, Mojo compatibility, trio bridge execution, binary wire protocol v0x200. Guide generated November 2026. For the latest updates, check the Novium repository.*

---
*(Done. The file has been written to `C:\Users\uchih\Novium Programming language\full-on learn guide.md`.)*

### 21. Cross-Language Interoperability

Novium v0.2.0 delivers seamless interoperability across all three language variants:

- **.nvm <-> .nvi**: Import and call functions between systems compiler and full-stack interpreter; automatic generation/sweep GC coordination
- **.nvm <-> .nvw**: Compile .nvm modules to Wasm+JS for web execution; DataPacket tensor metadata interop
- **.nvi <-> .nvw**: Trio bridge execution with WASM module loading/calling; sync_globals_across_trio; call_* bridge functions
- **Mojo Compatibility**: Python/Mojo superset features with native compilation targets; pub, struct, enum, pass, raise, with, cast, sizeof, alignof, Tensor, Matrix keywords
- **Trio Bridge Functions**: import_module, call_nvm_function_from_nvi, call_nvi_function_from_nvm, call_nvw_function, serialize_to_datapacket, deserialize_from_datapacket, sync_globals_across_trio, trio_compatibility_info()
- **Wire Protocol**: Binary format (with JSON fallback) using "NOVI" magic bytes, version 0x200, TensorMeta, ExecutionCommandPayload, type mappings, dynamic_axes field, format_version field

### 22. Mojo Language Compatibility

Novium v0.2.0 extends Python/Mojo superset features with native compilation targets:

**Keywords Added for Mojo Compatibility**:
- `pub` - Public access modifier
- `struct` - Value type definition
- `enum` - Enumeration type
- `pass` - Placeholder/empty function body
- `raise` - Exception raising (optional/error value style)
- `with` - Context manager / scope boundary
- `cast` - Type conversion operator
- `sizeof` - Size of type in bytes
- `alignof` - Alignment requirement of type
- `Tensor` - Tensor type for numerical computations
- `Matrix` - Matrix type for linear algebra

**Mojo Interop Features**:
- Python/Mojo syntax subset compatibility
- Native compilation targets: .nvm (systems), .nvi (interpreted), .nvw (Wasm/JS)
- Ownership types with flexibility (automatic GC in .nvi, deterministic in .nvm)
- Tensor and Matrix types for numerical computing
- FFI bridge to C via unified_connections.h

### 23. Trio Bridge Execution

Novium v0.2.0 trio bridge enables cross-variant function calls:

```novium
// Import a .nvm module from .nvi
import_module("math.nvm")

// Call .nvm function from .nvi
result = call_nvm_function_from_nvi("add", 5, 3)

// Call .nvi function from .nvm
result = call_nvi_function_from_nvm("double", 7)

// Call .nvw function
result = call_nvw_function("render", width, height)

// Serialize/Deserialize DataPacket
packet = serialize_to_datapacket(tensor, "float", [1,2,3])
data = deserialize_from_datapacket(packet)

// Sync globals across trio
sync_globals_across_trio()

# Trio compatibility info
trio_info = trio_compatibility_info()
```

**Bridge Execution Functions (bridge_execution.h:Sprints 25-29)**:
- WasmModuleInterface abstract class with WasmModuleLoadResult
- Concrete WasmModuleNVI/.nvw implementations
- CallConvention ABI standard for function calling
- Factory functions for each variant pair
- Trio compatibility methods for global state sync

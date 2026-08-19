# Novium syntax and style

Novium keeps one readable base language while accepting familiar conventions from C++, Rust, and Go. The goal is recognisable code, not a mixture of incompatible dialects.

## Functions

Both forms are valid. Prefer the arrow form in new code because it is immediately familiar to Rust and C++ developers.

```novium
// Original Novium / Go-like form
fn add(a int, b int) int:
    return a + b

// Recommended Rust/C++-familiar form
fn add(a: int, b: int) -> int {
    return a + b
}
```

Indentation blocks remain first-class and are recommended for multi-line control flow. Braces are available for developers coming from C++, Rust, Go, Java, or JavaScript.

## Variables and mutability

```novium
let title: string = "Novium" // immutable, like Rust `let`
let mut count: int = 0        // recommended Rust-familiar mutable binding
var legacy_count: int = 0     // original Novium mutable spelling; still supported
count = count + 1
```

`let` cannot be reassigned. `let mut` and `var` can. This retains Novium’s original explicit-mutability idea while being close to Rust and understandable to Go/C++ developers.

## Loops and efficient updates

```novium
let mut total: int = 0
while total < 1000:
    total += 1
```

`while` and `+=`/`-=`/`*=`/`/=` avoid recursive control flow and are familiar to C++, Rust, and Go developers. The v0.1 interpreter executes these loops directly without allocating a new variable environment for each iteration.

## Familiar features retained

```novium
// C++ / Rust / Go familiar pieces
// - `//` and `#` comments
// - `{}` or `:` + indentation blocks
// - `true`, `false`, `null`
// - `==`, `!=`, `<`, `<=`, `&&`, `||`
// - `fn`, typed parameters, `return`, `main`
```

## Deliberate Novium differences

- No implicit mutable variables: use `var`.
- No semicolons required; they are optional in brace blocks.
- No ownership/borrow promises until the checker exists. Future resource safety should be introduced through explicit `unsafe`, `defer`, and C-ABI boundaries rather than surprising rules.
- Prefer one canonical spelling per feature. Compatibility syntax should ease migration, not make code harder to read.

## Recommended v0.1 style

```novium
fn main() -> void:
    let greeting: string = "Hello from Novium"
    let result: int = add(10, 20)
    print(greeting)
    print(result)

fn add(a: int, b: int) -> int {
    return a + b
}
```

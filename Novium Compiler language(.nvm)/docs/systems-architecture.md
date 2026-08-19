# Novium systems architecture

## Stable boundaries

Novium code should cross language and hardware boundaries through a small, versioned C ABI. Public APIs use fixed-width integers, `novium_string_view`, slices, opaque handles, status codes, and `novium_abi_version()`. They never expose C++ classes, exceptions, Rust layouts, garbage-collected objects, or compiler-private structs.

The starter ABI is [../include/novium_abi.h](../include/novium_abi.h). It is a contract for the native backend; the v0.1 interpreter does not generate shared libraries yet.

## Compilation speed

The eventual compiler architecture uses parsed-module caches and a typed intermediate representation, not textual header inclusion. Packages expose interface metadata; changing implementation files recompiles only dependent implementation units. Generics are type-checked once and monomorphized only for exported/used specializations. Borrow/lifetime analysis happens after parsing, on the typed IR, and is incremental.

This avoids treating templates, imports, and safety checks as reasons to reparse a project from scratch. It cannot guarantee zero compile-time cost, but it makes the cost visible and cacheable.

## Memory and safety

Default bindings are immutable. `let mut`/`var` are explicit. Values should have deterministic layouts in native mode; heap allocations use ownership-carrying handles and resource cleanup uses `defer`. Unsafe pointers, platform APIs, and FFI must be opt-in through an `unsafe` boundary. Full borrow checking is a later compiler pass, not a claim made by the current interpreter.

## Concurrency and accelerators

The baseline runtime must run correctly on one thread with no special hardware. Parallelism and GPU/NPU kernels are opt-in capabilities selected by the compiler only when the source declares a parallel/data-parallel operation and the target supports it. A correct CPU fallback is required. Async uses explicit tasks and cancellation-safe scopes; `await` never implicitly pins a thread or hides blocking work.

## Language evolution

ADTs/enums, pattern matching, collections, OOP classes/interfaces, functional operations, modules, async, and native backends are separate staged features. Each gains a specification, AST/IR support, diagnostics, tests, and ABI implications before it is advertised as stable.

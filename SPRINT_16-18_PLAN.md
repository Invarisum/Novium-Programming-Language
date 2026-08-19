# Novium Sprints 16-18 Plan (v0.1.6 → v0.2.0)

**Current:** Sprint 15 Complete — **v0.1.5 (v0.2.0-beta)** released  
**Roadmap:** [v0.1-roadmap.md](Novium%20Compiler%20language(.nvm)/docs/v0.1-roadmap.md)  
**Target:** 3 Sprints × 2 Weeks = 6 Weeks to v0.2.0 (stable native core)

---

## 🏁 Sprint 16: Portable C Backend (v0.1.6) — Week 1-2
**Goal:** Emit portable C as first native backend — "keeps generated output inspectable and lets Novium call mature C APIs immediately"

### Must-Have (P0)
| Task | File | Est. | Owner |
|------|------|------|-------|
| C codegen: emit `.c` + `.h` from AST (no LLVM) | `codegen/c_codegen.cpp` (new) | 4d | |
| Type mapping: Novium → C (int→int64_t, string→struct, slices→ptr+len) | `codegen/c_codegen.cpp` | 2d | |
| Runtime C library: `novium_rt.c/h` (alloc, panic, print, slices) | `runtime/novium_rt.c` (new) | 2d | |
| `--emit=c` CLI flag + `novium build --target=c` | `main.cpp` + CMake | 1d | |
| Struct layout: `#[repr(C)]` compatible, no padding surprises | `codegen/c_codegen.cpp` | 1d | |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| Compile & run `examples/hello.nvm` via `gcc -o hello hello.c novium_rt.c` | `examples/` | 1d |
| Array/slice/string operations in C runtime | `runtime/novium_rt.c` | 1d |
| Basic `extern "C"` import support (call C from Novium) | `parser.cpp` + `c_codegen.cpp` | 2d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| Cross-compilation flags: `--target=arm64`, `--target=wasm32` (via C) | `CMakeLists.txt` | 2d |

### Definition of Done
- [ ] `novium build --target=c hello.nvm` → `hello.c` + `hello.h`
- [ ] `gcc hello.c novium_rt.c -o hello` runs and prints "Hello from Novium"
- [ ] Structs, arrays, slices, strings work in emitted C
- [ ] Can call `novium` functions from C `main()`

---

## 🏁 Sprint 17: C ABI Layer + Resource Safety (v0.1.7) — Week 3-4
**Goal:** "`extern "C"` imports/exports, shared-library builds, documented type mapping" + "defer, RAII-style wrappers, explicit unsafe blocks"

### Must-Have (P0)
| Task | File | Est. |
|------|------|------|
| `extern "C"` export: `#[export] fn foo() -> int` → C-callable symbol | `parser.cpp` + `c_codegen.cpp` | 2d |
| `extern "C"` import: `extern "C" { fn libc_malloc(size) -> *u8 }` | `parser.cpp` + `type_checker.cpp` | 2d |
| Shared library build: `novium build --shared libfoo.nvm` → `libfoo.so/.dll` | `CMakeLists.txt` + `main.cpp` | 2d |
| Type mapping doc: `novium.md#c-abi-type-mapping` | `docs/c_abi.md` (new) | 1d |
| `defer` statement: `defer close(file)` — runs at scope exit | `parser.cpp` + `interpreter.cpp` + `c_codegen.cpp` | 2d |
| RAII wrapper pattern: `struct File { fd: int; ~File { close(fd) } }` | `examples/raii.nvm` | 1d |
| `unsafe { ... }` blocks: disable borrow checker locally | `parser.cpp` + `type_checker.cpp` | 2d |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| C header generation: `novium build --header libfoo.nvm` → `libfoo.h` | `c_codegen.cpp` | 1d |
| `#[no_mangle]` attribute for custom symbol names | `parser.cpp` | 4h |
| Bindgen integration: `novium bindgen foo.h` → `foo.nvm` | `tools/bindgen/` (new) | 3d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| C++ name mangling support (optional) | `migration/migratir.cpp` | 3d |

### Definition of Done
- [ ] `novium build --shared libfoo.nvm` → `libfoo.so` + `libfoo.h`
- [ ] C program `#include "libfoo.h"` calls Novium functions
- [ ] Novium calls `malloc`/`free` via `extern "C"`
- [ ] `defer` runs on normal return, panic, and unwind
- [ ] `unsafe` block compiles and runs (borrow checker disabled inside)

---

## 🏁 Sprint 18: LLVM Backend + Stabilization (v0.2.0) — Week 5-6
**Goal:** "Add LLVM backend only after language semantics and C ABI are stable"

### Must-Have (P0)
| Task | File | Est. |
|------|------|------|
| LLVM IR codegen: revive `codegen.cpp` for native backend | `codegen.cpp` | 4d |
| `--target=llvm` / `--target=native` CLI flag | `main.cpp` | 1d |
| Debug info: DWARF emission (line tables, variables) | `codegen.cpp` | 2d |
| Optimization passes: inlining, DCE, const propagation | `codegen.cpp` | 2d |
| Self-host: compile compiler with LLVM backend | `CMakeLists.txt` | 1d |
| Release artifacts: `novium-v0.2.0-{linux,macos,windows}.tar.gz` | `.github/workflows/release.yml` | 1d |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| LSP: stabilize on native backend (not interpreter) | `novium_lsp.go` | 2d |
| Package manager: publish to GitHub Releases | `novium_pkg_manager.py` | 1d |
| Benchmark suite: `novium bench` vs C/Rust/Go | `examples/bench/` | 2d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| WASM target via LLVM: `--target=wasm32` | `web_codegen.cpp` | 2d |
| Incremental compilation (query engine) | `query_engine.cpp` (new) | 3d |

### Definition of Done
- [ ] `novium build --target=native hello.nvm` → `hello` (native binary)
- [ ] Binary runs without interpreter, links `novium_rt` statically
- [ ] Debug build works in `lldb` / VS Code (`break main`, step, inspect)
- [ ] Self-hosted compiler builds and passes all tests
- [ ] v0.2.0 tagged, released, announced

---

## 📊 Capacity Planning (Roadmap-Aligned)

| Sprint | Theme | P0 Days | P1 Days | P2 Days | Total | Buffer (20%) |
|--------|-------|---------|---------|---------|-------|--------------|
| 16 | Portable C Backend | 10 | 4 | 2 | 16 | 19.2 |
| 17 | C ABI + Resource Safety | 11 | 3.5 | 3 | 17.5 | 21 |
| 18 | LLVM Backend + Release | 11 | 5 | 2 | 18 | 21.6 |

**Note:** Sprint 16 is heaviest (new C codegen from scratch). Consider 3-week Sprint 16 if needed.

---

## 🎯 Milestone Gates (Per Roadmap)

| Gate | Version | Criteria | Roadmap Step |
|------|---------|----------|--------------|
| **C Backend Alpha** | v0.1.6 | `novium build --target=c` works for hello world | Step 2 |
| **C ABI Beta** | v0.1.7 | Shared libs + `extern "C"` + `defer` + `unsafe` | Step 3, 4 |
| **LLVM RC1** | v0.2.0-rc1 | Native binary + debug + self-host | Step 5 |
| **v0.2.0 Stable** | v0.2.0 | All above + release artifacts + benchmarks | — |

---

## 🔄 Dependencies (Roadmap Order)

```
Sprint 16 (C Backend)
  ├─ C runtime → enables Sprint 17 shared libs
  ├─ Type mapping → enables Sprint 17 extern "C"
  └─ Struct layout → enables Sprint 18 LLVM ABI compat

Sprint 17 (C ABI + Safety)
  ├─ Shared libs → enables Sprint 18 dynamic linking
  ├─ defer/RAII → enables safe resource mgmt in LLVM
  ├─ unsafe blocks → enables low-level LLVM intrinsics
  └─ C header gen → enables C/C++ interop examples

Sprint 18 (LLVM Backend)
  ├─ Reuses C type mapping / ABI
  ├─ Reuses novium_rt.c as runtime
  └─ Emits same calling convention as C backend
```

---

## 📝 Notes

- **Strict roadmap adherence:** No borrow checker until v0.3+ (roadmap: "separate project")
- **No package registry** until v0.3+ (roadmap: "should not block native core")
- **No WASM/web** until v0.3+ (roadmap: "separate project")
- **C backend is throwaway prototype** — validates semantics before LLVM investment
- **Self-host is the ultimate test** — compiler compiles itself with LLVM backend

---

## 🚀 Quick Start for Sprint 16

```bash
# 1. Create C codegen skeleton
mkdir -p "Novium Compiler language(.nvm)/src/codegen"
touch "Novium Compiler language(.nvm)/src/codegen/c_codegen.{cpp,h}"

# 2. Add to CMakeLists.txt
# add_library(novium_c_codegen src/codegen/c_codegen.cpp)

# 3. Implement: fn codegen_c(program: &Program) -> String

# 4. Test pipeline:
#    novium --ast examples/hello.nvm      # verify AST
#    novium --codegen-c examples/hello.nvm # emit hello.c
#    gcc hello.c novium_rt.c -o hello     # compile
#    ./hello                              # run
```

---

*Aligned with [v0.1-roadmap.md](Novium%20Compiler%20language(.nvm)/docs/v0.1-roadmap.md) — portable C → C ABI → LLVM*
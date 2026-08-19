# Novium Sprints 16-18 Plan (v0.1.6 → v0.3.0)

**Current:** Sprint 15 Complete — **v0.1.5 (v0.2.0-beta)** released  
**Roadmap:** [v0.1-roadmap.md](Novium%20Compiler%20language(.nvm)/docs/v0.1-roadmap.md)  
**Target:** 3 Sprints × 2 Weeks = 6 Weeks to v0.3.0 (SDK + Cross-Language + Production)

---

## 🎯 Vision: Novium as a Systems Language Platform

> **Goal:** Make Novium the best "glue language" for systems programming — combining Python ergonomics with C performance, cross-language interop, and a genuine SDK ecosystem.

```
         +-------------------+       +-------------------+
         |   Python Scripts|       |   C/C++ Projects  |
         +--------+----------+       +--------+----------+
                  \              /
                   \            /
                    \          /
                     \        /
          +-----------------+-----------------+
          |                 |                 |
          |   Novium        |    Mojo         |
          |   Programs      |   Interop       |
          |   (Safe, Fast)  |   (Python+)     |
          +-----------------+-----------------+
                   ^                  ^
                   |                  |
                   +----------+-------+
                                |
                            SDK & Tooling
```

---

## � Sprint 16: SDK Foundations (v0.2.0) — Week 1-2
**Goal:** Build the Novium SDK — libraries, headers, build integration, and Moji

### Must-Have (P0)
| Task | File | Est. | Owner |
|------|------|------|-------|
| C ABI type map doc: every Novium type → C type | `docs/c_abi.md` (new) | 2d | |
| Novium → C header generation: `novium build --header` | `c_codegen.cpp` | 1d | |
| Novium → Moji bridge: `import moji` + `fn c_call() -> int` | `parser.cpp` + new `moji_bridge.cpp` | 3d | |
| SDK `include/novium.h`: all types, macros, calling convention | `src/sdk/novium.h` (new) | 2d | |
| `--sdk` CLI: emit SDK package (headers + lib + cmake config) | `main.cpp` + CMake | 1d | |
| Build: `novium sdk` → `novium-sdk-v0.2.0/` with all artifacts | `CMakeLists.txt` | 1d | |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| Moji `fn` → Novium `fn` auto-translation (type mapping) | `moji_bridge.cpp` | 2d |
| C → Novium `extern "C"` import stubs | `parser.cpp` | 1d |
| CMake `find_package(novium)` integration | `CMakeLists.txt` | 2d |
| `novium-sdk.pc` (pkg-config) for easy C/C++ linking | `tools/sdk/pkgconfig/` (new) | 1d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| Rust `extern "C"` ↔ Novium FFI generator | `tools/ffi/` (new) | 3d |
| Swift bridging support | `tools/bridges/` (new) | 3d |

### Definition of Done
- [ ] `novium sdk` generates `include/novium.h`, `libnovium.a`, `novium-sdk.cmake`
- [ ] `cargo new --type novium` or equivalent scaffold works
- [ ] `#include <novium.h>` + `gcc test.c novium_rt.c -o test` compiles + runs
- [ ] `import moji` in Novium works for basic types (int, string, array)
- [ ] `novium pkg install moji-sdk` hypothetical future flow

---

## � Sprint 17: Mojo Cross-Language + Runtime (v0.2.0) — Week 3-4
**Goal:** Deep Mojo interop + runtime features that make Novium a true systems language

### Must-Have (P0)
| Task | File | Est. |
|------|------|------|
| Moji FFI: `extern "Mojo"` import of C functions | `parser.cpp` + `moji_bridge.cpp` | 3d |
| Novium → Moji: compile Novium code from within Moji playground | `moji_bridge.cpp` | 2d |
| Runtime: `novium_gc` / arena allocation for zero-overhead interop | `runtime/novium_rt.c` | 2d |
| `defer` + `unsafe` blocks compile for Moji interop code | `parser.cpp` + `type_checker.cpp` | 2d |
| `novium benchmark` vs Moji hello world | `examples/bench/` | 1d |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| Shared type system: `novium.Int` <-> `mojo.Int` equivalence | `type_checker.cpp` + `moji_bridge.cpp` | 2d |
| Cross-language test: Novium `fn` called from Moji `fn` and vice versa | `examples/cross_lang.nvm` + Moji test | 2d |
| `async fn` in Novium awaitable from Moji coroutine | `runtime/async_executor.cpp` | 2d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| Moji `print()` → Novium `print()` routed through FFI | `moji_bridge.cpp` | 1d |
| Build: single binary mixing Novium + Moji + C | `CMakeLists.txt` | 2d |

### Definition of Done
- [ ] Moji `print("hello")` can be called from Novium `go print("hello")`
- [ ] Novium `fn add(a: int, b: int) -> int` callable from Moji `def add(a: Int, b: Int) -> Int`
- [ ] No segfaults in cross-language interop baseline cases
- [ ] `novium bench` numbers published and compared

---

## � Sprint 18: SDK & Tooling Generalization (v0.2.1 → v0.3.0) — Week 5-6
**Goal:** SDK maturity, language-agnostic tooling, and "better things" for developers

### Must-Have (P0)
| Task | File | Est. |
|------|------|------|
| SDK: `novium new <name>` project scaffold (like `cargo new`) | `tools/scaffold/` (new) | 2d |
| SDK: `novium test` runs cross-language tests (Novium + Moji + C) | `tools/test/` (new) | 2d |
| SDK docs: `novium-sdk.md` — tutorials, FFI guides, migration | `docs/sdk.md` (new) | 3d |
| `novium migrate` updated: Novium → Moji, Novium → C, C → Novium | `migration/migratir.cpp` | 2d |
| Windows MSI installer OR `novium windows-x86_64.msi` | `tools/installer/` (new) | 3d |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| SDK: `novium add <pkg>` — local git path + GitHub repo packages | `novium_pkg_manager.py` | 2d |
| SDK: `novium doc <function>` — quick type docs from AST | `tools/docgen/` (new) | 1d |
| CI: GitHub Actions matrix (Windows/Linux/Mac, Novium/Moji/C tests) | `.github/workflows/ci.yml` | 2d |
| `novium fmt` — format source code (like `gofmt`, `rustfmt`) | `tools/fmt/` (new) | 2d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| Language-agnostic AST schema (JSON) for inter-tool communication | `ast/schema.json` (new) | 2d |
| AI-assisted Novium code: `novium suggest` (basic) | `tools/ai/` (new) | 3d |

### Definition of Done
- [ ] `novium new my_app` → scaffolded app with Novium + C FFI template
- [ ] `novium test --cross` runs Novium + Moji + C unit tests
- [ ] `novium sdk docs` opens browser with FFI guides
- [ ] `novium fmt` formats 100% of `examples/` without manual edits
- [ ] Windows installer runs and adds `novium` to PATH

---

## 📊 Capacity Planning (Ambitious but Focused)

| Sprint | P0 Days | P1 Days | P2 Days | Total | Buffer (15%) |
|--------|---------|---------|---------|-------|--------------|
| 16 | 8 | 4 | 3 | 15 | 17.25 |
| 17 | 8 | 4 | 2 | 14 | 16.1 |
| 18 | 9 | 4 | 4 | 17 | 19.5 |

**Critical Path:** Sprint 16 (SDK) → Sprint 17 (Moji interop) → Sprint 18 (maturity/tooling)

---

## 🎯 Milestone Gates (Expanded)

| Gate | Version | Criteria | Date |
|------|---------|----------|------|
| **SDK Alpha** | v0.2.0 | `novium sdk` generates usable C headers + Moji FFI baseline | Sprint 16 |
| **Moji Interop Beta** | v0.2.0 | Novium `fn` callable from Moji, no segfaults | Sprint 17 |
| **SDK RC** | v0.2.1 | `novium new`, `novium test --cross`, `novium fmt` all work | Sprint 18 |
| **v0.3.0 Stable** | v0.3.0 | Full SDK + interop + tooling + docs + installer | After S18 |

---

## 🔄 Dependencies (Expanded)

```
Sprint 16 (SDK Foundations)
  ├─ C ABI doc → enables Sprint 17 Moji FFI type mapping
  ├─ C header gen → enables Sprint 18 SDK distribution
  ├─ Moji bridge → enables Sprint 17 cross-language calls
  └─ SDK scaffold → enables Sprint 18 developer onboarding

Sprint 17 (Mojo Interop)
  ├─ FFI baseline → enables Sprint 18 cross-language testing
  ├─ Runtime features → enables Sprint 18 stable SDK
  └─ Shared types → enables Sprint 18 pkg manager integration

Sprint 18 (SDK & Tooling)
  ├─ Scaffold → immediate developer value
  ├─ CI matrix → confidence for 1.0 release
  └─ `novium fmt` → codebase consistency for long-term
```

---

## 🚀 Quick Start for SDK

```bash
# 1. Install SDK
novium sdk

# 2. Create new project
novium new my_game

# 3. Add C FFI support
novium add ./my_c_lib

# 4. Cross-language test
novium test --cross

# 4. Format code
novium fmt

# 5. Read docs
novium sdk docs  # opens browser
```

---

*Aligned with [v0.1-roadmap.md](Novium%20Compiler%20language(.nvm)/docs/v0.1-roadmap.md) — SDK → Moji interop → Production maturity*

**Note:** Mojo is a trademark of Modular AI. This plan implements compatible FFI, not an official Mojo integration. "Moji" is used as a shortened reference for cross-language interop functionality.
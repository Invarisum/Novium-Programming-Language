# Novium Sprints 16-18 Plan

**Current:** Sprint 15 Complete (Compiler + Interpreter + Web + Package Manager + Docs)  
**Target:** 3 Sprints × 2 Weeks = 6 Weeks to "Usable Systems Language"

---

## 🏁 Sprint 16: Runtime Foundations (Week 1-2)
**Goal:** Make interpreter run real programs with collections, I/O, and async

### Must-Have (P0)
| Task | File | Est. | Owner |
|------|------|------|-------|
| Add `Vec<T>`, `Map<K,V>` to `Value::ValueData` | `interpreter.h/cpp` | 2d | |
| Implement `len()`, `push()`, `pop()`, `get()`, `set()` builtins | `interpreter.cpp` | 1d | |
| Complete `io.nvm` (read_file, write_file, stdin, stdout) | `libraries/io.nvm` | 1d | |
| Implement `Array`/`Slice` type checking in type checker | `type_checker.cpp` | 1d | |
| Fix `novium --repl` to link C++ interpreter (not Go stub) | `novium_repl.go` + CMake | 4h | |
| Add `--emit=ast,ir,asm` CLI flags | `main.cpp` | 2h | |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| String methods: `split`, `trim`, `replace`, `parse_int` | `libraries/string.nvm` | 1d |
| Math module: `random`, `min`, `max`, `clamp` | `libraries/math.nvm` | 4h |
| Fuzz testing integration: `novium test --fuzz` | `libraries/fuzz.nvm` + CLI | 1d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| Basic `net.nvm` (TCP connect, HTTP GET) | `libraries/net.nvm` | 2d |

### Definition of Done
- [ ] `novium --run examples/http_client.nvm` works
- [ ] `novium --repl` runs interpreter (not Go stub)
- [ ] Arrays/slices type-check and execute
- [ ] File I/O works in interpreted programs

---

## 🏁 Sprint 17: Concurrency + Safety (Week 3-4)
**Goal:** True async/await + borrow checker + pattern exhaustiveness

### Must-Have (P0)
| Task | File | Est. |
|------|------|------|
| Async executor: thread pool + task queue + waker | `runtime/async_executor.cpp` (new) | 3d |
| `Future<T>` / `Promise<T>` in Value + `await` suspension | `interpreter.cpp` | 2d |
| `go` spawns on executor (not detached thread) | `interpreter.cpp` | 1d |
| Flow-sensitive borrow checker (NLL-style) | `type_checker.cpp` + new `borrow_checker.cpp` | 4d |
| Move semantics: track moved-from state in symbol table | `symbol_table.cpp` + `type_checker.cpp` | 2d |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| Match exhaustiveness for enums/ints/structs | `type_checker.cpp` | 2d |
| Channel type: `chan<T>` with send/recv/select | `libraries/chan.nvm` + runtime | 2d |
| `defer` statement (RAII for runtime) | `parser.cpp` + `interpreter.cpp` | 1d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| Async stdlib: `sleep`, `timeout`, `join_all` | `libraries/async.nvm` | 1d |

### Definition of Done
- [ ] `async fn fetch() -> Result<Data>` + `await` works
- [ ] `go` runs on thread pool, not leaked threads
- [ ] Borrow checker catches: use-after-move, double mut borrow, mut+shared conflict
- [ ] Match on enum warns if variant missing
- [ ] Channels work for goroutine communication

---

## 🏁 Sprint 18: Polish + Ecosystem (Week 5-6)
**Goal:** Production-ready tooling, standard library, interop

### Must-Have (P0)
| Task | File | Est. |
|------|------|------|
| Package manager: remote registry (GitHub releases) | `novium_pkg_manager.py` + `pm_backend.go` | 3d |
| `novium pkg add github.com/user/repo@v1.2.3` | CLI + backend | 1d |
| C/C++ header parsing for `extern "C++"` | `migration/migratir.cpp` | 3d |
| DWARF debug info in LLVM codegen | `codegen.cpp` | 2d |
| LSP: go-to-def for types, inlay hints, semantic tokens | `novium_lsp.go` | 2d |

### Should-Have (P1)
| Task | File | Est. |
|------|------|------|
| JSON module: `json.nvm` (serialize/deserialize) | `libraries/json.nvm` | 1d |
| Regex module: `regex.nvm` (PCRE2 binding) | `libraries/regex.nvm` | 1d |
| Time module: `time.nvm` (instant, duration, format) | `libraries/time.nvm` | 1d |
| Test framework: `#[test]` attribute + `novium test` | `parser.cpp` + CLI | 1d |
| Benchmark harness: `novium bench` | CLI + `libraries/bench.nvm` | 1d |

### Stretch (P2)
| Task | File | Est. |
|------|------|------|
| WASM target: `novium build --target wasm` | `web_codegen.cpp` + CMake | 2d |
| Macro system: `macro_rules!` hygiene | `parser/` + new `macro_expander.cpp` | 3d |
| Incremental compilation (query-based) | `main.cpp` + new `query_engine.cpp` | 3d |

### Definition of Done
- [ ] `novium pkg add` installs from GitHub
- [ ] `extern "C++" { #include <vector> }` works
- [ ] Debug build runs in VS Code / lldb
- [ ] LSP works in VS Code extension
- [ ] `novium test` runs `#[test]` functions
- [ ] Standard library covers: io, fs, net, json, regex, time, async, chan

---

## 📊 Capacity Planning

| Sprint | P0 Days | P1 Days | P2 Days | Total | Buffer (20%) |
|--------|---------|---------|---------|-------|--------------|
| 16 | 6 | 2.5 | 2 | 10.5 | 12.6 |
| 17 | 12 | 5 | 1 | 18 | 21.6 |
| 18 | 11 | 5 | 5 | 21 | 25.2 |

**Recommendation:** Sprint 17 is heaviest. Consider moving Channel/Defer to Sprint 18 if needed.

---

## 🎯 Milestone Gates

| Gate | Criteria | Sprint |
|------|----------|--------|
| **Alpha** | Run HTTP server example end-to-end | 16 |
| **Beta** | Borrow checker passes Rust borrowck test suite subset | 17 |
| **RC1** | Package manager installs 3rd party pkg; debug works | 18 |
| **v1.0** | All P0 + P1 complete; docs + examples for each module | 18+ |

---

## 🔄 Dependencies

```
Sprint 16
  ├─ Vec/Map → enables Sprint 17 channels
  ├─ io.nvm → enables Sprint 17 async I/O
  └─ REPL fix → dogfooding for Sprint 17/18

Sprint 17
  ├─ Async executor → enables Sprint 18 async stdlib
  ├─ Borrow checker → required for safe C++ interop (Sprint 18)
  └─ Match exhaustiveness → language completeness

Sprint 18
  ├─ Package manager → needs Sprint 16/17 stdlib packages
  ├─ C++ interop → needs borrow checker for safety
  └─ LSP/Debug → needs stable AST/IR from Sprint 16/17
```

---

## 📝 Notes

- **Parallelize:** P0 tasks within sprint can run in parallel (different files)
- **Dogfood:** Write Sprint 16 examples in Novium, test with Sprint 17 features
- **CI:** Add GitHub Actions for `novium test` + `novium --check examples/*.nvm` each sprint
- **Metrics:** Track compile time, binary size, test count per sprint
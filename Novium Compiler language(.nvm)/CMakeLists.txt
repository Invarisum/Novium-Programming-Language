cmake_minimum_required(VERSION 3.16)
project(novium-compiler VERSION 0.1.5 LANGUAGES CXX)

# ── C++ Standard ──────────────────────────────────────────────────────────────
# C++17 gives us: std::optional, std::string_view, structured bindings,
# if-init statements — all useful for clean compiler code.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Compiler Warnings ─────────────────────────────────────────────────────────
# We treat warnings seriously — a compiler must be correct, so our own code
# should be held to the highest standard.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

# ── Source Files ──────────────────────────────────────────────────────────────
# LLVM codegen is optional: enable with -DNOVIUM_WITH_LLVM=ON when LLVM is
# installed. Without it, a stub implementation keeps the build self-contained.
option(NOVIUM_WITH_LLVM "Build with LLVM IR code generation" OFF)

if(NOVIUM_WITH_LLVM)
    find_package(LLVM CONFIG REQUIRED)
    message(STATUS "LLVM found: ${LLVM_PACKAGE_VERSION}")
    set(NOVIUM_CODEGEN_SOURCES src/codegen.cpp)
else()
    set(NOVIUM_CODEGEN_SOURCES src/codegen_stub.cpp)
endif()

set(NOVIUM_SOURCES
    src/lexer/token.cpp
    src/lexer/lexer.cpp
    src/parser/ast.cpp
    src/parser/ast_printer.cpp
    src/parser/parser.cpp
    src/sema/types.cpp
    src/sema/symbol_table.cpp
    src/sema/type_checker.cpp
    ${NOVIUM_CODEGEN_SOURCES}
    src/web_codegen.cpp
    src/unified_connections.cpp
    src/runtime/novium_rt.cpp
    src/sdk/novium.c
    src/moji/novium_moji_bridge.cpp
    src/runtime/interpreter.cpp
)

# The SDK runtime is plain C but the project builds with C++ only;
# compile it as C++ (the code is valid in both languages).
set_source_files_properties(src/sdk/novium.c PROPERTIES LANGUAGE CXX)

# ── Novium Compiler Library ───────────────────────────────────────────────────
# Build the compiler internals as a static library so both the main binary
# and the test binary can link against it without duplicating compilation.
add_library(novium_lib STATIC ${NOVIUM_SOURCES})
target_include_directories(novium_lib PUBLIC src)

# ── Main Compiler Binary ─────────────────────────────────────────────────────
add_executable(novium src/main.cpp)
target_link_libraries(novium PRIVATE novium_lib)

# ── Migration Tool ────────────────────────────────────────────────────────────
add_executable(migration_tool migration/migratir.cpp)
target_include_directories(migration_tool PRIVATE migration src)
target_link_libraries(migration_tool PRIVATE novium_lib)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(lexer_test tests/lexer_test.cpp)
target_link_libraries(lexer_test PRIVATE novium_lib)
add_test(NAME LexerTest COMMAND lexer_test)

add_executable(lexer_stress_test tests/lexer_stress_test.cpp)
target_link_libraries(lexer_stress_test PRIVATE novium_lib)
add_test(NAME LexerStressTest COMMAND lexer_stress_test)

add_executable(runtime_test tests/runtime_test.cpp)
target_link_libraries(runtime_test PRIVATE novium_lib)
add_test(NAME RuntimeTest COMMAND runtime_test)

add_executable(test_harness tests/framework/test_harness.cpp tests/test_main.cpp)
target_include_directories(test_harness PRIVATE tests)
target_link_libraries(test_harness PRIVATE novium_lib)
add_test(NAME TestHarness COMMAND test_harness)

# ── Cross-Language Build Integration ────────────────────────────────────────
# Novium can be integrated with other languages in several ways:

# 1. CMake + Novium: Link Novium-compiled libraries with C/C++ projects
#    add_subdirectory(novium)  # After building novium compiler
#    target_link_libraries(my_c_app PRIVATE novium_lib)
#    # Use: extern "C" { fn novium_func() } from C code

# 2. CMake presets for mixed-language projects:
#    # Build Novium .nvm first, then link
#    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/novium-build)
#    cd ${CMAKE_BINARY_DIR}/novium-build
#    cmake -DNOVIUM_OUTPUT=wasm ../novium  # or: cmake -DNOVIUM_OUTPUT=native
#    make
#    # Output: novium.nvm.so (shared lib) or novium.wasm

# 3. Python integration via .nvi:
#    # Novium scripts can be run from Python:
#    import subprocess
#    result = subprocess.run(["novium", "run", "script.nvi"], capture_output=True)
#    # Or import compiled .nvm as shared library:
#    import ctypes
#    lib = ctypes.CDLL("./build/novium/libhelloworld.nvm.so")

# 4. Wasm deployment workflow:
#    # Build for web: novium build --target wasm fibonacci.nvm -o static.wasm
#    # Embed in HTML with JS glue:
#    # <script src="static.wasm"></script>
#    # <script>init().then(() => { console.log(nov.fib(10)) })</script>

# 5. IDE/Editor integration:
#    # Language server protocol (LSP) for Novium code completion
#    # .nvm, .nvi, .nvw files get syntax highlighting, type diagnostics
#    # Can be configured in VS Code, Neovim, Emacs

# 6. Package manager integration (future):
#    # novium pkg install <name>  # from novium registry
#    # #novium pkg list  # show installed packages
#    # #novium pkg search "web framework"

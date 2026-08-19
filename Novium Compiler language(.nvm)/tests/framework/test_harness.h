// ============================================================================
// test_harness.h — Novium Test Framework Harness
// ============================================================================
//
// Provides a self-registering test framework for Novium compiler testing:
// - TEST_CASE(name) auto-registration (no manual main bookkeeping)
// - Exception-based assertions with file:line failure reporting
// - Filtering (--filter <substr>) and listing (--list) support
// - Per-test timing and summary reporting
// - Property-based testing helper
//
// Usage:
//   #include "framework/test_harness.h"
//
//   TEST_CASE(my_test) {
//       int x = 1 + 1;
//       TEST_CHECK(x == 2);
//       TEST_CHECK_EQ(2, x);
//   }
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <functional>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"

namespace novium::test {

// ============================================================================
// Test Failure
// ============================================================================

// Thrown by assertions; carries the failure location for reporting.
class TestFailure : public std::runtime_error {
public:
    TestFailure(const std::string& message, const std::string& file, int line)
        : std::runtime_error(message), file_(file), line_(line) {}

    const std::string& file() const { return file_; }
    int line() const { return line_; }

private:
    std::string file_;
    int line_;
};

// ============================================================================
// Assertion Macros
// ============================================================================

// Internal: throw a TestFailure with the given location
[[noreturn]] inline void fail_impl(const std::string& message,
                                   const std::string& file, int line) {
    throw TestFailure(message, file, line);
}

#define TEST_FAIL(message) \
    ::novium::test::fail_impl((message), __FILE__, __LINE__)

#define TEST_CHECK(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream _msg; \
            _msg << "CHECK failed: " << #condition; \
            ::novium::test::fail_impl(_msg.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_CHECK_EQ(expected, actual) \
    do { \
        auto _expected = (expected); \
        auto _actual = (actual); \
        if (!(_expected == _actual)) { \
            std::ostringstream _msg; \
            _msg << "CHECK_EQ failed: " << #expected << " == " << #actual \
                 << " (actual: " << _actual << ", expected: " << _expected << ")"; \
            ::novium::test::fail_impl(_msg.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_CHECK_NE(left, right) \
    do { \
        auto _left = (left); \
        auto _right = (right); \
        if (_left == _right) { \
            std::ostringstream _msg; \
            _msg << "CHECK_NE failed: " << #left << " != " << #right \
                 << " (both: " << _left << ")"; \
            ::novium::test::fail_impl(_msg.str(), __FILE__, __LINE__); \
        } \
    } while (0)

// Assert that a piece of code throws (any exception)
#define TEST_CHECK_THROWS(expr) \
    do { \
        bool _threw = false; \
        try { \
            (void)(expr); \
        } catch (...) { \
            _threw = true; \
        } \
        if (!_threw) { \
            std::ostringstream _msg; \
            _msg << "CHECK_THROWS failed: " << #expr << " did not throw"; \
            ::novium::test::fail_impl(_msg.str(), __FILE__, __LINE__); \
        } \
    } while (0)

// Assert that a piece of code throws a TestFailure/exception whose
// message contains the given substring
#define TEST_CHECK_THROWS_WITH(expr, expected_substring) \
    do { \
        bool _threw = false; \
        try { \
            (void)(expr); \
        } catch (const std::exception& _ex) { \
            _threw = true; \
            if (std::string(_ex.what()).find(expected_substring) == std::string::npos) { \
                std::ostringstream _msg; \
                _msg << "CHECK_THROWS_WITH failed: exception message \"" \
                     << _ex.what() << "\" does not contain \"" \
                     << expected_substring << "\""; \
                ::novium::test::fail_impl(_msg.str(), __FILE__, __LINE__); \
            } \
        } catch (...) { \
            _threw = true; \
        } \
        if (!_threw) { \
            std::ostringstream _msg; \
            _msg << "CHECK_THROWS_WITH failed: " << #expr << " did not throw"; \
            ::novium::test::fail_impl(_msg.str(), __FILE__, __LINE__); \
        } \
    } while (0)

// ============================================================================
// Test Case Registration
// ============================================================================

using TestFn = std::function<void()>;

struct TestCase {
    std::string name;
    std::string file;
    int line;
    TestFn fn;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void add(TestCase test_case) {
        tests_.push_back(std::move(test_case));
    }

    std::vector<TestCase>& tests() { return tests_; }
    const std::vector<TestCase>& tests() const { return tests_; }

private:
    std::vector<TestCase> tests_;
};

class TestRegistrar {
public:
    TestRegistrar(const std::string& name, TestFn fn, const std::string& file, int line) {
        TestRegistry::instance().add(TestCase{name, file, line, std::move(fn)});
    }
};

// Register a test function for automatic discovery:
//   TEST_CASE(my_test_name) { ...body... }
#define TEST_CASE(name) \
    static void name(); \
    static ::novium::test::TestRegistrar _novium_test_reg_##name( \
        #name, &name, __FILE__, __LINE__); \
    static void name()

// ============================================================================
// Test Runner
// ============================================================================

struct TestRunResult {
    int total = 0;
    int passed = 0;
    int failed = 0;
    double total_seconds = 0.0;
};

class TestRunner {
public:
    explicit TestRunner(const std::string& filter = "") : filter_(filter) {}

    // Run all tests (optionally filtered); returns counts and wall time
    TestRunResult run() {
        TestRunResult result;
        const auto start = std::chrono::steady_clock::now();

        for (const auto& test_case : TestRegistry::instance().tests()) {
            if (!filter_.empty() &&
                test_case.name.find(filter_) == std::string::npos) {
                continue;
            }
            ++result.total;
            try {
                test_case.fn();
                ++result.passed;
                std::cout << "  PASS: " << test_case.name << "\n";
            } catch (const TestFailure& failure) {
                ++result.failed;
                std::cout << "  FAIL: " << test_case.name
                          << " (" << failure.file() << ":" << failure.line() << ")\n"
                          << "        " << failure.what() << "\n";
            } catch (const std::exception& ex) {
                ++result.failed;
                std::cout << "  ERROR: " << test_case.name
                          << " - unexpected exception: " << ex.what() << "\n";
            } catch (...) {
                ++result.failed;
                std::cout << "  ERROR: " << test_case.name
                          << " - unknown exception\n";
            }
        }

        const auto end = std::chrono::steady_clock::now();
        result.total_seconds =
            std::chrono::duration<double>(end - start).count();
        return result;
    }

    // List registered tests without running them
    static void list() {
        for (const auto& test_case : TestRegistry::instance().tests()) {
            std::cout << "  " << test_case.name
                      << " (" << test_case.file << ":" << test_case.line << ")\n";
        }
    }

    // Print a summary of results
    static void summary(const TestRunResult& result) {
        std::cout << "\n═══════ Test Results ═══════\n";
        std::cout << "Total:  " << result.total << "\n";
        std::cout << "Passed: " << result.passed << "\n";
        std::cout << "Failed: " << result.failed << "\n";
        std::cout << "Time:   " << result.total_seconds << "s\n";
        if (result.failed > 0) {
            std::cout << "\n" << result.failed << " test(s) failed.\n";
        } else {
            std::cout << "\nAll tests passed!\n";
        }
    }

private:
    std::string filter_;
};

// ============================================================================
// Common Test Utilities
// ============================================================================

// Parse and type-check a Novium source string
inline std::vector<std::unique_ptr<novium::Stmt>> parse_code(
    const std::string& source, bool& has_errors) {
    novium::Lexer lexer(source, "test.nvm");
    auto tokens = lexer.tokenize();
    novium::Parser parser(tokens);
    auto program = parser.parse_program();
    has_errors = parser.has_errors();
    return program;
}

// Print AST for test output (whole program)
inline std::string dump_ast(
    const std::vector<std::unique_ptr<novium::Stmt>>& program) {
    std::stringstream ss;
    novium::ASTPrinter printer(ss);
    for (const auto& stmt : program) {
        printer.print(stmt.get());
    }
    return ss.str();
}

// Print AST for test output (single node)
inline std::string dump_ast(const novium::ASTNode* node) {
    std::stringstream ss;
    novium::ASTPrinter printer(ss);
    printer.print(const_cast<novium::ASTNode*>(node));
    return ss.str();
}

// ============================================================================
// Property-Based Testing Helper
// ============================================================================

// Run a property function repeatedly; returns false on the first failure.
template <typename PropertyFn>
bool run_property(PropertyFn property, int min_size = 1, int max_size = 50,
                  int iterations = 100) {
    (void)min_size;
    (void)max_size;
    for (int i = 0; i < iterations; ++i) {
        if (!property()) {
            return false;
        }
    }
    return true;
}

} // namespace novium::test
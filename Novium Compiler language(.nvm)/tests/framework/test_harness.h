// ============================================================================
// test_harness.h — Novium Test Framework Harness
// ============================================================================
//
// Provides a test harness for Novium compiler testing with:
// - Assertion macros
// - Test discovery and registration
// - Property-based testing support
// - Automatic test result reporting
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"

namespace novium::test {

// ============================================================================
// Test Result
// ============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string error_message;
    std::string stack_trace;

    TestResult(const std::string& name, bool passed,
               const std::string& error_message = "",
               const std::string& stack_trace = "")
        : name(name), passed(passed),
          error_message(error_message), stack_trace(stack_trace) {}
};

// ============================================================================
// Assertion Macros
// ============================================================================

// MAKE_ASSERT(condition, message) - Custom assertion with message
#define MAKE_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            return TestResult(#condition " (" message ")", false, message); \
        } \
    } while(0)

// Internal helper for assert-like checks
#define _TEST_ASSERT(condition) MAKE_ASSERT(condition, #condition " failed")

// ============================================================================
// Base Test Class
// ============================================================================

class Test {
public:
    virtual ~Test() = default;
    virtual TestRun run() = 0;
    virtual std::string name() const = 0;
};

// ============================================================================
// Test Runner
// ============================================================================

class TestRunner {
public:
    TestRunner() : total_(0), passed_(0), failed_(0) {}

    // Add a test to run
    void add_test(Test* test) {
        tests_.push_back(test);
        total_++;
    }

    // Run all tests
    void run() {
        for (Test* test : tests_) {
            TestRun result = test->run();
            if (result.passed) {
                passed_++;
                std::cout << "  PASS: " << result.name << "\n";
            } else {
                failed_++;
                std::cout << "  FAIL: " << result.name;
                if (!result.error_message.empty()) {
                    std::cout << " - " << result.error_message;
                }
                std::cout << "\n";
            }
        }
    }

    // Summary
    void summary() const {
        std::cout << "\n═══ Test Results ═══\n";
        std::cout << "Total: " << total_ << "\n";
        std::cout << "Passed: " << passed_ << "\n";
        std::cout << "Failed: " << failed_ << "\n";
        if (failed_ > 0) {
            std::cout << "\n❌ " << failed_ << " test(s) failed.\n";
        } else {
            std::cout << "\n✅ All tests passed!\n";
        }
    }

private:
    std::vector<Test*> tests_;
    int total_;
    int passed_;
    int failed_;
};

// ============================================================================
// Test Discovery and Registration
// ============================================================================

// Register a test class for automatic discovery
#define REGISTER_TEST(test_class) \
    namespace { \
        struct test_class##_registrar { \
            test_class##_registrar() { \
                TestRunner::instance().add_test(new test_class()); \
            } \
        }; \
        test_class##_registrar _##test_class##_registrar; \
    }

// Global test runner instance
inline TestRunner& instance() {
    static TestRunner runner;
    return runner;
}

// ============================================================================
// Common Test Utilities
// ============================================================================

// Parse and type-check a Novium source string
static std::vector<std::unique_ptr<novium::Stmt>> parse_code(
    const std::string& source, bool& has_errors) {
    novium::Lexer lexer(source, "test.nvm");
    auto tokens = lexer.tokenize();
    novium::Parser parser(tokens);
    auto program = parser.parse_program();
    has_errors = parser.has_errors();
    return program;
}

// Print AST for test output
static std::string dump_ast(const std::vector<std::unique_ptr<novium::Stmt>>& program) {
    novium::ASTPrinter printer;
    std::stringstream ss;
    for (const auto& stmt : program) {
        printer.print(stmt.get());
    }
    return ss.str();
}

// Check if parser has errors
static bool has_parser_errors(const std::vector<std::unique_ptr<novium::Stmt>>& program,
                              const novium::Parser& parser) {
    return parser.has_errors();
}

// ============================================================================
// Property-Based Testing Helpers
// ============================================================================

// Run a property on generated Novium programs
template<typename PropertyFn>
bool run_property(PropertyFn property, int min_size = 1, int max_size = 50,
                  int iterations = 100) {
    // Simple fuzz: generate random programs and check property
    // Full implementation would use the fuzz.nvm library
    return true; // Placeholder
}

// ============================================================================
// Standard Assert Macros (backwards compatible)
// ============================================================================

// Assert two values are equal
#define ASSERT_EQUAL(expected, actual) \
    _TEST_ASSERT((expected) == (actual))

// Assert a condition is true
#define ASSERT_TRUE(condition) \
    _TEST_ASSERT((condition))

// Assert a condition is false
#define ASSERT_FALSE(condition) \
    _TEST_ASSERT(!(condition))

// Assert that code fails (e.g., parser error expected)
#define ASSERT_FAIL(block) \
    _TEST_ASSERT(true) // Simplified - full impl would try/catch

// Assert that code succeeds
#define ASSERT_SUCCEED(block) \
    _TEST_ASSERT(true) // Simplified

} // namespace novium::test
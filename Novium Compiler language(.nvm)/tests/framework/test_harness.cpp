// ============================================================================
// test_harness.cpp — Novium Test Framework Implementation
// ============================================================================

#include "framework/test_harness.h"
#include <iostream>

namespace novium::test {

// TestRunner implementation
TestRunner::TestRunner() : total_(0), passed_(0), failed_(0) {}

void TestRunner::add_test(Test* test) {
    tests_.push_back(test);
    total_++;
}

void TestRunner::run() {
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

void TestRunner::summary() const {
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

// Default test run result
TestRun::TestRun() : passed(false), name("unnamed"), error_message(""), stack_trace(") {}

} // namespace novium::test
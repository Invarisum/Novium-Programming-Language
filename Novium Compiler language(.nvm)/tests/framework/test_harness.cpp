// ============================================================================
// test_harness.cpp — Novium Test Framework Entry Point
// ============================================================================
// Runs all registered test cases (see TEST_CASE in test_harness.h).
//
// Usage:
//   test_harness                 run all tests
//   test_harness --list          list registered tests without running
//   test_harness --filter <sub>  run only tests whose name contains <sub>
//
// Exit code: 0 if all tests passed, 1 if any failed, 2 on bad arguments.
//
// ============================================================================

#include "framework/test_harness.h"

#include <iostream>

int main(int argc, char** argv) {
    bool list_only = false;
    std::string filter;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--list") {
            list_only = true;
        } else if (arg == "--filter" && i + 1 < argc) {
            filter = argv[++i];
        } else {
            std::cerr << "usage: test_harness [--list] [--filter <substring>]\n";
            return 2;
        }
    }

    if (list_only) {
        std::cout << "Registered test cases:\n";
        novium::test::TestRunner::list();
        return 0;
    }

    novium::test::TestRunner runner(filter);
    auto result = runner.run();
    novium::test::TestRunner::summary(result);
    return result.failed > 0 ? 1 : 0;
}
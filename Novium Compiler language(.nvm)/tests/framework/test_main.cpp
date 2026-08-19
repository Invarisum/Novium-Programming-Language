// ============================================================================
// test_main.cpp — Novium Test Framework Main
// ============================================================================
// Runs the full test harness including parser tests, lexer tests, and the
// new test framework assertions.
//
// ============================================================================

#include "framework/test_harness.h"

// ── Parser Tests ────────────────────────────────────────────────────────

// Test: Variable declarations
bool test_variable_declarations() {
    bool has_err = false;
    auto ast = parse_code(
        "let x = 5\n"
        "var name: string? = null\n", 
        has_err
    );
    if (has_err || ast.size() != 2) {
        return false;
    }
    
    // Check first declaration: let x = 5
    std::string print_out = novium::dump_ast(ast[0]);
    if (print_out.find("VarDeclStmt \"x\"") == std::string::npos ||
        print_out.find("Literal (INTEGER_LITERAL) \"5\"") == std::string::npos) {
        return false;
    }
    
    // Check second declaration: var name: string? = null
    std::string print_out2 = novium::dump_ast(ast[1]);
    if (print_out2.find("VarDeclStmt \"name\"") == std::string::npos ||
        print_out2.find("Type: string?") == std::string::npos ||
        print_out2.find("Literal (KW_NULL) \"null\"") == std::string::npos) {
        return false;
    }
    
    return true;
}

// Test: Expression precedence
bool test_expression_precedence() {
    bool has_err = false;
    auto ast = parse_code("let val = 1 + 2 * 3\n", has_err);
    if (has_err || ast.size() != 1) return false;
    
    std::string out = novium::dump_ast(ast[0]);
    size_t plus_pos = out.find("BinaryExpr (+)");
    size_t star_pos = out.find("BinaryExpr (*)");
    
    if (plus_pos == std::string::npos || star_pos == std::string::npos || plus_pos > star_pos) {
        return false;
    }
    return true;
}

// Test: Function declarations
bool test_function_declarations() {
    bool has_err = false;
    auto ast = parse_code(
        "fn add(a int, b int) int:\n"
        "    return a + b\n", 
        has_err
    );
    if (has_err || ast.size() != 1) return false;
    
    std::string out = novium::dump_ast(ast[0]);
    if (out.find("FunctionDeclStmt \"add\"") == std::string::npos ||
        out.find("a : int") == std::string::npos ||
        out.find("b : int") == std::string::npos ||
        out.find("ReturnType: int") == std::string::npos ||
        out.find("ReturnStmt") == std::string::npos) {
        return false;
    }
    return true;
}

// ── Conditional Tests ──────────────────────────────────────────────────

bool test_conditionals() {
    bool has_err = false;
    auto ast = parse_code(
        "if x > 0:\n"
        "    print(\"pos\")\n"
        "elif x == 0:\n"
        "    print(\"zero\")\n"
        "else:\n"
        "    print(\"neg\")\n", 
        has_err
    );
    if (has_err || ast.size() != 1) return false;
    
    std::string out = novium::dump_ast(ast[0]);
    if (out.find("IfStmt") == std::string::npos ||
        out.find("Condition: BinaryExpr (>)") == std::string::npos ||
        out.find("Elif Branch Condition: BinaryExpr (==)") == std::string::npos ||
        out.find("Else Branch:") == std::string::npos) {
        return false;
    }
    return true;
}

// Test: Pattern matching
bool test_pattern_matching() {
    bool has_err = false;
    auto ast = parse_code(
        "match status:\n"
        "    1 => print(\"active\")\n"
        "    _ => print(\"unknown\")\n", 
        has_err
    );
    if (has_err || ast.size() != 1) return false;
    
    std::string out = novium::dump_ast(ast[0]);
    if (out.find("MatchStmt") == std::string::npos ||
        out.find("Subject: Identifier \"status\"") == std::string::npos ||
        out.find("Pattern: Literal (INTEGER_LITERAL) \"1\"") == std::string::npos ||
        out.find("Pattern: Identifier \"_\"") == std::string::npos) {
        return false;
    }
    return true;
}

// Test: Exception blocks
bool test_exception_blocks() {
    bool has_err = false;
    auto ast = parse_code(
        "try:\n"
        "    let x = 1\n"
        "catch IOError as e:\n"
        "    print(e)\n"
        "finally:\n"
        "    clean()\n", 
        has_err
    );
    if (has_err || ast.size() != 1) return false;
    
    std::string out = novium::dump_ast(ast[0]);
    if (out.find("TryCatchStmt") == std::string::npos ||
        out.find("Catch Type: IOError Var: e:") == std::string::npos ||
        out.find("Finally Block:") == std::string::npos) {
        return false;
    }
    return true;
}

// ── Main ───────────────────────────────────────────────────────────────

int main() {
    std::cout << "\n═══ Novium Test Suite ═══\n\n";
    
    int tests_passed = 0;
    int tests_failed = 0;
    
    // Run parser tests
    std::cout << "\n-- Parser Tests --\n";
    
    if (test_variable_declarations()) {
        std::cout << "  PASS: test_variable_declarations\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: test_variable_declarations\n";
        tests_failed++;
    }
    
    if (test_expression_precedence()) {
        std::cout << "  PASS: test_expression_precedence\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: test_expression_precedence\n";
        tests_failed++;
    }
    
    if (test_function_declarations()) {
        std::cout << "  PASS: test_function_declarations\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: test_function_declarations\n";
        tests_failed++;
    }
    
    // Run conditional tests
    std::cout << "\n-- Conditional Tests --\n";
    
    if (test_conditionals()) {
        std::cout << "  PASS: test_conditionals\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: test_conditionals\n";
        tests_failed++;
    }
    
    if (test_pattern_matching()) {
        std::cout << "  PASS: test_pattern_matching\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: test_pattern_matching\n";
        tests_failed++;
    }
    
    if (test_exception_blocks()) {
        std::cout << "  PASS: test_exception_blocks\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: test_exception_blocks\n";
        tests_failed++;
    }
    
    // Summary
    std::cout << "\n═══ Results: " << tests_passed << " passed, "
              << tests_failed << " failed ═══\n\n";
    
    return tests_failed > 0 ? 1 : 0;
}
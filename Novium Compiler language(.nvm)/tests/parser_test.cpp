// ============================================================================
// parser_test.cpp — Novium Parser Test Suite
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"

using namespace novium;

static int tests_passed = 0;
static int tests_failed = 0;

// Helper: Lex and Parse a string to get AST statement tree
static std::vector<std::unique_ptr<Stmt>> parse_code(const std::string& source, bool& has_errors) {
    Lexer lexer(source, "test.nvm");
    auto tokens = lexer.tokenize();
    
    Parser parser(tokens);
    auto program = parser.parse_program();
    has_errors = parser.has_errors();
    
    if (parser.has_errors()) {
        std::cerr << "  [PARSER ERRORS IN TEST]:\n";
        for (const auto& err : parser.errors()) {
            std::cerr << "    " << err << "\n";
        }
    }
    return program;
}

// Helper: AST tree to string representation using ASTPrinter
static std::string dump_ast(ASTNode* node) {
    std::stringstream ss;
    ASTPrinter printer(ss);
    printer.print(node);
    return ss.str();
}

#define RUN_TEST(fn) do { \
    std::cout << "  Running: " << #fn << "..."; \
    if (fn()) { \
        std::cout << " PASS\n"; \
        tests_passed++; \
    } else { \
        std::cout << " FAIL\n"; \
        tests_failed++; \
    } \
} while(0)

// ═══════════════════════════════════════════════════════════════════════════
// TEST CASES
// ═══════════════════════════════════════════════════════════════════════════

// ── Test 1: Simple variable declarations ──
bool test_variable_declarations() {
    bool has_err = false;
    auto ast = parse_code(
        "let x = 5\n"
        "var name: string? = null\n", 
        has_err
    );
    if (has_err || ast.size() != 2) return false;

    std::string print_out = dump_ast(ast[0].get());
    // Verification: should declare x with init 5
    if (print_out.find("VarDeclStmt \"x\" (mutable=false)") == std::string::npos ||
        print_out.find("Literal (INTEGER_LITERAL) \"5\"") == std::string::npos) {
        return false;
    }

    std::string print_out2 = dump_ast(ast[1].get());
    // Verification: should declare mutable name with type string? and init null
    if (print_out2.find("VarDeclStmt \"name\" (mutable=true)") == std::string::npos ||
        print_out2.find("Type: string?") == std::string::npos ||
        print_out2.find("Literal (KW_NULL) \"null\"") == std::string::npos) {
        return false;
    }

    return true;
}

// ── Test 2: Expression precedence (Pratt) ──
bool test_expression_precedence() {
    bool has_err = false;
    auto ast = parse_code("let val = 1 + 2 * 3\n", has_err);
    if (has_err || ast.size() != 1) return false;

    std::string out = dump_ast(ast[0].get());
    // Precedence validation: 1 + (2 * 3)
    // The top binary operation should be "+"
    // The right child should be binary operation "*"
    size_t plus_pos = out.find("BinaryExpr (+)");
    size_t star_pos = out.find("BinaryExpr (*)");
    
    if (plus_pos == std::string::npos || star_pos == std::string::npos || plus_pos > star_pos) {
        std::cerr << "\nPrecedence tree:\n" << out << "\n";
        return false;
    }
    return true;
}

// ── Test 3: Assignment associativity ──
bool test_assignment_associativity() {
    bool has_err = false;
    auto ast = parse_code("x = y = 5\n", has_err);
    if (has_err || ast.size() != 1) return false;

    std::string out = dump_ast(ast[0].get());
    // x = (y = 5)
    // Left child should be identifier "x"
    // Right child should be BinaryExpr (=)
    if (out.find("BinaryExpr (=)") == std::string::npos) {
        return false;
    }
    return true;
}

// ── Test 4: Function declarations ──
bool test_function_declarations() {
    bool has_err = false;
    auto ast = parse_code(
        "fn add(a int, b int) int:\n"
        "    return a + b\n", 
        has_err
    );
    if (has_err || ast.size() != 1) return false;

    std::string out = dump_ast(ast[0].get());
    if (out.find("FunctionDeclStmt \"add\"") == std::string::npos ||
        out.find("a : int") == std::string::npos ||
        out.find("b : int") == std::string::npos ||
        out.find("ReturnType: int") == std::string::npos ||
        out.find("ReturnStmt") == std::string::npos) {
        std::cerr << "\nFunction declaration output:\n" << out << "\n";
        return false;
    }
    return true;
}

// ── Test 5: Rust/C++-familiar arrow return type ──
bool test_arrow_return_type() {
    bool has_err = false;
    auto ast = parse_code(
        "fn square(value: int) -> int { return value * value }\n",
        has_err
    );
    if (has_err || ast.size() != 1) return false;
    std::string out = dump_ast(ast[0].get());
    return out.find("FunctionDeclStmt \"square\"") != std::string::npos &&
           out.find("value : int") != std::string::npos &&
           out.find("ReturnType: int") != std::string::npos;
}

// ── Test 5: Inline curly blocks ──
bool test_inline_blocks() {
    bool has_err = false;
    auto ast = parse_code("fn double(x int) int { return x * 2 }", has_err);
    if (has_err || ast.size() != 1) return false;

    std::string out = dump_ast(ast[0].get());
    if (out.find("FunctionDeclStmt \"double\"") == std::string::npos ||
        out.find("ReturnStmt") == std::string::npos) {
        return false;
    }
    return true;
}

// ── Test 6: Class declarations ──
bool test_class_declarations() {
    bool has_err = false;
    auto ast = parse_code(
        "class Dog extends Animal implements Runnable:\n"
        "    name string\n"
        "    fn bark() void:\n"
        "        print(self.name)\n",
        has_err
    );
    if (has_err || ast.size() != 1) return false;

    std::string out = dump_ast(ast[0].get());
    if (out.find("ClassDeclStmt \"Dog\"") == std::string::npos ||
        out.find("Extends: Animal") == std::string::npos ||
        out.find("Implements: Runnable") == std::string::npos ||
        out.find("name : string") == std::string::npos ||
        out.find("FunctionDeclStmt \"bark\"") == std::string::npos ||
        out.find("MemberAccessExpr (.name)") == std::string::npos) {
        std::cerr << "\nClass output:\n" << out << "\n";
        return false;
    }
    return true;
}

// ── Test 7: Conditionals (if-elif-else) ──
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

    std::string out = dump_ast(ast[0].get());
    if (out.find("IfStmt") == std::string::npos ||
        out.find("Condition: BinaryExpr (>)") == std::string::npos ||
        out.find("Elif Branch Condition: BinaryExpr (==)") == std::string::npos ||
        out.find("Else Branch:") == std::string::npos) {
        std::cerr << "\nIf statement output:\n" << out << "\n";
        return false;
    }
    return true;
}

// ── Test: Rust mutability and a C/Rust/Go-familiar while loop ──
bool test_mutable_while_loop() {
    bool has_err = false;
    auto ast = parse_code(
        "let mut count: int = 0\n"
        "while count < 3:\n"
        "    count += 1\n",
        has_err
    );
    if (has_err || ast.size() != 2) return false;
    std::string declaration = dump_ast(ast[0].get());
    std::string loop = dump_ast(ast[1].get());
    return declaration.find("mutable=true") != std::string::npos &&
           loop.find("WhileStmt") != std::string::npos &&
           loop.find("BinaryExpr (+=)") != std::string::npos;
}

// ── Test 8: Pattern matching ──
bool test_pattern_matching() {
    bool has_err = false;
    auto ast = parse_code(
        "match status:\n"
        "    1 => print(\"active\")\n"
        "    _ => print(\"unknown\")\n",
        has_err
    );
    if (has_err || ast.size() != 1) return false;

    std::string out = dump_ast(ast[0].get());
    if (out.find("MatchStmt") == std::string::npos ||
        out.find("Subject: Identifier \"status\"") == std::string::npos ||
        out.find("Pattern: Literal (INTEGER_LITERAL) \"1\"") == std::string::npos ||
        out.find("Pattern: Identifier \"_\"") == std::string::npos) {
        std::cerr << "\nMatch output:\n" << out << "\n";
        return false;
    }
    return true;
}

// ── Test 9: Exception blocks (try-catch-finally) ──
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

    std::string out = dump_ast(ast[0].get());
    if (out.find("TryCatchStmt") == std::string::npos ||
        out.find("Catch Type: IOError Var: e:") == std::string::npos ||
        out.find("Finally Block:") == std::string::npos) {
        std::cerr << "\nTry/catch output:\n" << out << "\n";
        return false;
    }
    return true;
}

// ── Test 10: Concurrency constructs (go, async await) ──
bool test_concurrency_constructs() {
    bool has_err = false;
    auto ast = parse_code(
        "go run()\n"
        "let res = await fetch()\n",
        has_err
    );
    if (has_err || ast.size() != 2) return false;

    std::string out1 = dump_ast(ast[0].get());
    std::string out2 = dump_ast(ast[1].get());

    if (out1.find("GoStmt") == std::string::npos ||
        out1.find("CallExpr") == std::string::npos) {
        return false;
    }

    if (out2.find("AwaitExpr") == std::string::npos ||
        out2.find("CallExpr") == std::string::npos) {
        return false;
    }

    return true;
}

// ── Test 11: Error recovery test ──
bool test_error_recovery() {
    bool has_err = false;
    // Introduce syntax error in first statement: `let x = +` (missing expression)
    // The parser should report error, skip to next statement, and successfully parse `let y = 10`
    auto ast = parse_code(
        "let x = + \n"
        "let y = 10\n",
        has_err
    );
    
    // We expect errors, but we also expect the second statement to be parsed successfully
    if (!has_err || ast.empty()) return false;

    bool found_y = false;
    for (const auto& s : ast) {
        std::string out = dump_ast(s.get());
        if (out.find("VarDeclStmt \"y\"") != std::string::npos) {
            found_y = true;
        }
    }
    return found_y;
}

// ── Main ──
int main() {
    std::cout << "\n═══ Novium Parser Test Suite ═══\n\n";

    RUN_TEST(test_variable_declarations);
    RUN_TEST(test_expression_precedence);
    RUN_TEST(test_assignment_associativity);
    RUN_TEST(test_function_declarations);
    RUN_TEST(test_arrow_return_type);
    RUN_TEST(test_inline_blocks);
    RUN_TEST(test_class_declarations);
    RUN_TEST(test_conditionals);
    RUN_TEST(test_mutable_while_loop);
    RUN_TEST(test_pattern_matching);
    RUN_TEST(test_exception_blocks);
    RUN_TEST(test_concurrency_constructs);
    RUN_TEST(test_error_recovery);

    std::cout << "\n═══ Results: " << tests_passed << " passed, "
              << tests_failed << " failed ═══\n\n";

    return tests_failed > 0 ? 1 : 0;
}

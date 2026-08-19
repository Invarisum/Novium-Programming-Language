// ============================================================================
// test_main.cpp — Novium Parser Test Cases (framework style)
// ============================================================================
// Parser coverage tests written against the self-registering test
// framework. Compiled into the test_harness binary.
//
// ============================================================================

#include "framework/test_harness.h"

using novium::test::parse_code;
using novium::test::dump_ast;

// ── Variable declarations ──────────────────────────────────────────────────

TEST_CASE(parser_variable_declarations) {
    bool has_err = false;
    auto ast = parse_code(
        "let x = 5\n"
        "var name: string? = null\n",
        has_err
    );
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(2), ast.size());

    // First declaration: let x = 5 (immutable)
    std::string print_out = dump_ast(ast[0].get());
    TEST_CHECK(print_out.find("VarDeclStmt \"x\" (mutable=false)") != std::string::npos);
    TEST_CHECK(print_out.find("Literal (INTEGER_LITERAL) \"5\"") != std::string::npos);

    // Second declaration: var name: string? = null (mutable)
    std::string print_out2 = dump_ast(ast[1].get());
    TEST_CHECK(print_out2.find("VarDeclStmt \"name\" (mutable=true)") != std::string::npos);
    TEST_CHECK(print_out2.find("Type: string?") != std::string::npos);
    TEST_CHECK(print_out2.find("Literal (KW_NULL) \"null\"") != std::string::npos);
}

// ── Expression precedence (Pratt) ──────────────────────────────────────────

TEST_CASE(parser_expression_precedence) {
    bool has_err = false;
    auto ast = parse_code("let val = 1 + 2 * 3\n", has_err);
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    // Precedence validation: 1 + (2 * 3); the top operator is "+"
    size_t plus_pos = out.find("BinaryExpr (+)");
    size_t star_pos = out.find("BinaryExpr (*)");
    TEST_CHECK(plus_pos != std::string::npos);
    TEST_CHECK(star_pos != std::string::npos);
    TEST_CHECK(plus_pos < star_pos);
}

// ── Assignment associativity ───────────────────────────────────────────────

TEST_CASE(parser_assignment_associativity) {
    bool has_err = false;
    auto ast = parse_code("x = y = 5\n", has_err);
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    // x = (y = 5): right child should be another BinaryExpr (=)
    TEST_CHECK(out.find("BinaryExpr (=)") != std::string::npos);
}

// ── Function declarations ──────────────────────────────────────────────────

TEST_CASE(parser_function_declarations) {
    bool has_err = false;
    auto ast = parse_code(
        "fn add(a int, b int) int:\n"
        "    return a + b\n",
        has_err
    );
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    TEST_CHECK(out.find("FunctionDeclStmt \"add\"") != std::string::npos);
    TEST_CHECK(out.find("a : int") != std::string::npos);
    TEST_CHECK(out.find("b : int") != std::string::npos);
    TEST_CHECK(out.find("ReturnType: int") != std::string::npos);
    TEST_CHECK(out.find("ReturnStmt") != std::string::npos);
}

// ── Arrow return type ──────────────────────────────────────────────────────

TEST_CASE(parser_arrow_return_type) {
    bool has_err = false;
    auto ast = parse_code(
        "fn square(value: int) -> int { return value * value }\n",
        has_err
    );
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    TEST_CHECK(out.find("FunctionDeclStmt \"square\"") != std::string::npos);
    TEST_CHECK(out.find("value : int") != std::string::npos);
    TEST_CHECK(out.find("ReturnType: int") != std::string::npos);
}

// ── Inline curly blocks ────────────────────────────────────────────────────

TEST_CASE(parser_inline_blocks) {
    bool has_err = false;
    auto ast = parse_code("fn double(x int) int { return x * 2 }", has_err);
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    TEST_CHECK(out.find("FunctionDeclStmt \"double\"") != std::string::npos);
    TEST_CHECK(out.find("ReturnStmt") != std::string::npos);
}

// ── Class declarations ─────────────────────────────────────────────────────

TEST_CASE(parser_class_declarations) {
    bool has_err = false;
    auto ast = parse_code(
        "class Dog extends Animal implements Runnable:\n"
        "    name string\n"
        "    fn bark() void:\n"
        "        print(self.name)\n",
        has_err
    );
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    TEST_CHECK(out.find("ClassDeclStmt \"Dog\"") != std::string::npos);
    TEST_CHECK(out.find("Extends: Animal") != std::string::npos);
    TEST_CHECK(out.find("Implements: Runnable") != std::string::npos);
    TEST_CHECK(out.find("name : string") != std::string::npos);
    TEST_CHECK(out.find("FunctionDeclStmt \"bark\"") != std::string::npos);
    TEST_CHECK(out.find("MemberAccessExpr (.name)") != std::string::npos);
}

// ── Conditionals (if-elif-else) ────────────────────────────────────────────

TEST_CASE(parser_conditionals) {
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
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    TEST_CHECK(out.find("IfStmt") != std::string::npos);
    TEST_CHECK(out.find("Condition: BinaryExpr (>)") != std::string::npos);
    TEST_CHECK(out.find("Elif Branch Condition: BinaryExpr (==)") != std::string::npos);
    TEST_CHECK(out.find("Else Branch:") != std::string::npos);
}

// ── Rust-style mutability and while loop ───────────────────────────────────

TEST_CASE(parser_mutable_while_loop) {
    bool has_err = false;
    auto ast = parse_code(
        "let mut count: int = 0\n"
        "while count < 3:\n"
        "    count += 1\n",
        has_err
    );
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(2), ast.size());

    std::string declaration = dump_ast(ast[0].get());
    std::string loop = dump_ast(ast[1].get());
    TEST_CHECK(declaration.find("mutable=true") != std::string::npos);
    TEST_CHECK(loop.find("WhileStmt") != std::string::npos);
    TEST_CHECK(loop.find("BinaryExpr (+=)") != std::string::npos);
}

// ── Pattern matching ───────────────────────────────────────────────────────

TEST_CASE(parser_pattern_matching) {
    bool has_err = false;
    auto ast = parse_code(
        "match status:\n"
        "    1 => print(\"active\")\n"
        "    _ => print(\"unknown\")\n",
        has_err
    );
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    TEST_CHECK(out.find("MatchStmt") != std::string::npos);
    TEST_CHECK(out.find("Subject: Identifier \"status\"") != std::string::npos);
    TEST_CHECK(out.find("Pattern: Literal (INTEGER_LITERAL) \"1\"") != std::string::npos);
    TEST_CHECK(out.find("Pattern: Identifier \"_\"") != std::string::npos);
}

// ── Exception blocks (try-catch-finally) ───────────────────────────────────

TEST_CASE(parser_exception_blocks) {
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
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(1), ast.size());

    std::string out = dump_ast(ast[0].get());
    TEST_CHECK(out.find("TryCatchStmt") != std::string::npos);
    TEST_CHECK(out.find("Catch Type: IOError Var: e:") != std::string::npos);
    TEST_CHECK(out.find("Finally Block:") != std::string::npos);
}

// ── Concurrency constructs (go, async await) ───────────────────────────────

TEST_CASE(parser_concurrency_constructs) {
    bool has_err = false;
    auto ast = parse_code(
        "go run()\n"
        "let res = await fetch()\n",
        has_err
    );
    TEST_CHECK(!has_err);
    TEST_CHECK_EQ(static_cast<size_t>(2), ast.size());

    std::string out1 = dump_ast(ast[0].get());
    TEST_CHECK(out1.find("GoStmt") != std::string::npos);
    TEST_CHECK(out1.find("CallExpr") != std::string::npos);

    std::string out2 = dump_ast(ast[1].get());
    TEST_CHECK(out2.find("AwaitExpr") != std::string::npos);
    TEST_CHECK(out2.find("CallExpr") != std::string::npos);
}

// ── Error recovery ─────────────────────────────────────────────────────────

TEST_CASE(parser_error_recovery) {
    bool has_err = false;
    // `let x = +` is a syntax error; the parser should report it, skip the
    // statement, and still successfully parse `let y = 10`.
    auto ast = parse_code(
        "let x = + \n"
        "let y = 10\n",
        has_err
    );
    TEST_CHECK(has_err);
    TEST_CHECK(!ast.empty());

    bool found_y = false;
    for (const auto& s : ast) {
        std::string out = dump_ast(s.get());
        if (out.find("VarDeclStmt \"y\"") != std::string::npos) {
            found_y = true;
        }
    }
    TEST_CHECK(found_y);
}
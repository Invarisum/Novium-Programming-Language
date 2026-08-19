// ============================================================================
// lexer_test.cpp — Novium Lexer Test Suite
// ============================================================================
//
// This is a lightweight test framework — no external dependencies needed.
// Each test function tokenizes a string of Novium source code and checks
// that the resulting token types match expectations.
//
// HOW TO READ THESE TESTS:
// Each test has a descriptive name, source code input, and expected tokens.
// If a test fails, it prints what was expected vs what was found.
//
// TO RUN:
//   cd build && cmake .. && make && ctest --verbose
//   (or just run the lexer_test binary directly)
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "lexer/lexer.h"

using namespace novium;

// ── Test Infrastructure ──────────────────────────────────────────────────

static int tests_passed = 0;
static int tests_failed = 0;

// Helper: tokenize source and return the token vector
static std::vector<Token> lex(const std::string& source) {
    Lexer lexer(source, "test.nvm");
    return lexer.tokenize();
}

// Helper: check that token types match expected sequence
static bool check_types(const std::vector<Token>& tokens,
                        const std::vector<TokenType>& expected,
                        const std::string& test_name) {
    bool pass = true;

    if (tokens.size() != expected.size()) {
        std::cerr << "  FAIL [" << test_name << "]: Expected "
                  << expected.size() << " tokens, got " << tokens.size() << "\n";

        // Print what we got for debugging
        std::cerr << "  Got tokens:\n";
        for (size_t i = 0; i < tokens.size(); i++) {
            std::cerr << "    [" << i << "] " << token_type_to_string(tokens[i].type)
                      << " \"" << tokens[i].value << "\"\n";
        }
        return false;
    }

    for (size_t i = 0; i < expected.size(); i++) {
        if (tokens[i].type != expected[i]) {
            std::cerr << "  FAIL [" << test_name << "] at token " << i
                      << ": expected " << token_type_to_string(expected[i])
                      << ", got " << token_type_to_string(tokens[i].type)
                      << " (\"" << tokens[i].value << "\")\n";
            pass = false;
        }
    }

    return pass;
}

// Macro for running a test function
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
// TESTS
// ═══════════════════════════════════════════════════════════════════════════

// ── Test 1: Simple keyword and identifier ──
bool test_keyword_vs_identifier() {
    auto tokens = lex("fn main");
    return check_types(tokens, {
        TokenType::KW_FN,
        TokenType::IDENTIFIER,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "keyword_vs_identifier");
}

// ── Test 2: Integer literals ──
bool test_integer_literals() {
    auto tokens = lex("42 0xFF 0b1010");
    return check_types(tokens, {
        TokenType::INTEGER_LITERAL,
        TokenType::INTEGER_LITERAL,
        TokenType::INTEGER_LITERAL,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "integer_literals");
}

// ── Test 3: Float literals ──
bool test_float_literals() {
    auto tokens = lex("3.14 1.0e10 2.5e-3");
    return check_types(tokens, {
        TokenType::FLOAT_LITERAL,
        TokenType::FLOAT_LITERAL,
        TokenType::FLOAT_LITERAL,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "float_literals");
}

// ── Test 4: Simple string ──
bool test_simple_string() {
    auto tokens = lex("\"hello world\"");
    return check_types(tokens, {
        TokenType::STRING_LITERAL,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "simple_string");
}

// ── Test 5: String with interpolation ──
bool test_string_interpolation() {
    auto tokens = lex("\"Hello ${name}!\"");
    // Expected: STRING_START("Hello "), IDENTIFIER(name), STRING_END("!")
    return check_types(tokens, {
        TokenType::STRING_START,     // "Hello "
        TokenType::IDENTIFIER,       // name
        TokenType::STRING_END,       // "!"
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "string_interpolation");
}

// ── Test 6: Multiple interpolations ──
bool test_multiple_interpolations() {
    auto tokens = lex("\"${a} and ${b}\"");
    return check_types(tokens, {
        TokenType::STRING_START,     // ""
        TokenType::IDENTIFIER,       // a
        TokenType::STRING_MIDDLE,    // " and "
        TokenType::IDENTIFIER,       // b
        TokenType::STRING_END,       // ""
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "multiple_interpolations");
}

// ── Test 7: Operators ──
bool test_operators() {
    auto tokens = lex("+ - * / == != <= >= && || => ->");
    return check_types(tokens, {
        TokenType::PLUS,
        TokenType::MINUS,
        TokenType::STAR,
        TokenType::SLASH,
        TokenType::EQUAL_EQUAL,
        TokenType::BANG_EQUAL,
        TokenType::LESS_EQUAL,
        TokenType::GREATER_EQUAL,
        TokenType::AND_AND,
        TokenType::OR_OR,
        TokenType::FAT_ARROW,
        TokenType::ARROW,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "operators");
}

// ── Test 8: Compound assignment operators ──
bool test_compound_assignment() {
    auto tokens = lex("+= -= *= /=");
    return check_types(tokens, {
        TokenType::PLUS_EQUAL,
        TokenType::MINUS_EQUAL,
        TokenType::STAR_EQUAL,
        TokenType::SLASH_EQUAL,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "compound_assignment");
}

// ── Test 9: Delimiters ──
bool test_delimiters() {
    auto tokens = lex("( ) [ ] { } : , ;");
    return check_types(tokens, {
        TokenType::LPAREN,
        TokenType::RPAREN,
        TokenType::LBRACKET,
        TokenType::RBRACKET,
        TokenType::LBRACE,
        TokenType::RBRACE,
        TokenType::COLON,
        TokenType::COMMA,
        TokenType::SEMICOLON,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "delimiters");
}

// ── Test 10: Indentation (INDENT/DEDENT) ──
bool test_indentation() {
    auto tokens = lex(
        "fn foo():\n"
        "    let x = 1\n"
        "    let y = 2\n"
    );
    return check_types(tokens, {
        // Line 1: fn foo():
        TokenType::KW_FN,
        TokenType::IDENTIFIER,      // foo
        TokenType::LPAREN,
        TokenType::RPAREN,
        TokenType::COLON,
        TokenType::NEWLINE,

        // Line 2: (indented) let x = 1
        TokenType::INDENT,
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // x
        TokenType::EQUAL,
        TokenType::INTEGER_LITERAL, // 1
        TokenType::NEWLINE,

        // Line 3: (same level) let y = 2
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // y
        TokenType::EQUAL,
        TokenType::INTEGER_LITERAL, // 2
        TokenType::NEWLINE,

        // EOF: emit DEDENT for the open block
        TokenType::DEDENT,
        TokenType::END_OF_FILE,
    }, "indentation");
}

// ── Test 11: Nested indentation ──
bool test_nested_indentation() {
    auto tokens = lex(
        "if true:\n"
        "    if false:\n"
        "        x\n"
        "    y\n"
        "z\n"
    );
    return check_types(tokens, {
        // Line 1: if true:
        TokenType::KW_IF,
        TokenType::KW_TRUE,
        TokenType::COLON,
        TokenType::NEWLINE,

        // Line 2: (indent to 4) if false:
        TokenType::INDENT,
        TokenType::KW_IF,
        TokenType::KW_FALSE,
        TokenType::COLON,
        TokenType::NEWLINE,

        // Line 3: (indent to 8) x
        TokenType::INDENT,
        TokenType::IDENTIFIER,      // x
        TokenType::NEWLINE,

        // Line 4: (dedent to 4) y
        TokenType::DEDENT,
        TokenType::IDENTIFIER,      // y
        TokenType::NEWLINE,

        // Line 5: (dedent to 0) z
        TokenType::DEDENT,
        TokenType::IDENTIFIER,      // z
        TokenType::NEWLINE,

        TokenType::END_OF_FILE,
    }, "nested_indentation");
}

// ── Test 12: Bracket depth suppresses newlines ──
bool test_bracket_newline_suppression() {
    auto tokens = lex(
        "let x = (\n"
        "    1 + 2\n"
        ")"
    );
    return check_types(tokens, {
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // x
        TokenType::EQUAL,
        TokenType::LPAREN,
        // No NEWLINE inside parens
        TokenType::INTEGER_LITERAL, // 1
        TokenType::PLUS,
        TokenType::INTEGER_LITERAL, // 2
        // No NEWLINE inside parens
        TokenType::RPAREN,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "bracket_newline_suppression");
}

// ── Test 13: Line comments ──
bool test_line_comments() {
    auto tokens = lex(
        "let x = 5 // this is a comment\n"
        "let y = 10"
    );
    return check_types(tokens, {
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // x
        TokenType::EQUAL,
        TokenType::INTEGER_LITERAL, // 5
        // comment is skipped
        TokenType::NEWLINE,
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // y
        TokenType::EQUAL,
        TokenType::INTEGER_LITERAL, // 10
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "line_comments");
}

// ── Test 14: Hash-style line comments ──
bool test_hash_line_comments() {
    auto tokens = lex(
        "# a comment\n"
        "let x = 5 # trailing comment\n"
    );
    return check_types(tokens, {
        TokenType::KW_LET,
        TokenType::IDENTIFIER,
        TokenType::EQUAL,
        TokenType::INTEGER_LITERAL,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "hash_line_comments");
}

// ── Test 14: Block comments ──
bool test_block_comments() {
    auto tokens = lex("let /* comment */ x = 5");
    return check_types(tokens, {
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // x
        TokenType::EQUAL,
        TokenType::INTEGER_LITERAL, // 5
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "block_comments");
}

// ── Test 15: All keywords ──
bool test_all_keywords() {
    auto tokens = lex("fn class interface let var if else elif match "
                      "while for in return break continue try catch "
                      "finally throw go async await import from as "
                      "extends implements self own mut true false null "
                      "macro component state int float string bool void");
    // 41 keywords + NEWLINE + EOF = 43 tokens
    bool pass = true;
    if (tokens.size() != 43) {
        std::cerr << "  FAIL [all_keywords]: Expected 43 tokens, got "
                  << tokens.size() << "\n";
        pass = false;
    }
    // Spot-check a few
    if (tokens[0].type != TokenType::KW_FN) {
        std::cerr << "  FAIL [all_keywords]: First token should be KW_FN\n";
        pass = false;
    }
    if (tokens[40].type != TokenType::KW_VOID) {
        std::cerr << "  FAIL [all_keywords]: Token 40 should be KW_VOID, got "
                  << token_type_to_string(tokens[40].type) << "\n";
        pass = false;
    }
    return pass;
}

// ── Test 16: Function declaration (full syntax) ──
bool test_function_declaration() {
    auto tokens = lex(
        "fn add(a int, b int) int:\n"
        "    return a + b\n"
    );
    return check_types(tokens, {
        TokenType::KW_FN,
        TokenType::IDENTIFIER,      // add
        TokenType::LPAREN,
        TokenType::IDENTIFIER,      // a
        TokenType::KW_INT,
        TokenType::COMMA,
        TokenType::IDENTIFIER,      // b
        TokenType::KW_INT,
        TokenType::RPAREN,
        TokenType::KW_INT,          // return type
        TokenType::COLON,
        TokenType::NEWLINE,

        TokenType::INDENT,
        TokenType::KW_RETURN,
        TokenType::IDENTIFIER,      // a
        TokenType::PLUS,
        TokenType::IDENTIFIER,      // b
        TokenType::NEWLINE,

        TokenType::DEDENT,
        TokenType::END_OF_FILE,
    }, "function_declaration");
}

// ── Test 17: Inline function (curly brace variant) ──
bool test_inline_function() {
    auto tokens = lex("fn double(x int) int { return x * 2 }");
    return check_types(tokens, {
        TokenType::KW_FN,
        TokenType::IDENTIFIER,      // double
        TokenType::LPAREN,
        TokenType::IDENTIFIER,      // x
        TokenType::KW_INT,
        TokenType::RPAREN,
        TokenType::KW_INT,          // return type
        TokenType::LBRACE,
        TokenType::KW_RETURN,
        TokenType::IDENTIFIER,      // x
        TokenType::STAR,
        TokenType::INTEGER_LITERAL, // 2
        TokenType::RBRACE,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "inline_function");
}

// ── Test 18: Nullable type and safe unwrap ──
bool test_nullable() {
    auto tokens = lex(
        "let name string? = null\n"
        "if let actual = name:\n"
        "    print(actual)\n"
    );
    return check_types(tokens, {
        // let name string? = null
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // name
        TokenType::KW_STRING_TYPE,
        TokenType::QUESTION,
        TokenType::EQUAL,
        TokenType::KW_NULL,
        TokenType::NEWLINE,

        // if let actual = name:
        TokenType::KW_IF,
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // actual
        TokenType::EQUAL,
        TokenType::IDENTIFIER,      // name
        TokenType::COLON,
        TokenType::NEWLINE,

        // print(actual)
        TokenType::INDENT,
        TokenType::IDENTIFIER,      // print
        TokenType::LPAREN,
        TokenType::IDENTIFIER,      // actual
        TokenType::RPAREN,
        TokenType::NEWLINE,

        TokenType::DEDENT,
        TokenType::END_OF_FILE,
    }, "nullable");
}

// ── Test 19: Escape sequences in strings ──
bool test_escape_sequences() {
    auto tokens = lex("\"hello\\nworld\\t!\"");
    if (tokens.size() < 1) return false;
    if (tokens[0].type != TokenType::STRING_LITERAL) {
        std::cerr << "  FAIL [escape_sequences]: Expected STRING_LITERAL\n";
        return false;
    }
    if (tokens[0].value != "hello\nworld\t!") {
        std::cerr << "  FAIL [escape_sequences]: Escape not processed correctly\n";
        std::cerr << "  Got: \"" << tokens[0].value << "\"\n";
        return false;
    }
    return true;
}

// ── Test 20: Empty source ──
bool test_empty_source() {
    auto tokens = lex("");
    return check_types(tokens, {
        TokenType::END_OF_FILE,
    }, "empty_source");
}

// ── Test 21: Source location tracking ──
bool test_source_locations() {
    auto tokens = lex(
        "fn main() void:\n"
        "    return\n"
    );
    bool pass = true;

    // "fn" should be at line 1, column 1
    if (tokens[0].location.line != 1 || tokens[0].location.column != 1) {
        std::cerr << "  FAIL [source_locations]: 'fn' at wrong position: "
                  << tokens[0].location.line << ":" << tokens[0].location.column
                  << " (expected 1:1)\n";
        pass = false;
    }

    // "main" should be at line 1, column 4
    if (tokens[1].location.line != 1 || tokens[1].location.column != 4) {
        std::cerr << "  FAIL [source_locations]: 'main' at wrong position: "
                  << tokens[1].location.line << ":" << tokens[1].location.column
                  << " (expected 1:4)\n";
        pass = false;
    }

    return pass;
}

// ── Test 22: Hex and binary number values ──
bool test_number_values() {
    auto tokens = lex("0xFF 0b1010 1_000_000");
    bool pass = true;

    if (tokens[0].value != "0xFF") {
        std::cerr << "  FAIL [number_values]: Hex value: " << tokens[0].value << "\n";
        pass = false;
    }
    if (tokens[1].value != "0b1010") {
        std::cerr << "  FAIL [number_values]: Binary value: " << tokens[1].value << "\n";
        pass = false;
    }
    if (tokens[2].value != "1000000") {
        std::cerr << "  FAIL [number_values]: Underscore value: " << tokens[2].value << "\n";
        pass = false;
    }

    return pass;
}

// ── Test 23: Pattern matching syntax ──
bool test_pattern_matching() {
    auto tokens = lex(
        "match x:\n"
        "    0 => \"zero\"\n"
        "    _ => \"other\"\n"
    );
    return check_types(tokens, {
        TokenType::KW_MATCH,
        TokenType::IDENTIFIER,       // x
        TokenType::COLON,
        TokenType::NEWLINE,

        TokenType::INDENT,
        TokenType::INTEGER_LITERAL,  // 0
        TokenType::FAT_ARROW,
        TokenType::STRING_LITERAL,   // "zero"
        TokenType::NEWLINE,

        TokenType::IDENTIFIER,       // _
        TokenType::FAT_ARROW,
        TokenType::STRING_LITERAL,   // "other"
        TokenType::NEWLINE,

        TokenType::DEDENT,
        TokenType::END_OF_FILE,
    }, "pattern_matching");
}

// ── Test 24: Ownership annotations ──
bool test_ownership() {
    auto tokens = lex("fn process(own data string, view &string, editor &mut string) void:");
    return check_types(tokens, {
        TokenType::KW_FN,
        TokenType::IDENTIFIER,      // process
        TokenType::LPAREN,
        TokenType::KW_OWN,
        TokenType::IDENTIFIER,      // data
        TokenType::KW_STRING_TYPE,
        TokenType::COMMA,
        TokenType::IDENTIFIER,      // view
        TokenType::AMPERSAND,
        TokenType::KW_STRING_TYPE,
        TokenType::COMMA,
        TokenType::IDENTIFIER,      // editor
        TokenType::AMPERSAND,
        TokenType::KW_MUT,
        TokenType::KW_STRING_TYPE,
        TokenType::RPAREN,
        TokenType::KW_VOID,
        TokenType::COLON,
        TokenType::NEWLINE,
        TokenType::END_OF_FILE,
    }, "ownership");
}

// ── Test 25: Async/await and goroutines ──
bool test_concurrency_keywords() {
    auto tokens = lex(
        "async fn fetch() int:\n"
        "    let x = await get()\n"
        "    go fn():\n"
        "        print(x)\n"
    );
    return check_types(tokens, {
        // async fn fetch() int:
        TokenType::KW_ASYNC,
        TokenType::KW_FN,
        TokenType::IDENTIFIER,      // fetch
        TokenType::LPAREN,
        TokenType::RPAREN,
        TokenType::KW_INT,
        TokenType::COLON,
        TokenType::NEWLINE,

        // let x = await get()
        TokenType::INDENT,
        TokenType::KW_LET,
        TokenType::IDENTIFIER,      // x
        TokenType::EQUAL,
        TokenType::KW_AWAIT,
        TokenType::IDENTIFIER,      // get
        TokenType::LPAREN,
        TokenType::RPAREN,
        TokenType::NEWLINE,

        // go fn():
        TokenType::KW_GO,
        TokenType::KW_FN,
        TokenType::LPAREN,
        TokenType::RPAREN,
        TokenType::COLON,
        TokenType::NEWLINE,

        // print(x)
        TokenType::INDENT,
        TokenType::IDENTIFIER,      // print
        TokenType::LPAREN,
        TokenType::IDENTIFIER,      // x
        TokenType::RPAREN,
        TokenType::NEWLINE,

        TokenType::DEDENT,
        TokenType::DEDENT,
        TokenType::END_OF_FILE,
    }, "concurrency_keywords");
}


// ═══════════════════════════════════════════════════════════════════════════
// MAIN — Run All Tests
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n═══ Novium Lexer Test Suite ═══\n\n";

    RUN_TEST(test_keyword_vs_identifier);
    RUN_TEST(test_integer_literals);
    RUN_TEST(test_float_literals);
    RUN_TEST(test_simple_string);
    RUN_TEST(test_string_interpolation);
    RUN_TEST(test_multiple_interpolations);
    RUN_TEST(test_operators);
    RUN_TEST(test_compound_assignment);
    RUN_TEST(test_delimiters);
    RUN_TEST(test_indentation);
    RUN_TEST(test_nested_indentation);
    RUN_TEST(test_bracket_newline_suppression);
    RUN_TEST(test_line_comments);
    RUN_TEST(test_hash_line_comments);
    RUN_TEST(test_block_comments);
    RUN_TEST(test_all_keywords);
    RUN_TEST(test_function_declaration);
    RUN_TEST(test_inline_function);
    RUN_TEST(test_nullable);
    RUN_TEST(test_escape_sequences);
    RUN_TEST(test_empty_source);
    RUN_TEST(test_source_locations);
    RUN_TEST(test_number_values);
    RUN_TEST(test_pattern_matching);
    RUN_TEST(test_ownership);
    RUN_TEST(test_concurrency_keywords);

    std::cout << "\n═══ Results: " << tests_passed << " passed, "
              << tests_failed << " failed ═══\n\n";

    return tests_failed > 0 ? 1 : 0;
}

// ============================================================================
// lexer.h — Novium Lexer Interface
// ============================================================================
//
// WHAT THE LEXER DOES:
// The lexer (also called a "tokenizer" or "scanner") is the first stage of
// the compiler pipeline. It reads raw source code as a string of characters
// and breaks it into a sequence of tokens — meaningful units like keywords,
// numbers, operators, and identifiers.
//
// ANALOGY:
// If source code is a sentence, the lexer splits it into words and
// punctuation. The parser (next stage) figures out the grammar.
//
// HOW INDENTATION WORKS:
// Novium uses Python-Go hybrid syntax where blocks are started with ":"
// followed by indented lines. The lexer tracks indentation levels using a
// stack. When indentation increases, it emits an INDENT token. When it
// decreases, it emits one or more DEDENT tokens.
//
//   fn add(a int, b int) int:    ← colon starts block
//       return a + b             ← indented = inside block (INDENT emitted)
//   let x = add(1, 2)           ← back to base level (DEDENT emitted)
//
// HOW STRING INTERPOLATION WORKS:
// Strings like "Hello ${name}!" are split into multiple tokens:
//   STRING_START  "Hello "
//   IDENTIFIER    name
//   STRING_END    "!"
// This lets the parser treat the interpolated expression as real code.
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include "lexer/token.h"

namespace novium {

class Lexer {
public:
    // ── Constructor ──────────────────────────────────────────────────────
    // source:   the full source code text to tokenize
    // filename: name of the source file (used in error messages)
    Lexer(const std::string& source, const std::string& filename);

    // ── Main Entry Point ─────────────────────────────────────────────────
    // Tokenizes the entire source and returns a vector of all tokens.
    // The last token is always END_OF_FILE.
    // If errors are encountered, ERROR tokens are inserted in the stream.
    std::vector<Token> tokenize();

    // ── Error Reporting ──────────────────────────────────────────────────
    bool has_errors() const { return has_errors_; }

private:
    // ── Source State ─────────────────────────────────────────────────────
    std::string source_;        // The full source text
    std::string filename_;      // Source file name
    size_t pos_;                // Current position in source (0-based)
    int line_;                  // Current line number (1-based)
    int column_;                // Current column number (1-based)

    // ── Indentation Tracking ─────────────────────────────────────────────
    // The indent_stack_ holds the column numbers where each block started.
    // It always starts with [0] (the base indentation level).
    //
    // Example progression:
    //   "fn foo():"     → stack: [0]
    //   "    body"      → stack: [0, 4]    (INDENT emitted)
    //   "        deep"  → stack: [0, 4, 8] (INDENT emitted)
    //   "    back"      → stack: [0, 4]    (DEDENT emitted)
    //   "done"          → stack: [0]       (DEDENT emitted)
    std::stack<int> indent_stack_;
    bool at_line_start_;        // Are we at the beginning of a new line?

    // ── Bracket Depth ────────────────────────────────────────────────────
    // When inside parentheses (), brackets [], or braces {},
    // newlines and indentation are IGNORED. This allows multi-line
    // expressions without special escaping:
    //
    //   let result = (
    //       a + b +
    //       c + d
    //   )  ← no NEWLINE tokens emitted between a+b and c+d
    int bracket_depth_;

    // ── Error State ──────────────────────────────────────────────────────
    bool has_errors_;

    // ── Keyword Lookup Table ─────────────────────────────────────────────
    static const std::unordered_map<std::string, TokenType> keywords_;

    // ── Character Operations ─────────────────────────────────────────────
    // These are the low-level building blocks. Every scanning function
    // is built on top of these four primitives.

    char peek() const;          // Look at current char WITHOUT advancing
    char peek_next() const;     // Look at next char WITHOUT advancing
    char advance();             // Return current char AND move forward
    bool is_at_end() const;     // Are we past the end of source?
    bool match(char expected);  // If current char matches, advance and return true

    // ── Token Factory ────────────────────────────────────────────────────
    Token make_token(TokenType type, const std::string& value) const;
    Token make_token_at(TokenType type, const std::string& value,
                        int line, int column) const;
    Token error_token(const std::string& message);

    // ── Scanning Functions ───────────────────────────────────────────────
    // Each function handles one category of token.

    Token scan_identifier();                    // Keywords and identifiers
    Token scan_number();                        // Integer and float literals
    void  scan_string(std::vector<Token>& out); // String with interpolation
    Token scan_operator();                      // Operators and delimiters

    // ── Expression Scanning (for string interpolation) ───────────────────
    // Scans a single token inside a ${...} interpolation expression.
    // Uses the same logic as the main scanner but without indentation.
    Token scan_expression_token();

    // ── Whitespace & Comments ────────────────────────────────────────────
    void skip_line_comment();   // Skip from // to end of line
    void skip_block_comment();  // Skip from /* to */

    // ── Indentation Processing ───────────────────────────────────────────
    // Called at the start of each line. Counts leading spaces and emits
    // INDENT/DEDENT tokens as needed.
    void handle_line_start(std::vector<Token>& tokens);
};

} // namespace novium

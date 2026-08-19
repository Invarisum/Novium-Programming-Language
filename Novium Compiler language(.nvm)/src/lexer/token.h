// ============================================================================
// token.h — Token Types and Data Structures for the Novium Lexer
// ============================================================================
//
// WHAT THIS FILE DOES:
// Defines every kind of "word" (token) that the Novium language recognizes.
// Think of tokens like parts of speech in English:
//   - Keywords are like grammar words (fn, if, class)
//   - Identifiers are like nouns (variable names, function names)
//   - Operators are like verbs (+, -, *, /)
//   - Literals are like specific values (42, "hello", true)
//
// The lexer reads raw text and produces a stream of these tokens.
// The parser then reads the token stream to build the AST.
// ============================================================================

#pragma once

#include <string>
#include <ostream>

namespace novium {

// ── Token Type Enum ──────────────────────────────────────────────────────────
// Every distinct kind of token the lexer can produce.
// Organized by category for readability.

enum class TokenType {
    // ── Literals (concrete values in source code) ──
    INTEGER_LITERAL,    // 42, 0xFF, 0b1010
    FLOAT_LITERAL,      // 3.14, 1.0e-5
    STRING_LITERAL,     // "hello world" (no interpolation)
    STRING_START,       // "Hello ${       (before first interpolation)
    STRING_MIDDLE,      // } and ${        (between interpolations)
    STRING_END,         // } world"        (after last interpolation)

    // ── Identifiers (user-defined names) ──
    IDENTIFIER,         // foo, myVariable, Vec2

    // ── Keywords (reserved words with special meaning) ──
    KW_FN,              // fn         — function declaration
      KW_EXTERN,        // extern     // extern "C" function declaration
    KW_CLASS,           // class      — class declaration
    KW_INTERFACE,       // interface  — interface declaration
    KW_LET,             // let        — immutable variable binding
    KW_VAR,             // var        — mutable variable binding
    KW_IF,              // if         — conditional
    KW_ELSE,            // else       — else branch
    KW_ELIF,            // elif       — else-if branch
    KW_MATCH,           // match      — pattern matching
    KW_WHILE,           // while      — while loop
    KW_FOR,             // for        — for loop
    KW_IN,              // in         — used in for-in loops
    KW_RETURN,          // return     — return from function
    KW_BREAK,           // break      — exit loop
    KW_CONTINUE,        // continue   — skip to next loop iteration
    KW_TRY,             // try        — exception handling
    KW_CATCH,           // catch      — catch exception
    KW_FINALLY,         // finally    — always-run block
    KW_THROW,           // throw      — raise exception
    KW_GO,              // go         — spawn goroutine
    KW_ASYNC,           // async      — async function marker
    KW_AWAIT,           // await      — suspend until ready
    KW_IMPORT,          // import     — import module
    KW_FROM,            // from       — import source
    KW_AS,              // as         — alias or type cast
    KW_EXTENDS,         // extends    — class inheritance
    KW_IMPLEMENTS,      // implements — interface implementation
    KW_SELF,            // self       — current instance reference
    KW_OWN,             // own        — ownership transfer marker
    KW_MUT,             // mut        — mutable borrow marker
    KW_TRUE,            // true       — boolean literal
    KW_FALSE,           // false      — boolean literal
    KW_NULL,            // null       — null literal (nullable types only)
    KW_MACRO,           // macro      — macro definition
    KW_COMPONENT,       // component  — web component (for .nvw)
    KW_STATE,           // state      — component state (for .nvw)

    // ── Type Keywords ──
    KW_INT,             // int        — 64-bit signed integer
    KW_FLOAT,           // float      — 64-bit IEEE float
    KW_STRING_TYPE,     // string     — UTF-8 string type
    KW_BOOL,            // bool       — boolean type
    KW_VOID,            // void       — no return value

    // ── Operators ──
    PLUS,               // +
    MINUS,              // -
    STAR,               // *
    SLASH,              // /
    PERCENT,            // %
    EQUAL,              // =
    EQUAL_EQUAL,        // ==
    BANG,               // !
    BANG_EQUAL,         // !=
    LESS,               // <
    LESS_EQUAL,         // <=
    GREATER,            // >
    GREATER_EQUAL,      // >=
    AND_AND,            // &&
    OR_OR,              // ||
    AMPERSAND,          // &   (also used for immutable borrow)
    PLUS_EQUAL,         // +=
    MINUS_EQUAL,        // -=
    STAR_EQUAL,         // *=
    SLASH_EQUAL,        // /=
    ARROW,              // ->  (return type annotation)
    FAT_ARROW,          // =>  (match arm, lambda)
    DOT,                // .   (member access)
    QUESTION,           // ?   (nullable type marker)

    // ── Delimiters (grouping and separation) ──
    LPAREN,             // (
    RPAREN,             // )
    LBRACKET,           // [
    RBRACKET,           // ]
    LBRACE,             // {
    RBRACE,             // }
    COLON,              // :   (block start, type annotation)
    COMMA,              // ,   (parameter separator)
    SEMICOLON,          // ;   (optional statement terminator)

    // ── Whitespace-Significant Tokens ──
    // These are "synthetic" tokens — they don't correspond to a visible
    // character. The lexer generates them based on indentation changes.
    NEWLINE,            // End of a logical line (statement terminator)
    INDENT,             // Indentation level increased (block start)
    DEDENT,             // Indentation level decreased (block end)

    // ── Special ──
    END_OF_FILE,        // No more input
    ERROR,              // Lexer error (with message in value field)
};

// ── Source Location ──────────────────────────────────────────────────────────
// Tracks where a token came from in the source file.
// Essential for error messages: "error at line 42, column 10 in main.nvm"

struct SourceLocation {
    std::string filename;   // Which file this token is from
    int line;               // Line number (1-based)
    int column;             // Column number (1-based)
};

// ── Token ────────────────────────────────────────────────────────────────────
// A single token produced by the lexer.
//
// Example: the source text "fn" becomes:
//   Token { type: KW_FN, value: "fn", location: {file, 1, 1} }

struct Token {
    TokenType type;         // What kind of token this is
    std::string value;      // The raw text that was matched
    SourceLocation location;// Where it appeared in the source
};

// ── Utility Functions ────────────────────────────────────────────────────────

// Convert a TokenType enum value to a human-readable string.
// Used for debugging and error messages.
const char* token_type_to_string(TokenType type);

// Print a token in a formatted way: TYPE "value" @ line:column
std::ostream& operator<<(std::ostream& os, const Token& token);

} // namespace novium

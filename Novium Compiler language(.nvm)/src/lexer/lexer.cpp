// ============================================================================
// lexer.cpp — Novium Lexer Implementation
// ============================================================================
//
// This is the heart of Sprint 1. Read through this file top-to-bottom —
// it's organized from low-level primitives (peek, advance) up to the
// high-level tokenize() function.
//
// ARCHITECTURE:
//   1. Character primitives: peek(), advance(), is_at_end()
//   2. Token factory: make_token(), error_token()
//   3. Scanners: scan_identifier(), scan_number(), scan_string(), scan_operator()
//   4. Indentation: handle_line_start()
//   5. Main loop: tokenize()
//
// ============================================================================

#include "lexer/lexer.h"
#include <iostream>
#include <cassert>

namespace novium {

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 1: Character Classification Helpers
// ═══════════════════════════════════════════════════════════════════════════
// These are pure functions — they just classify characters.
// We use these instead of <cctype> to avoid locale-dependent behavior.

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 2: Keyword Lookup Table
// ═══════════════════════════════════════════════════════════════════════════
// When the lexer scans an identifier like "fn", it checks this table to
// see if it's actually a keyword. If it's not in the table, it's a
// user-defined identifier.

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    // Core language keywords
    {"fn",         TokenType::KW_FN},
    {"extern",     TokenType::KW_EXTERN},
    {"moji",       TokenType::KW_MOJI},
    {"class",      TokenType::KW_CLASS},
    {"interface",  TokenType::KW_INTERFACE},
    {"let",        TokenType::KW_LET},
    {"var",        TokenType::KW_VAR},
    {"if",         TokenType::KW_IF},
    {"else",       TokenType::KW_ELSE},
    {"elif",       TokenType::KW_ELIF},
    {"match",      TokenType::KW_MATCH},
    {"while",      TokenType::KW_WHILE},
    {"for",        TokenType::KW_FOR},
    {"in",         TokenType::KW_IN},
    {"return",     TokenType::KW_RETURN},
    {"break",      TokenType::KW_BREAK},
    {"continue",   TokenType::KW_CONTINUE},
    {"try",        TokenType::KW_TRY},
    {"catch",      TokenType::KW_CATCH},
    {"finally",    TokenType::KW_FINALLY},
    {"throw",      TokenType::KW_THROW},
    {"panic",      TokenType::KW_PANIC},
    {"go",         TokenType::KW_GO},
    {"async",      TokenType::KW_ASYNC},
    {"await",      TokenType::KW_AWAIT},
    {"defer",      TokenType::KW_DEFER},
    {"import",     TokenType::KW_IMPORT},
    {"from",       TokenType::KW_FROM},
    {"as",         TokenType::KW_AS},
    {"extends",    TokenType::KW_EXTENDS},
    {"implements", TokenType::KW_IMPLEMENTS},
    {"self",       TokenType::KW_SELF},
    {"own",        TokenType::KW_OWN},
    {"mut",        TokenType::KW_MUT},
    {"unsafe",     TokenType::KW_UNSAFE},
    {"true",       TokenType::KW_TRUE},
    {"false",      TokenType::KW_FALSE},
    {"null",       TokenType::KW_NULL},
    {"macro",      TokenType::KW_MACRO},
    {"component",  TokenType::KW_COMPONENT},
    {"state",      TokenType::KW_STATE},

    // Type keywords
    {"int",        TokenType::KW_INT},
    {"float",      TokenType::KW_FLOAT},
    {"string",     TokenType::KW_STRING_TYPE},
    {"bool",       TokenType::KW_BOOL},
    {"void",       TokenType::KW_VOID},

    // Mojo compatibility keywords
    {"pub",        TokenType::KW_PUB},      // public visibility
    {"struct",     TokenType::KW_STRUCT},   // struct type
    {"enum",       TokenType::KW_ENUM},     // enum type
    {"borrow",     TokenType::KW_BORROW},   // borrow reference
    {"using",      TokenType::KW_USING},    // using declaration
    {"cast",       TokenType::KW_CAST},     // type cast
    {"sizeof",     TokenType::KW_SIZEOF},   // size of type
    {"alignof",    TokenType::KW_ALIGNOF},  // alignment of type
    {"tensor",     TokenType::KW_TENSOR},   // tensor type
    {"matrix",     TokenType::KW_MATRIX},   // matrix type
    {"core",       TokenType::KW_CORE},     // core module
    {"math",       TokenType::KW_MATH},     // math module
    {"array",      TokenType::KW_ARRAY},    // array type
    {"slice",      TokenType::KW_SLICE},    // slice type

    // Python compatibility keywords
    {"pass",       TokenType::KW_PASS},     // pass statement (Python)
    {"raise",      TokenType::KW_RAISE},    // raise exception (Python)
    {"with",       TokenType::KW_WITH},     // with statement (Python)
    {"python",     TokenType::KW_PYTHON},   // python compatibility block

    // .nvw (web) keywords
    {"jsx",            TokenType::KW_JSX},
    {"css",            TokenType::KW_CSS},
    {"html",           TokenType::KW_HTML},
    {"import_python",  TokenType::KW_IMPORT_PYTHON},
    {"export_js",      TokenType::KW_EXPORT_JS},
};

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 3: Constructor
// ═══════════════════════════════════════════════════════════════════════════

Lexer::Lexer(const std::string& source, const std::string& filename)
    : source_(source)
    , filename_(filename)
    , pos_(0)
    , line_(1)
    , column_(1)
    , at_line_start_(true)
    , bracket_depth_(0)
    , has_errors_(false)
{
    // The indent stack always starts with 0 — the base indentation level.
    // Every source file begins at column 0, and any function body will be
    // indented relative to this baseline.
    indent_stack_.push(0);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 4: Character Primitives
// ═══════════════════════════════════════════════════════════════════════════
// These are the four atomic operations the lexer uses to read source text.
// Every scanning function is built on top of these.
//
// MENTAL MODEL: Think of `pos_` as a cursor pointing at one character in
// the source string. peek() reads the character under the cursor without
// moving it. advance() reads the character AND moves the cursor forward.

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return source_[pos_];
}

char Lexer::peek_next() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}

char Lexer::advance() {
    // Read the character at the current position
    char c = source_[pos_];
    pos_++;

    // Update line/column tracking.
    // We track the position AFTER advancing, so column_ will point to
    // the position of the NEXT character to be read.
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }

    return c;
}

bool Lexer::is_at_end() const {
    return pos_ >= source_.size();
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (source_[pos_] != expected) return false;
    advance();
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 5: Token Factory
// ═══════════════════════════════════════════════════════════════════════════

Token Lexer::make_token(TokenType type, const std::string& value) const {
    return Token{type, value, {filename_, line_, column_ - static_cast<int>(value.size())}};
}

Token Lexer::make_token_at(TokenType type, const std::string& value,
                           int line, int column) const {
    return Token{type, value, {filename_, line, column}};
}

Token Lexer::error_token(const std::string& message) {
    has_errors_ = true;
    return Token{TokenType::ERROR, message, {filename_, line_, column_}};
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 6: Comment Handling
// ═══════════════════════════════════════════════════════════════════════════

void Lexer::skip_line_comment() {
    // We're sitting on the first '/'. Advance past '//'
    advance(); // /
    advance(); // /

    // Consume everything until end of line (but don't consume the newline
    // itself — the main loop needs to see it for NEWLINE token emission)
    while (!is_at_end() && peek() != '\n') {
        advance();
    }
}

void Lexer::skip_block_comment() {
    // We're sitting on the first '/'. Advance past '/*'
    advance(); // /
    advance(); // *

    // Track nesting depth so /* /* */ */ works correctly
    int depth = 1;

    while (!is_at_end() && depth > 0) {
        if (peek() == '/' && peek_next() == '*') {
            advance(); advance();
            depth++;
        } else if (peek() == '*' && peek_next() == '/') {
            advance(); advance();
            depth--;
        } else {
            advance();
        }
    }

    if (depth > 0) {
        // We hit EOF without closing the comment
        has_errors_ = true;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 7: Identifier & Keyword Scanner
// ═══════════════════════════════════════════════════════════════════════════
// Identifiers start with a letter or underscore, followed by letters,
// digits, or underscores. After scanning the full identifier, we check
// if it's a keyword.
//
// Examples:
//   "fn"       → KW_FN (keyword)
//   "myVar"    → IDENTIFIER
//   "_private" → IDENTIFIER
//   "x2"       → IDENTIFIER

Token Lexer::scan_identifier() {
    int start_col = column_;
    int start_line = line_;
    std::string word;

    while (!is_at_end() && is_alnum(peek())) {
        word += advance();
    }

    // Check if the word is a keyword
    auto it = keywords_.find(word);
    if (it != keywords_.end()) {
        return make_token_at(it->second, word, start_line, start_col);
    }

    return make_token_at(TokenType::IDENTIFIER, word, start_line, start_col);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 8: Number Scanner
// ═══════════════════════════════════════════════════════════════════════════
// Handles:
//   - Integers: 42, 0, 1000000
//   - Floats:   3.14, 0.5, 1.0e10, 2.5e-3
//   - Hex:      0xFF, 0xDEAD
//   - Binary:   0b1010, 0b11110000
//
// IMPORTANT: "1.method()" should NOT parse "1." as a float.
// We only treat the dot as decimal if it's followed by a digit.

Token Lexer::scan_number(std::vector<Token>& out) {
    int start_col = column_;
    int start_line = line_;
    std::string num;
    bool is_float = false;

    // Check for hex (0x) or binary (0b) prefix
    if (peek() == '0' && !is_at_end()) {
        char next = peek_next();
        if (next == 'x' || next == 'X') {
            num += advance(); // '0'
            num += advance(); // 'x'
            bool has_digits = false;
            while (!is_at_end() && (is_digit(peek()) ||
                   (peek() >= 'a' && peek() <= 'f') ||
                   (peek() >= 'A' && peek() <= 'F') ||
                    peek() == '_')) {
                if (peek() != '_') {
                    num += advance();
                    has_digits = true;
                } else {
                    advance();
                    if (is_at_end() || (!is_digit(peek()) && 
                        !(peek() >= 'a' && peek() <= 'f') && 
                        !(peek() >= 'A' && peek() <= 'F'))) {
                        out.push_back(error_token("Trailing underscore in hex literal"));
                        break;
                    }
                }
            }
            if (!has_digits) {
                out.push_back(error_token("Hex literal requires at least one digit"));
            }
            return make_token_at(TokenType::INTEGER_LITERAL, num, start_line, start_col);
        }
        if (next == 'b' || next == 'B') {
            num += advance(); // '0'
            num += advance(); // 'b'
            bool has_digits = false;
            while (!is_at_end() && (peek() == '0' || peek() == '1' || peek() == '_')) {
                if (peek() != '_') {
                    num += advance();
                    has_digits = true;
                } else {
                    advance();
                    if (is_at_end() || (peek() != '0' && peek() != '1')) {
                        out.push_back(error_token("Trailing underscore in binary literal"));
                        break;
                    }
                }
            }
            if (!has_digits) {
                out.push_back(error_token("Binary literal requires at least one digit"));
            }
            return make_token_at(TokenType::INTEGER_LITERAL, num, start_line, start_col);
        }
    }

    // Decimal digits
    bool has_digits = false;
    while (!is_at_end() && (is_digit(peek()) || peek() == '_')) {
        if (peek() != '_') {
            num += advance();
            has_digits = true;
        } else {
            advance();
            if (is_at_end() || !is_digit(peek())) {
                out.push_back(error_token("Trailing underscore in integer literal"));
                break;
            }
        }
    }
    if (!has_digits) {
        out.push_back(error_token("Integer literal requires at least one digit"));
    }

    // Decimal point — only if followed by a digit (to avoid "1.method()")
    if (!is_at_end() && peek() == '.' && is_digit(peek_next())) {
        is_float = true;
        num += advance(); // '.'
        bool has_digits_after_dot = false;
        while (!is_at_end() && (is_digit(peek()) || peek() == '_')) {
            if (peek() != '_') {
                num += advance();
                has_digits_after_dot = true;
            } else {
                // Underscore allowed only between digits
                advance();
                // Check if next is a digit or we're at end (would be trailing)
                if (is_at_end() || !is_digit(peek())) {
                    // Trailing underscore - emit error but continue
                    out.push_back(error_token("Trailing underscore in float literal"));
                    break;
                }
            }
        }
        // Ensure at least one digit after decimal point
        if (!has_digits_after_dot) {
            out.push_back(error_token("Float literal requires digits after decimal point"));
        }
    }

    // Exponent: e10, E-5, e+3
    if (!is_at_end() && (peek() == 'e' || peek() == 'E')) {
        is_float = true;
        num += advance(); // 'e'
        if (!is_at_end() && (peek() == '+' || peek() == '-')) {
            num += advance(); // sign
        }
        // Require at least one digit after the exponent sign — reject "1e", "1e+"
        bool has_exponent_digits = false;
        while (!is_at_end() && is_digit(peek())) {
            num += advance();
            has_exponent_digits = true;
        }
        if (!has_exponent_digits) {
            out.push_back(error_token("Float literal requires digits in exponent"));
        }
    }

    TokenType type = is_float ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL;
    return make_token_at(type, num, start_line, start_col);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 9: String Scanner (with Interpolation)
// ═══════════════════════════════════════════════════════════════════════════
// This is the most complex scanning function because strings can contain
// interpolated expressions: "Hello ${name}!"
//
// For a plain string like "hello", we produce:
//   STRING_LITERAL "hello"
//
// For an interpolated string like "Hello ${name}!", we produce:
//   STRING_START  "Hello "
//   IDENTIFIER    name
//   STRING_END    "!"
//
// For multiple interpolations like "${a} and ${b}":
//   STRING_START  ""
//   IDENTIFIER    a
//   STRING_MIDDLE " and "
//   IDENTIFIER    b
//   STRING_END    ""
//
// The tokens between STRING_START/MIDDLE/END are normal expression tokens
// that the parser will evaluate.

void Lexer::scan_string(std::vector<Token>& out) {
    int start_line = line_;
    int start_col = column_;

    advance(); // consume opening "

    bool has_interpolation = false;
    std::string buffer;

    while (!is_at_end() && peek() != '"') {
        // ── Escape Sequences ──
        if (peek() == '\\') {
            advance(); // consume backslash
            if (is_at_end()) {
                out.push_back(error_token("Unterminated string: backslash at end of file"));
                return;
            }
            switch (peek()) {
                case 'n':  buffer += '\n'; advance(); break;
                case 't':  buffer += '\t'; advance(); break;
                case 'r':  buffer += '\r'; advance(); break;
                case '\\': buffer += '\\'; advance(); break;
                case '"':  buffer += '"';  advance(); break;
                case '$':  buffer += '$';  advance(); break;
                case '0':  buffer += '\0'; advance(); break;
                default:
                    out.push_back(error_token(
                        std::string("Unknown escape sequence: \\") + peek()));
                    advance();
                    return;
            }
            continue;
        }

// ── String Interpolation: ${...} ──
            if (peek() == '$' && peek_next() == '{') {
                // Emit the string content accumulated so far
                TokenType type = has_interpolation ? TokenType::STRING_MIDDLE
                                                  : TokenType::STRING_START;
                out.push_back(make_token_at(type, buffer, start_line, start_col));
                has_interpolation = true;
                buffer.clear();

                advance(); // consume '$'
                advance(); // consume '{'

                // Track all bracket types for proper nesting
                // We need to handle: ${...}, ${(...)}, ${[...]}, and nested strings
                int brace_depth = 1;
                int paren_depth = 0;
                int bracket_depth = 0;
                bool in_string = false;
                int string_nesting = 0; // Track nested string interpolations

                while (!is_at_end() && (brace_depth > 0 || paren_depth > 0 || bracket_depth > 0)) {
                    // Skip whitespace inside interpolation
                    while (!is_at_end() && (peek() == ' ' || peek() == '\t')) {
                        advance();
                    }
                    if (is_at_end()) break;

                    // Handle nested string interpolation: ${"text ${var}"}
                    if (peek() == '"' && !in_string) {
                        in_string = true;
                        string_nesting++;
                        // Scan the nested string using the same logic (recursive-ish)
                        // For simplicity, we'll just consume until the closing quote
                        // but properly handle escapes
                        advance(); // consume opening "
                        while (!is_at_end() && peek() != '"') {
                            if (peek() == '\\') {
                                advance(); advance(); // skip escape sequence
                            } else if (peek() == '$' && peek_next() == '{') {
                                // Nested interpolation - track it
                                advance(); advance();
                                brace_depth++;
                            } else {
                                advance();
                            }
                        }
                        if (!is_at_end()) advance(); // consume closing "
                        in_string = false;
                        string_nesting--;
                        continue;
                    }

                    // Handle brackets/braces/parens for proper nesting
                    char c = peek();
                    if (c == '{') {
                        brace_depth++;
                    } else if (c == '}') {
                        brace_depth--;
                        if (brace_depth == 0 && paren_depth == 0 && bracket_depth == 0) {
                            advance(); // consume the closing '}'
                            break;
                        }
                    } else if (c == '(') {
                        paren_depth++;
                    } else if (c == ')') {
                        if (paren_depth > 0) paren_depth--;
                    } else if (c == '[') {
                        bracket_depth++;
                    } else if (c == ']') {
                        if (bracket_depth > 0) bracket_depth--;
                    }

                    // Scan one expression token
                    Token t = scan_expression_token(out);
                    out.push_back(t);
                }

                if (brace_depth > 0 || paren_depth > 0 || bracket_depth > 0) {
                    out.push_back(error_token("Unterminated string interpolation"));
                    // Don't return - continue tokenizing to find more errors
                }

                // Update the start position for the next string segment
                start_line = line_;
                start_col = column_;
                continue;
            }

        // ── Newline inside string ──
        // We allow multi-line strings (the newline is included literally)
        if (peek() == '\n') {
            buffer += '\n';
            advance();
            continue;
        }

        // ── Regular character ──
        buffer += advance();
    }

    // Check for unterminated string
    if (is_at_end()) {
        out.push_back(error_token("Unterminated string literal"));
        return;
    }

    advance(); // consume closing "

    // Emit the final token
    if (has_interpolation) {
        out.push_back(make_token_at(TokenType::STRING_END, buffer, start_line, start_col));
    } else {
        out.push_back(make_token_at(TokenType::STRING_LITERAL, buffer, start_line, start_col));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 10: Expression Token Scanner (for string interpolation)
// ═══════════════════════════════════════════════════════════════════════════
// Scans a single token inside a ${...} interpolation. Uses the same logic
// as the main scanner but doesn't handle indentation or newlines.

Token Lexer::scan_expression_token(std::vector<Token>& out) {
    // Skip spaces
    while (!is_at_end() && (peek() == ' ' || peek() == '\t')) {
        advance();
    }

    if (is_at_end()) {
        return error_token("Unexpected end of file in string interpolation");
    }

    char c = peek();

    if (is_alpha(c) || c == '_') return scan_identifier();
    if (is_digit(c)) return scan_number(out);

    // Operators and delimiters (subset relevant in expressions)
    return scan_operator();
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 11: Operator & Delimiter Scanner
// ═══════════════════════════════════════════════════════════════════════════
// Handles all single-character and multi-character operators and delimiters.
// Multi-character operators are handled by "lookahead": after seeing '+',
// check if the next character is '=' to form '+='.

Token Lexer::scan_operator() {
    int start_col = column_;
    int start_line = line_;
    char c = advance();

    switch (c) {
        // ── Single-character tokens (no lookahead needed) ──
        case '(': return make_token_at(TokenType::LPAREN,   "(", start_line, start_col);
        case ')': return make_token_at(TokenType::RPAREN,   ")", start_line, start_col);
        case '[': return make_token_at(TokenType::LBRACKET, "[", start_line, start_col);
        case ']': return make_token_at(TokenType::RBRACKET, "]", start_line, start_col);
        case '{': return make_token_at(TokenType::LBRACE,   "{", start_line, start_col);
        case '}': return make_token_at(TokenType::RBRACE,   "}", start_line, start_col);
        case ':': return make_token_at(TokenType::COLON,    ":", start_line, start_col);
        case ',': return make_token_at(TokenType::COMMA,    ",", start_line, start_col);
        case ';': return make_token_at(TokenType::SEMICOLON,";", start_line, start_col);
        case '.': return make_token_at(TokenType::DOT,      ".", start_line, start_col);
        case '?': return make_token_at(TokenType::QUESTION, "?", start_line, start_col);
        case '%': return make_token_at(TokenType::PERCENT,  "%", start_line, start_col);

        // ── Two-character operators (require one character of lookahead) ──
        case '+':
            if (match('=')) return make_token_at(TokenType::PLUS_EQUAL,  "+=", start_line, start_col);
            return make_token_at(TokenType::PLUS, "+", start_line, start_col);

        case '-':
            if (match('>')) return make_token_at(TokenType::ARROW,       "->", start_line, start_col);
            if (match('=')) return make_token_at(TokenType::MINUS_EQUAL, "-=", start_line, start_col);
            return make_token_at(TokenType::MINUS, "-", start_line, start_col);

        case '*':
            if (match('=')) return make_token_at(TokenType::STAR_EQUAL,  "*=", start_line, start_col);
            return make_token_at(TokenType::STAR, "*", start_line, start_col);

        case '/':
            if (match('=')) return make_token_at(TokenType::SLASH_EQUAL, "/=", start_line, start_col);
            if (match('>')) return make_token_at(TokenType::SLASH_GREATER, "/>", start_line, start_col);
            return make_token_at(TokenType::SLASH, "/", start_line, start_col);

        case '=':
            if (match('=')) return make_token_at(TokenType::EQUAL_EQUAL, "==", start_line, start_col);
            if (match('>')) return make_token_at(TokenType::FAT_ARROW,   "=>", start_line, start_col);
            return make_token_at(TokenType::EQUAL, "=", start_line, start_col);

        case '!':
            if (match('=')) return make_token_at(TokenType::BANG_EQUAL,  "!=", start_line, start_col);
            return make_token_at(TokenType::BANG, "!", start_line, start_col);

        case '<':
            if (match('=')) return make_token_at(TokenType::LESS_EQUAL,  "<=", start_line, start_col);
            return make_token_at(TokenType::LESS, "<", start_line, start_col);

        case '>':
            if (match('=')) return make_token_at(TokenType::GREATER_EQUAL, ">=", start_line, start_col);
            return make_token_at(TokenType::GREATER, ">", start_line, start_col);

        case '&':
            if (match('&')) return make_token_at(TokenType::AND_AND, "&&", start_line, start_col);
            return make_token_at(TokenType::AMPERSAND, "&", start_line, start_col);

        case '|':
            if (match('|')) return make_token_at(TokenType::OR_OR, "||", start_line, start_col);
            return error_token(std::string("Unexpected character: |  (did you mean '||'?)"));

        default:
            return error_token(std::string("Unexpected character: ") + c);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 12: Indentation Handler
// ═══════════════════════════════════════════════════════════════════════════
// Called at the start of every new line. Counts the leading spaces and
// compares them to the current indentation level on the stack.
//
// THREE CASES:
//   1. indent > stack.top()  → Block is opening → emit INDENT
//   2. indent < stack.top()  → Block(s) closing → emit DEDENT(s)
//   3. indent == stack.top() → Same level → no token
//
// EXAMPLE:
//   fn foo():         ← stack: [0], no indent change
//       let x = 1     ← stack: [0, 4], INDENT emitted
//       if x > 0:     ← stack: [0, 4], same level
//           print(x)  ← stack: [0, 4, 8], INDENT emitted
//       let y = 2     ← stack: [0, 4], DEDENT emitted (back to 4)
//   fn bar():         ← stack: [0], DEDENT emitted (back to 0)

void Lexer::handle_line_start(std::vector<Token>& tokens) {
    at_line_start_ = false;

    // Count leading spaces
    int indent = 0;
    while (!is_at_end() && peek() == ' ') {
        advance();
        indent++;
    }

    // Handle tabs: we reject them in indentation to avoid ambiguity
    if (!is_at_end() && peek() == '\t') {
        tokens.push_back(error_token(
            "Tab character in indentation. Novium requires spaces for indentation."));
        // Skip the tab and continue
        advance();
        return;
    }

    // Skip blank lines entirely — they don't affect indentation
    // A blank line is one that's empty or contains only whitespace
    if (is_at_end() || peek() == '\n' || peek() == '\r') {
        return; // Don't process indentation for blank lines
    }

    // Skip comment-only lines — they don't affect indentation either
    if ((peek() == '/' && peek_next() == '/') || peek() == '#') {
        return; // The comment will be handled in the main loop
    }

    // Only process indentation when we're NOT inside brackets
    // Inside brackets, indentation is meaningless (just formatting)
    if (bracket_depth_ > 0) {
        return;
    }

    int current_indent = indent_stack_.top();

    if (indent > current_indent) {
        // ── INDENT: entering a deeper block ──
        indent_stack_.push(indent);
        tokens.push_back(make_token_at(TokenType::INDENT, "<indent>", line_, 1));
    } else if (indent < current_indent) {
        // ── DEDENT: leaving one or more blocks ──
        // We might need to emit multiple DEDENTs if we're jumping back
        // several levels at once:
        //
        //   fn foo():
        //       if true:
        //           print("deep")
        //   fn bar():          ← goes from indent 8 to 0, emits TWO dedents
        //
        while (indent_stack_.top() > indent) {
            indent_stack_.pop();
            tokens.push_back(make_token_at(TokenType::DEDENT, "<dedent>", line_, 1));
        }

        // After popping, the top of the stack should exactly match the
        // current line's indentation. If it doesn't, the indentation is
        // inconsistent (e.g., 3 spaces where we expect 0 or 4).
        if (indent_stack_.top() != indent) {
            tokens.push_back(error_token(
                "Inconsistent indentation: expected " +
                std::to_string(indent_stack_.top()) +
                " spaces, got " + std::to_string(indent)));
        }
    }
    // If indent == current_indent, we're at the same level — no token needed
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 13: Main Tokenization Loop
// ═══════════════════════════════════════════════════════════════════════════
// This is the top-level function. It loops through the source, calling
// the appropriate scanner for each character, and manages the overall
// state (line starts, newlines, brackets).
//
// FLOW:
//   1. Check if we're at a line start → handle indentation
//   2. Skip whitespace (spaces, not newlines)
//   3. Identify what the next character starts:
//      - Newline → emit NEWLINE token
//      - '//' or '/*' → skip comment
//      - Letter/_ → scan identifier or keyword
//      - Digit → scan number
//      - '"' → scan string
//      - Anything else → scan operator or delimiter
//   4. Repeat until EOF
//   5. Emit remaining DEDENT tokens and EOF

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!is_at_end()) {
        // ── Step 1: Handle line starts ──
        if (at_line_start_) {
            handle_line_start(tokens);
            // After handle_line_start, we're positioned at the first
            // non-space character on the line. Continue to scan it.
            if (is_at_end()) break;
        }

        char c = peek();

        // ── Step 2: Skip inline whitespace ──
        if (c == ' ' || c == '\t') {
            // Reject tabs in indentation and inline - Novium requires spaces
            if (c == '\t') {
                tokens.push_back(error_token(
                    "Tab character found. Novium requires spaces for indentation and alignment."));
            }
            advance();
            continue;
        }

        // ── Carriage return (Windows line endings \r\n) ──
        if (c == '\r') {
            advance();
            continue;
        }

        // ── Step 3: Newlines ──
        if (c == '\n') {
            advance();

            // Only emit NEWLINE when we're NOT inside brackets.
            // Inside brackets like ( ), [ ], or { }, newlines are ignored
            // to allow multi-line expressions:
            //
            //   let x = (
            //       1 + 2 +    ← no NEWLINE emitted here
            //       3 + 4      ← or here
            //   )
            if (bracket_depth_ == 0) {
                // Avoid duplicate NEWLINEs — they'd confuse the parser
                if (!tokens.empty() &&
                    tokens.back().type != TokenType::NEWLINE &&
                    tokens.back().type != TokenType::INDENT &&
                    tokens.back().type != TokenType::DEDENT) {
                    tokens.push_back(make_token_at(TokenType::NEWLINE, "\\n", line_ - 1, column_));
                }
            }

            at_line_start_ = true;
            continue;
        }

        // ── Step 4: Comments ──
        // `#` is accepted as a line-comment marker as well as `//`.
        // It keeps indentation-style Novium source convenient to annotate.
        if (c == '#') {
            while (!is_at_end() && peek() != '\n') {
                advance();
            }
            continue;
        }
        if (c == '/' && peek_next() == '/') {
            skip_line_comment();
            continue;
        }
        if (c == '/' && peek_next() == '*') {
            skip_block_comment();
            continue;
        }

        // ── Step 5: Identifiers and keywords ──
        if (is_alpha(c) || c == '_') {
            tokens.push_back(scan_identifier());
            continue;
        }

        // ── Step 6: Numbers ──
        if (is_digit(c)) {
            tokens.push_back(scan_number(tokens));
            continue;
        }

        // ── Step 7: Strings ──
        if (c == '"') {
            scan_string(tokens);
            continue;
        }

        // ── Step 8: Operators and delimiters ──
        Token t = scan_operator();

        // Track bracket depth for newline suppression
        if (t.type == TokenType::LPAREN ||
            t.type == TokenType::LBRACKET ||
            t.type == TokenType::LBRACE) {
            bracket_depth_++;
        } else if (t.type == TokenType::RPAREN ||
                   t.type == TokenType::RBRACKET ||
                   t.type == TokenType::RBRACE) {
            if (bracket_depth_ > 0) {
                bracket_depth_--;
            } else {
                tokens.push_back(error_token("Unmatched closing bracket: " + t.value));
            }
        }

        tokens.push_back(t);
    }

    // ── Unbalanced opening brackets ──
    // If the file ends with unclosed brackets, every NEWLINE after them was
    // suppressed — report it so no source is silently swallowed.
    if (bracket_depth_ > 0) {
        tokens.push_back(error_token(
            "Unclosed bracket: " + std::to_string(bracket_depth_) +
            " bracket(s) left open at end of file"));
        bracket_depth_ = 0;
    }

    // ── Emit trailing NEWLINE ──
    // If the file doesn't end with a newline, we still need one to
    // terminate the last statement.
    if (!tokens.empty() &&
        tokens.back().type != TokenType::NEWLINE &&
        tokens.back().type != TokenType::INDENT &&
        tokens.back().type != TokenType::DEDENT) {
        tokens.push_back(make_token_at(TokenType::NEWLINE, "\\n", line_, column_));
    }

    // ── Emit remaining DEDENTs ──
    // At EOF, we need to close all open blocks. If the indent stack is
    // [0, 4, 8], we emit two DEDENTs to bring it back to [0].
    while (indent_stack_.size() > 1) {
        indent_stack_.pop();
        tokens.push_back(make_token_at(TokenType::DEDENT, "<dedent>", line_, column_));
    }

    // ── Emit EOF ──
    tokens.push_back(make_token_at(TokenType::END_OF_FILE, "", line_, column_));

    return tokens;
}

} // namespace novium

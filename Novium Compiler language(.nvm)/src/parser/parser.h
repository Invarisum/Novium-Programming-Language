// ============================================================================
// parser.h — Pratt / Recursive Descent Parser Interface
// ============================================================================

#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "lexer/token.h"
#include "parser/ast.h"

namespace novium {

// ── Operator Precedences ─────────────────────────────────────────────────────
// Controls the binding power of infix operators in the Pratt parser.
enum class Precedence {
    LOWEST = 0,
    ASSIGN,       // =  +=  -=  *=  /=
    LOGICAL_OR,   // ||
    LOGICAL_AND,  // &&
    COMPARISON,   // ==  !=  <  <=  >  >=
    SUM,          // +  -
    PRODUCT,      // *  /  %
    PREFIX,       // -  !  &  &mut  await
    CALL,         // function()
    MEMBER,       // object.member
    INDEX         // array[index]
};

class Parser {
public:
    // Constructor takes the token stream from the Lexer
    explicit Parser(std::vector<Token> tokens);

    // Main entry point: parses the entire file into a top-level block/list of statements
    std::vector<std::unique_ptr<Stmt>> parse_program();

    // Check if errors occurred during parsing
    bool has_errors() const { return !errors_.empty(); }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    std::vector<Token> tokens_;
    size_t current_;
    std::vector<std::string> errors_;

    // ── Function Pointer Typedefs for Pratt Parser ────────────────────────────
    // A prefix function parses prefix expressions (e.g. literals, variables, negative numbers).
    // An infix function parses infix operations (e.g. addition, function calls, field accesses).
    using PrefixParseFn = std::unique_ptr<Expr> (Parser::*)();
    using InfixParseFn  = std::unique_ptr<Expr> (Parser::*)(std::unique_ptr<Expr>);

    // Maps to store our Pratt parser routines
    std::unordered_map<TokenType, PrefixParseFn> prefix_fns_;
    std::unordered_map<TokenType, InfixParseFn> infix_fns_;

    void register_prefix(TokenType type, PrefixParseFn fn);
    void register_infix(TokenType type, InfixParseFn fn);
    void init_pratt_rules();

    // ── Navigation Primitives ────────────────────────────────────────────────
    Token peek() const;
    Token peek_next() const;
    Token previous() const;
    bool is_at_end() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);
    void error(Token token, const std::string& message);

    // ── Statement Parsing (Recursive Descent) ────────────────────────────────
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<Stmt> parse_var_decl();
    std::unique_ptr<Stmt> parse_function_decl(bool is_async);
    std::unique_ptr<Stmt> parse_class_decl();
    std::unique_ptr<Stmt> parse_interface_decl();
    std::unique_ptr<Stmt> parse_if_stmt();
    std::unique_ptr<Stmt> parse_while_stmt();
    std::unique_ptr<Stmt> parse_match_stmt();
    std::unique_ptr<Stmt> parse_return_stmt();
    std::unique_ptr<Stmt> parse_try_catch_stmt();
    std::unique_ptr<Stmt> parse_go_stmt();
    std::unique_ptr<Stmt> parse_defer_stmt();
    std::unique_ptr<Stmt> parse_unsafe_block();
    std::unique_ptr<Stmt> parse_panic_stmt();
    std::unique_ptr<Stmt> parse_pass_stmt();
    std::unique_ptr<Stmt> parse_raise_stmt();
    std::unique_ptr<Stmt> parse_python_ffi_block();
    std::unique_ptr<Stmt> parse_pub_decl();
    std::unique_ptr<Stmt> parse_struct_decl();
    std::unique_ptr<Stmt> parse_enum_decl();
    std::unique_ptr<Stmt> parse_using_decl();
    std::unique_ptr<Stmt> parse_with_stmt();
    std::unique_ptr<Expr> parse_cast_expr();
    std::unique_ptr<Expr> parse_sizeof_expr();
    std::unique_ptr<Expr> parse_alignof_expr();
    std::unique_ptr<Stmt> parse_tensor_type();
    std::unique_ptr<Stmt> parse_matrix_type();
    std::unique_ptr<Expr> parse_jsx_expr();
    std::unique_ptr<Stmt> parse_css_styles();
    std::unique_ptr<Stmt> parse_html_template();
    std::unique_ptr<Stmt> parse_python_import();
    std::unique_ptr<Stmt> parse_js_export();
    std::unique_ptr<BlockStmt> parse_block_stmt(const std::string& context_name);
    std::unique_ptr<Stmt> parse_expression_stmt();

    // Helper parsing methods
    TypeAnnotation parse_type_annotation();
    FunctionParam parse_function_param();
    ClassField parse_class_field();
    std::unique_ptr<FunctionDeclStmt> parse_method_decl();

    // Error recovery: synchronizes the parser to a safe statement boundary
    void synchronize();

    // ── Expression Parsing (Pratt Parsing) ───────────────────────────────────
    std::unique_ptr<Expr> parse_expression(Precedence precedence);

    // Prefix parse functions
    std::unique_ptr<Expr> parse_identifier();
    std::unique_ptr<Expr> parse_literal();
    std::unique_ptr<Expr> parse_unary();
    std::unique_ptr<Expr> parse_grouped_expr();

    // Infix parse functions
    std::unique_ptr<Expr> parse_binary(std::unique_ptr<Expr> left);
    std::unique_ptr<Expr> parse_call(std::unique_ptr<Expr> left);
    std::unique_ptr<Expr> parse_member_access(std::unique_ptr<Expr> left);
    std::unique_ptr<Expr> parse_index_expr(std::unique_ptr<Expr> left);

    // Precedence helpers
    Precedence get_precedence(TokenType type) const;
};

} // namespace novium

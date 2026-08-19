// ============================================================================
// parser.cpp — Pratt / Recursive Descent Parser Implementation
// ============================================================================

#include "parser/parser.h"
#include <iostream>

namespace novium {

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 1: Constructor & Pratt Map Initializer
// ═══════════════════════════════════════════════════════════════════════════

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), current_(0)
{
    init_pratt_rules();
}

void Parser::register_prefix(TokenType type, PrefixParseFn fn) {
    prefix_fns_[type] = fn;
}

void Parser::register_infix(TokenType type, InfixParseFn fn) {
    infix_fns_[type] = fn;
}

void Parser::init_pratt_rules() {
    // ── Prefix Rules ──
    register_prefix(TokenType::IDENTIFIER,      &Parser::parse_identifier);
    register_prefix(TokenType::INTEGER_LITERAL, &Parser::parse_literal);
    register_prefix(TokenType::FLOAT_LITERAL,   &Parser::parse_literal);
    register_prefix(TokenType::STRING_LITERAL,  &Parser::parse_literal);
    register_prefix(TokenType::STRING_START,    &Parser::parse_literal); // Starts interpolation
    register_prefix(TokenType::KW_TRUE,         &Parser::parse_literal);
    register_prefix(TokenType::KW_FALSE,        &Parser::parse_literal);
    register_prefix(TokenType::KW_NULL,         &Parser::parse_literal);
    register_prefix(TokenType::KW_SELF,         &Parser::parse_literal);
    
    register_prefix(TokenType::MINUS,           &Parser::parse_unary);
    register_prefix(TokenType::BANG,            &Parser::parse_unary);
    register_prefix(TokenType::AMPERSAND,       &Parser::parse_unary); // borrow
    register_prefix(TokenType::KW_AWAIT,        &Parser::parse_unary); // async await
    register_prefix(TokenType::LPAREN,          &Parser::parse_grouped_expr);

    // ── Infix Rules ──
    register_infix(TokenType::PLUS,             &Parser::parse_binary);
    register_infix(TokenType::MINUS,            &Parser::parse_binary);
    register_infix(TokenType::STAR,             &Parser::parse_binary);
    register_infix(TokenType::SLASH,            &Parser::parse_binary);
    register_infix(TokenType::PERCENT,          &Parser::parse_binary);
    
    register_infix(TokenType::EQUAL,            &Parser::parse_binary);
    register_infix(TokenType::PLUS_EQUAL,       &Parser::parse_binary);
    register_infix(TokenType::MINUS_EQUAL,      &Parser::parse_binary);
    register_infix(TokenType::STAR_EQUAL,       &Parser::parse_binary);
    register_infix(TokenType::SLASH_EQUAL,      &Parser::parse_binary);

    register_infix(TokenType::EQUAL_EQUAL,      &Parser::parse_binary);
    register_infix(TokenType::BANG_EQUAL,       &Parser::parse_binary);
    register_infix(TokenType::LESS,             &Parser::parse_binary);
    register_infix(TokenType::LESS_EQUAL,       &Parser::parse_binary);
    register_infix(TokenType::GREATER,          &Parser::parse_binary);
    register_infix(TokenType::GREATER_EQUAL,    &Parser::parse_binary);
    
    register_infix(TokenType::AND_AND,          &Parser::parse_binary);
    register_infix(TokenType::OR_OR,            &Parser::parse_binary);

    register_infix(TokenType::LPAREN,           &Parser::parse_call);
    register_infix(TokenType::DOT,              &Parser::parse_member_access);
    register_infix(TokenType::LBRACKET,         &Parser::parse_index_expr);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 2: Navigation Helpers
// ═══════════════════════════════════════════════════════════════════════════

Token Parser::peek() const {
    if (current_ >= tokens_.size()) return tokens_.back();
    return tokens_[current_];
}

Token Parser::peek_next() const {
    if (current_ + 1 >= tokens_.size()) return tokens_.back();
    return tokens_[current_ + 1];
}

Token Parser::previous() const {
    if (current_ == 0) return tokens_[0];
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::END_OF_FILE;
}

Token Parser::advance() {
    if (!is_at_end()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(peek(), message);
    return Token{TokenType::ERROR, "", peek().location};
}

void Parser::error(Token token, const std::string& message) {
    std::string err = "[" + token.location.filename + ":" +
                      std::to_string(token.location.line) + ":" +
                      std::to_string(token.location.column) + "] Error: " + message;
    errors_.push_back(err);
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 3: Main Program Loop & Statement Synchronization
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::unique_ptr<Stmt>> Parser::parse_program() {
    std::vector<std::unique_ptr<Stmt>> program;
    
    // Read tokens until EOF
    while (!is_at_end()) {
        // Skip stray newlines at package/file level
        if (match(TokenType::NEWLINE)) continue;
        
        auto stmt = parse_statement();
        if (stmt) {
            program.push_back(std::move(stmt));
        } else {
            synchronize();
        }
    }
    return program;
}

void Parser::synchronize() {
    advance(); // consume the failing token

    while (!is_at_end()) {
        // Synchronize on statement terminators
        if (previous().type == TokenType::NEWLINE || previous().type == TokenType::SEMICOLON) {
            return;
        }

        // Synchronize on major statement start keywords
        switch (peek().type) {
            case TokenType::KW_FN:
            case TokenType::KW_CLASS:
            case TokenType::KW_INTERFACE:
            case TokenType::KW_LET:
            case TokenType::KW_VAR:
            case TokenType::KW_IF:
            case TokenType::KW_MATCH:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::KW_TRY:
            case TokenType::KW_THROW:
            case TokenType::KW_GO:
            case TokenType::DEDENT:
                return;
            default:
                break;
        }
        advance();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 4: Recursive Descent Statements
// ═══════════════════════════════════════════════════════════════════════════

std::unique_ptr<Stmt> Parser::parse_statement() {
    if (match(TokenType::KW_LET) || match(TokenType::KW_VAR)) {
        return parse_var_decl();
    }
    if (match(TokenType::KW_FN)) {
        return parse_function_decl(false);
    }
    if (match(TokenType::KW_ASYNC)) {
        consume(TokenType::KW_FN, "Expected 'fn' after 'async'.");
        return parse_function_decl(true);
    }
    if (match(TokenType::KW_CLASS)) {
        return parse_class_decl();
    }
    if (match(TokenType::KW_INTERFACE)) {
        return parse_interface_decl();
    }
    if (match(TokenType::KW_IF)) {
        return parse_if_stmt();
    }
    if (match(TokenType::KW_WHILE)) {
        return parse_while_stmt();
    }
    if (match(TokenType::KW_MATCH)) {
        return parse_match_stmt();
    }
    if (match(TokenType::KW_RETURN)) {
        return parse_return_stmt();
    }
    if (match(TokenType::KW_TRY)) {
        return parse_try_catch_stmt();
    }
    if (match(TokenType::KW_GO)) {
        return parse_go_stmt();
    }
    if (match(TokenType::KW_DEFER)) {
        return parse_defer_stmt();
    }
    if (match(TokenType::KW_PANIC)) {
        return parse_panic_stmt();
    }
    if (match(TokenType::KW_PYTHON)) {
        // Python/Mojo compatibility block
        return parse_python_ffi_block();
    }
    if (match(TokenType::KW_PUB)) {
        // Mojo-style public visibility
        return parse_pub_decl();
    }
    if (match(TokenType::KW_PASS)) {
        // Python-style pass statement - no-op
        return std::make_unique<EmptyStmt>(previous().location);
    }
    if (match(TokenType::KW_STRUCT)) {
        return parse_struct_decl();
    }
    if (match(TokenType::KW_ENUM)) {
        return parse_enum_decl();
    }
    if (match(TokenType::KW_USING)) {
        // Mojo-style using declaration
        return parse_using_decl();
    }
    if (match(TokenType::KW_RAISE)) {
        // Python-style raise
        return parse_raise_stmt();
    }
    if (match(TokenType::KW_WITH)) {
        // Python-style with statement
        return parse_with_stmt();
    }
    if (match(TokenType::KW_CAST)) {
        // Mojo-style type cast
        return std::make_unique<ExpressionStmt>(parse_cast_expr());
    }
    if (match(TokenType::KW_SIZEOF)) {
        // Mojo-style sizeof
        return std::make_unique<ExpressionStmt>(parse_sizeof_expr());
    }
    if (match(TokenType::KW_ALIGNOF)) {
        // Mojo-style alignof
        return std::make_unique<ExpressionStmt>(parse_alignof_expr());
    }
    if (match(TokenType::KW_TENSOR)) {
        // Mojo tensor type
        return parse_tensor_type();
    }
    if (match(TokenType::KW_MATRIX)) {
        // Mojo matrix type
        return parse_matrix_type();
    }
    if (match(TokenType::KW_JSX)) {
        // JSX expression in .nvw
        // JSX is expression-based, returns a JSX AST node
        return std::make_unique<ExpressionStmt>(parse_jsx_expr());
    }
    if (match(TokenType::KW_CSS)) {
        // CSS-in-JS styling
        return parse_css_styles();
    }
    if (match(TokenType::KW_HTML)) {
        // HTML template
        return parse_html_template();
    }
    if (match(TokenType::KW_IMPORT_PYTHON)) {
        // Import Python module from .nvw
        return parse_python_import();
    }
    if (match(TokenType::KW_EXPORT_JS)) {
        // Export function to JavaScript
        return parse_js_export();
    }
    if (match(TokenType::KW_UNSAFE)) {
        return parse_unsafe_block();
    }
    return parse_expression_stmt();
}

// Variable Declaration: `let x = 5` or `var x: int = 5`
std::unique_ptr<Stmt> Parser::parse_var_decl() {
    SourceLocation loc = previous().location;
    bool is_mutable = (previous().type == TokenType::KW_VAR);
    // Rust-familiar spelling: `let mut count = 0`.
    if (!is_mutable && match(TokenType::KW_MUT)) {
        is_mutable = true;
    }

    Token name_token = consume(TokenType::IDENTIFIER, "Expected variable name.");
    std::string name = name_token.value;

    TypeAnnotation ty;
    bool has_ty = false;

    // Check optional type annotation
    // In Novium syntax: `let x int = 5` (clean Go-style type declaration) or `let x: int = 5`
    // We support optional colon: e.g. let x int = 5 OR let x: int = 5
    bool type_follows = check(TokenType::IDENTIFIER) ||
                        check(TokenType::KW_INT) ||
                        check(TokenType::KW_FLOAT) ||
                        check(TokenType::KW_STRING_TYPE) ||
                        check(TokenType::KW_BOOL) ||
                        check(TokenType::AMPERSAND) ||
                        check(TokenType::KW_OWN) ||
                        check(TokenType::COLON);

    if (type_follows) {
        match(TokenType::COLON); // skip colon if present
        ty = parse_type_annotation();
        has_ty = true;
    }

    std::unique_ptr<Expr> init = nullptr;
    if (match(TokenType::EQUAL)) {
        init = parse_expression(Precedence::LOWEST);
    }

    if (!match(TokenType::NEWLINE) && !match(TokenType::SEMICOLON) && !is_at_end() &&
        peek().type != TokenType::DEDENT && peek().type != TokenType::RBRACE) {
        error(peek(), "Expected newline or semicolon after variable declaration.");
    }

    return std::make_unique<VarDeclStmt>(name, is_mutable, ty, has_ty, std::move(init), loc);
}

// Function Declaration: `fn add(a int, b int) int:`
std::unique_ptr<Stmt> Parser::parse_function_decl(bool is_async) {
    SourceLocation loc = previous().location;
    Token name_token = consume(TokenType::IDENTIFIER, "Expected function name.");
    std::string name = name_token.value;

    bool is_extern = false;
    std::string extern_name;
    std::string extern_lang;
    if (match(TokenType::KW_EXTERN)) {
        is_extern = true;
        if (check(TokenType::STRING_LITERAL)) {
            extern_name = advance().value;
        }
    } else if (match(TokenType::KW_MOJI)) {
        is_extern = true;
        extern_lang = "moji";
        // After `moji`, expect optional string literal for function name
        // or proceed with function parameters
    }

    consume(TokenType::LPAREN, "Expected '(' before function parameters.");

    std::vector<FunctionParam> params;
    if (!check(TokenType::RPAREN)) {
        do {
            params.push_back(parse_function_param());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after function parameters.");

    TypeAnnotation ret_type;
    bool has_ret = false;

    // Both familiar forms are accepted:
    //   fn add(a int, b int) int:     // original Go-like Novium form
    //   fn add(a: int, b: int) -> int { // Rust/C++-familiar form
    if (match(TokenType::ARROW)) {
        ret_type = parse_type_annotation();
        has_ret = true;
    } else if (!check(TokenType::COLON) && !check(TokenType::LBRACE)) {
        ret_type = parse_type_annotation();
        has_ret = true;
    }

    // Parse block body
    std::unique_ptr<BlockStmt> body = parse_block_stmt("function '" + name + "'");

return std::make_unique<FunctionDeclStmt>(name, 
std::move(params), ret_type, has_ret, std::move(body), is_async, is_extern,
extern_name, extern_lang, loc);
}

// Class Declaration: `class Vector extends Base implements Drawable:`
std::unique_ptr<Stmt> Parser::parse_class_decl() {
    SourceLocation loc = previous().location;
    Token name_token = consume(TokenType::IDENTIFIER, "Expected class name.");
    std::string name = name_token.value;

    std::string base_class;
    bool has_base = false;
    if (match(TokenType::KW_AS) || match(TokenType::KW_EXTENDS)) { // extends
        Token base_token = consume(TokenType::IDENTIFIER, "Expected base class name.");
        base_class = base_token.value;
        has_base = true;
    }

    std::vector<std::string> interfaces;
    if (match(TokenType::KW_IMPLEMENTS)) {
        do {
            Token iface_token = consume(TokenType::IDENTIFIER, "Expected interface name.");
            interfaces.push_back(iface_token.value);
        } while (match(TokenType::COMMA));
    }

    // Now consume start of block
    consume(TokenType::COLON, "Expected ':' to start class body.");
    consume(TokenType::NEWLINE, "Expected newline after class declaration.");
    consume(TokenType::INDENT, "Expected indented block for class body.");

    std::vector<ClassField> fields;
    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;

    while (!check(TokenType::DEDENT) && !is_at_end()) {
        if (match(TokenType::NEWLINE)) continue;

        // Methods start with `fn` or `async fn`
        bool is_method = check(TokenType::KW_FN) ||
                          check(TokenType::KW_ASYNC) ||
                          (check(TokenType::IDENTIFIER) && peek_next().type == TokenType::IDENTIFIER);

        if (check(TokenType::KW_FN) || check(TokenType::KW_ASYNC)) {
            bool is_async_method = match(TokenType::KW_ASYNC);
            consume(TokenType::KW_FN, "Expected 'fn' for method.");
            auto method = parse_function_decl(is_async_method);
            // Safe downcast with dynamic_cast
            if (auto* fn_stmt = dynamic_cast<FunctionDeclStmt*>(method.get())) {
                methods.push_back(std::unique_ptr<FunctionDeclStmt>(static_cast<FunctionDeclStmt*>(method.release())));
            } else {
                error(peek(), "Expected function declaration for method.");
            }
        } else {
            // Fields are name + type annotation
            fields.push_back(parse_class_field());
        }
    }

    consume(TokenType::DEDENT, "Expected dedent at the end of class body.");

    return std::make_unique<ClassDeclStmt>(name, base_class, has_base, std::move(interfaces), std::move(fields), std::move(methods), loc);
}

// Interface Declaration: `interface Drawable:`
std::unique_ptr<Stmt> Parser::parse_interface_decl() {
    SourceLocation loc = previous().location;
    Token name_token = consume(TokenType::IDENTIFIER, "Expected interface name.");
    std::string name = name_token.value;

    consume(TokenType::COLON, "Expected ':' to start interface body.");
    consume(TokenType::NEWLINE, "Expected newline after interface declaration.");
    consume(TokenType::INDENT, "Expected indented block for interface body.");

    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
    while (!check(TokenType::DEDENT) && !is_at_end()) {
        if (match(TokenType::NEWLINE)) continue;

        bool is_async = match(TokenType::KW_ASYNC);
        consume(TokenType::KW_FN, "Expected method declaration starting with 'fn'.");
        
        Token method_name_tok = consume(TokenType::IDENTIFIER, "Expected method name.");
        std::string m_name = method_name_tok.value;

        consume(TokenType::LPAREN, "Expected '(' before method parameters.");
        std::vector<FunctionParam> params;
        if (!check(TokenType::RPAREN)) {
            do {
                params.push_back(parse_function_param());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expected ')' after method parameters.");

        TypeAnnotation ret_type;
        bool has_ret = false;
        if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON)) {
            ret_type = parse_type_annotation();
            has_ret = true;
        }

        if (!match(TokenType::NEWLINE) && !match(TokenType::SEMICOLON)) {
            error(peek(), "Expected newline or semicolon after interface method declaration.");
        }

        methods.push_back(std::make_unique<FunctionDeclStmt>(m_name, std::move(params), ret_type, has_ret, nullptr, is_async, false, "", "", method_name_tok.location));
    }

    consume(TokenType::DEDENT, "Expected dedent at the end of interface body.");

    return std::make_unique<InterfaceDeclStmt>(name, std::move(methods), loc);
}

// If Statement: `if cond: ... elif cond: ... else: ...`
std::unique_ptr<Stmt> Parser::parse_if_stmt() {
    SourceLocation loc = previous().location;

    std::unique_ptr<Expr> condition = parse_expression(Precedence::LOWEST);
    std::unique_ptr<BlockStmt> then_branch = parse_block_stmt("if condition");

    std::vector<ElifBranch> elif_branches;
    while (match(TokenType::KW_ELIF)) {
        std::unique_ptr<Expr> elif_cond = parse_expression(Precedence::LOWEST);
        std::unique_ptr<BlockStmt> elif_body = parse_block_stmt("elif condition");
        elif_branches.push_back(ElifBranch{std::move(elif_cond), std::move(elif_body)});
    }

    std::unique_ptr<BlockStmt> else_branch = nullptr;
    if (match(TokenType::KW_ELSE)) {
        else_branch = parse_block_stmt("else branch");
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(elif_branches), std::move(else_branch), loc);
}

// While statement: `while condition: ...` or `while condition { ... }`
std::unique_ptr<Stmt> Parser::parse_while_stmt() {
    SourceLocation loc = previous().location;
    std::unique_ptr<Expr> condition = parse_expression(Precedence::LOWEST);
    std::unique_ptr<BlockStmt> body = parse_block_stmt("while loop");
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), loc);
}

// Match statement: `match subject:`
std::unique_ptr<Stmt> Parser::parse_match_stmt() {
    SourceLocation loc = previous().location;
    
    std::unique_ptr<Expr> subject = parse_expression(Precedence::LOWEST);
    
    consume(TokenType::COLON, "Expected ':' after match subject.");
    consume(TokenType::NEWLINE, "Expected newline after match expression.");
    consume(TokenType::INDENT, "Expected indented block for match body.");

    std::vector<MatchArm> arms;
    while (!check(TokenType::DEDENT) && !is_at_end()) {
        if (match(TokenType::NEWLINE)) continue;

        // Match pattern is an expression (identifier or literal or wildcard)
        std::unique_ptr<Expr> pattern = parse_expression(Precedence::LOWEST);
        consume(TokenType::FAT_ARROW, "Expected '=>' after match pattern.");

        std::unique_ptr<Stmt> body = parse_statement();
        arms.push_back(MatchArm{std::move(pattern), std::move(body)});
    }

    consume(TokenType::DEDENT, "Expected dedent at the end of match body.");

    return std::make_unique<MatchStmt>(std::move(subject), std::move(arms), loc);
}

// Return Statement: `return x`
std::unique_ptr<Stmt> Parser::parse_return_stmt() {
    SourceLocation loc = previous().location;
    std::unique_ptr<Expr> value = nullptr;

    if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !check(TokenType::DEDENT) && !is_at_end()) {
        value = parse_expression(Precedence::LOWEST);
    }

    if (!match(TokenType::NEWLINE) && !match(TokenType::SEMICOLON) && !is_at_end() &&
        peek().type != TokenType::DEDENT && peek().type != TokenType::RBRACE) {
        error(peek(), "Expected newline or semicolon after return statement.");
    }

    return std::make_unique<ReturnStmt>(std::move(value), loc);
}

// Try-Catch-Finally: `try: ... catch e: ... finally: ...`
std::unique_ptr<Stmt> Parser::parse_try_catch_stmt() {
    SourceLocation loc = previous().location;

    std::unique_ptr<BlockStmt> try_block = parse_block_stmt("try block");
    std::vector<CatchBlock> catch_blocks;

    while (match(TokenType::KW_CATCH)) {
        SourceLocation catch_loc = previous().location;
        std::string ex_type;
        bool has_type = false;
        std::string ex_var;
        bool has_var = false;

        // catch variables: e.g. `catch IOError as e:` or `catch e:`
        if (check(TokenType::IDENTIFIER)) {
            Token identifier = advance();
            if (match(TokenType::KW_AS)) {
                // It was a type: `catch IOError as e`
                ex_type = identifier.value;
                has_type = true;
                Token var_tok = consume(TokenType::IDENTIFIER, "Expected variable name after 'as'.");
                ex_var = var_tok.value;
                has_var = true;
            } else {
                // Simple: `catch e:` (exception variable)
                ex_var = identifier.value;
                has_var = true;
            }
        }

        std::unique_ptr<BlockStmt> catch_body = parse_block_stmt("catch block");
        catch_blocks.push_back(CatchBlock{ex_type, has_type, ex_var, has_var, std::move(catch_body), catch_loc});
    }

    std::unique_ptr<BlockStmt> finally_block = nullptr;
    if (match(TokenType::KW_FINALLY)) {
        finally_block = parse_block_stmt("finally block");
    }

    return std::make_unique<TryCatchStmt>(std::move(try_block), std::move(catch_blocks), std::move(finally_block), loc);
}

// Mojo compatibility: Python-style `pass` statement
std::unique_ptr<Stmt> Parser::parse_pass_stmt() {
    SourceLocation loc = previous().location;
    return std::make_unique<EmptyStmt>(loc);
}

// Mojo compatibility: Python-style `raise` statement
std::unique_ptr<Stmt> Parser::parse_raise_stmt() {
    SourceLocation loc = previous().location;
    return std::make_unique<EmptyStmt>(loc);
}

// Panic statement: `panic("message")` or `panic`
std::unique_ptr<Stmt> Parser::parse_panic_stmt() {
    SourceLocation loc = previous().location;
    std::unique_ptr<Expr> message = nullptr;
    if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !check(TokenType::DEDENT) && !is_at_end()) {
        message = parse_expression(Precedence::LOWEST);
    }
    if (!match(TokenType::NEWLINE) && !match(TokenType::SEMICOLON) && !is_at_end() &&
        peek().type != TokenType::DEDENT && peek().type != TokenType::RBRACE) {
        error(peek(), "Expected newline or semicolon after panic statement.");
    }
    return std::make_unique<PanicStmt>(std::move(message));
}

// Go statement: `go calculate(x)`
std::unique_ptr<Stmt> Parser::parse_go_stmt() {
    SourceLocation loc = previous().location;
    
    // Pratt expression to parse the target function call
    std::unique_ptr<Expr> call = parse_expression(Precedence::LOWEST);

    if (!match(TokenType::NEWLINE) && !match(TokenType::SEMICOLON) && !is_at_end() &&
        peek().type != TokenType::DEDENT && peek().type != TokenType::RBRACE) {
        error(peek(), "Expected newline or semicolon after 'go' statement.");
    }

    return std::make_unique<GoStmt>(std::move(call), loc);
}

// Defer statement: `defer cleanup()`
std::unique_ptr<Stmt> Parser::parse_defer_stmt() {
    SourceLocation loc = previous().location;
    
    // Parse the block to defer
    std::unique_ptr<BlockStmt> body = parse_block_stmt("defer block");
    
    return std::make_unique<DeferStmt>(std::move(body), loc);
}

// Unsafe block: `unsafe { ... }`
std::unique_ptr<Stmt> Parser::parse_unsafe_block() {
    SourceLocation loc = previous().location;
    
    // Parse the block content
    std::unique_ptr<BlockStmt> body = parse_block_stmt("unsafe block");
    
    return std::make_unique<UnsafeBlockStmt>(std::move(body), loc);
}

// Expression Statement
std::unique_ptr<Stmt> Parser::parse_expression_stmt() {
    std::unique_ptr<Expr> expr = parse_expression(Precedence::LOWEST);
    
    // If parsing failed (e.g., unexpected token with no prefix rule),
    // consume tokens until we hit a statement boundary to allow error recovery.
    if (!expr && !is_at_end()) {
        // Consume tokens until we find a statement terminator or major keyword
        while (!is_at_end()) {
            TokenType t = peek().type;
            if (t == TokenType::NEWLINE || t == TokenType::SEMICOLON || t == TokenType::DEDENT ||
                t == TokenType::KW_FN || t == TokenType::KW_CLASS || t == TokenType::KW_INTERFACE ||
                t == TokenType::KW_LET || t == TokenType::KW_VAR || t == TokenType::KW_IF ||
                t == TokenType::KW_MATCH || t == TokenType::KW_WHILE || t == TokenType::KW_FOR ||
                t == TokenType::KW_RETURN || t == TokenType::KW_TRY || t == TokenType::KW_THROW ||
                t == TokenType::KW_GO || t == TokenType::KW_DEFER || t == TokenType::KW_UNSAFE) {
                break;
            }
            advance();
        }
    }
    
    if (!match(TokenType::NEWLINE) && !match(TokenType::SEMICOLON) && !is_at_end() && peek().type != TokenType::DEDENT) {
        error(peek(), "Expected newline or semicolon after expression statement.");
    }
    
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

// Block Statement parser: handles `colon + indent` or inline `{}`
std::unique_ptr<BlockStmt> Parser::parse_block_stmt(const std::string& context_name) {
    SourceLocation loc = peek().location;
    std::vector<std::unique_ptr<Stmt>> statements;

    if (match(TokenType::COLON)) {
        // Indentation block: e.g. `: \n INDENT statement... DEDENT`
        consume(TokenType::NEWLINE, "Expected newline after ':' in " + context_name + ".");
        consume(TokenType::INDENT, "Expected indentation block for " + context_name + " body.");

        while (!check(TokenType::DEDENT) && !is_at_end()) {
            if (match(TokenType::NEWLINE)) continue;
            auto stmt = parse_statement();
            if (stmt) statements.push_back(std::move(stmt));
        }

        consume(TokenType::DEDENT, "Expected dedent at the end of " + context_name + " block.");
    } 
    else if (match(TokenType::LBRACE)) {
        // Curly brace block (inline): e.g. `{ statement; statement }`
        while (!check(TokenType::RBRACE) && !is_at_end()) {
            if (match(TokenType::NEWLINE)) continue;
            auto stmt = parse_statement();
            if (stmt) statements.push_back(std::move(stmt));
        }
        consume(TokenType::RBRACE, "Expected closing '}' at the end of " + context_name + " block.");
        
        // Optional trailing newline after the inline braces block
        match(TokenType::NEWLINE);
    } 
    else {
        error(peek(), "Expected ':' or '{' to start " + context_name + " block.");
    }

    return std::make_unique<BlockStmt>(std::move(statements), loc);
}

// ── Struct Parsing Helpers ───────────────────────────────────────────────────

TypeAnnotation Parser::parse_type_annotation() {
    TypeAnnotation ty;
    
    // Check ownership prefix: `own`
    if (match(TokenType::KW_OWN)) {
        ty.is_owned = true;
    }
    
    // Check borrow reference prefixes: `&` and `&mut`
    if (match(TokenType::AMPERSAND)) {
        ty.is_borrowed = true;
        if (match(TokenType::KW_MUT)) {
            ty.is_mutable_borrow = true;
        }
    }

    // The type name itself: e.g. `int`, `float`, `string`, `bool`, or a ClassName identifier
    if (match(TokenType::IDENTIFIER) || 
        match(TokenType::KW_INT) || 
        match(TokenType::KW_FLOAT) || 
        match(TokenType::KW_STRING_TYPE) || 
        match(TokenType::KW_BOOL) || 
        match(TokenType::KW_VOID)) {
        ty.name = previous().value;
    } else {
        error(peek(), "Expected type name.");
        ty.name = "void";
    }

    // Check optional nullability marker: `?`
    if (match(TokenType::QUESTION)) {
        ty.is_nullable = true;
    }

    return ty;
}

FunctionParam Parser::parse_function_param() {
    SourceLocation loc = peek().location;
    
    // Check for ownership prefixes: `own`, `&`, `&mut`
    TypeAnnotation ty;
    ty.is_owned = false;
    ty.is_borrowed = false;
    ty.is_mutable_borrow = false;
    ty.is_nullable = false;
    
    if (match(TokenType::KW_OWN)) {
        ty.is_owned = true;
    } else if (match(TokenType::AMPERSAND)) {
        ty.is_borrowed = true;
        if (match(TokenType::KW_MUT)) {
            ty.is_mutable_borrow = true;
        }
    }
    
    Token name_tok = consume(TokenType::IDENTIFIER, "Expected parameter name.");
    
    // Support colon: e.g. `a: int` or `a int`
    match(TokenType::COLON);
    
    // Now parse the type annotation
    TypeAnnotation base_ty = parse_type_annotation();
    // Merge ownership from prefix with base type
    ty.name = base_ty.name;
    ty.is_nullable = base_ty.is_nullable;
    // Note: parse_type_annotation() also handles ownership prefixes, so we combine
    if (base_ty.is_owned) ty.is_owned = true;
    if (base_ty.is_borrowed) ty.is_borrowed = true;
    if (base_ty.is_mutable_borrow) ty.is_mutable_borrow = true;
    
    return FunctionParam{name_tok.value, ty, loc};
}

ClassField Parser::parse_class_field() {
    SourceLocation loc = peek().location;
    Token name_tok = consume(TokenType::IDENTIFIER, "Expected class field name.");
    
    // Support colon
    match(TokenType::COLON);
    
    TypeAnnotation ty = parse_type_annotation();
    
    if (!match(TokenType::NEWLINE) && !match(TokenType::SEMICOLON)) {
        error(peek(), "Expected newline or semicolon after class field.");
    }
    return ClassField{name_tok.value, ty, loc};
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 5: Pratt Parsing Expressions
// ═══════════════════════════════════════════════════════════════════════════

std::unique_ptr<Expr> Parser::parse_expression(Precedence precedence) {
    // ── Prefix Parsing ──
    auto prefix_it = prefix_fns_.find(peek().type);
    if (prefix_it == prefix_fns_.end()) {
        error(peek(), "Expected expression. Found token: " + std::string(token_type_to_string(peek().type)) + " (\"" + peek().value + "\")");
        advance();  // Consume the unexpected token to allow error recovery
        return nullptr;
    }
    
    PrefixParseFn prefix = prefix_it->second;
    std::unique_ptr<Expr> left = (this->*prefix)();

    // ── Infix Parsing ──
    // While the next operator has higher precedence (binding power) than the current one,
    // we continue parsing it as an infix operator, pulling in the left expression.
    while (!is_at_end() && precedence < get_precedence(peek().type)) {
        auto infix_it = infix_fns_.find(peek().type);
        if (infix_it == infix_fns_.end()) {
            break;
        }
        InfixParseFn infix = infix_it->second;
        left = (this->*infix)(std::move(left));
    }

    return left;
}

// Prefix: Identifier
std::unique_ptr<Expr> Parser::parse_identifier() {
    Token ident = advance();
    return std::make_unique<IdentifierExpr>(ident.value, ident.location);
}

// Prefix: Literals (Int, Float, String, Boolean, Null, Self)
std::unique_ptr<Expr> Parser::parse_literal() {
    Token lit = advance();
    return std::make_unique<LiteralExpr>(lit);
}

// Prefix: Unary (negation, logical not, borrow, async await)
std::unique_ptr<Expr> Parser::parse_unary() {
    Token op = advance();
    // Parse expression with PREFIX binding power
    std::unique_ptr<Expr> right = parse_expression(Precedence::PREFIX);
    if (op.type == TokenType::KW_AWAIT) {
        return std::make_unique<AwaitExpr>(std::move(right), op.location);
    }
    return std::make_unique<UnaryExpr>(op, std::move(right));
}

// Prefix: Grouped Parentheses: `( expression )`
std::unique_ptr<Expr> Parser::parse_grouped_expr() {
    advance(); // consume opening '('
    std::unique_ptr<Expr> expr = parse_expression(Precedence::LOWEST);
    consume(TokenType::RPAREN, "Expected ')' to close grouped expression.");
    return expr;
}

// Infix: Binary Operations (including Arithmetic, Logic, Comparisons, and Assignments)
std::unique_ptr<Expr> Parser::parse_binary(std::unique_ptr<Expr> left) {
    Token op = advance();
    
    // Right-associative operators (like Assignment `=`) have slightly lower right-side binding power
    Precedence prec = get_precedence(op.type);
    if (op.type == TokenType::EQUAL ||
        op.type == TokenType::PLUS_EQUAL ||
        op.type == TokenType::MINUS_EQUAL ||
        op.type == TokenType::STAR_EQUAL ||
        op.type == TokenType::SLASH_EQUAL) {
        prec = static_cast<Precedence>(static_cast<int>(prec) - 1);
    }

    std::unique_ptr<Expr> right = parse_expression(prec);
    return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
}

// Infix: Function Call: `callee(arg1, arg2)`
std::unique_ptr<Expr> Parser::parse_call(std::unique_ptr<Expr> left) {
    advance(); // consume opening '('
    
    std::vector<std::unique_ptr<Expr>> args;
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parse_expression(Precedence::LOWEST));
        } while (match(TokenType::COMMA));
    }
    
    Token rparen = consume(TokenType::RPAREN, "Expected ')' after function call arguments.");
    return std::make_unique<CallExpr>(std::move(left), std::move(args), rparen.location);
}

// Infix: Member Access: `object.member`
std::unique_ptr<Expr> Parser::parse_member_access(std::unique_ptr<Expr> left) {
    advance(); // consume '.'
    Token member = consume(TokenType::IDENTIFIER, "Expected member name identifier after '.'.");
    return std::make_unique<MemberAccessExpr>(std::move(left), member.value, member.location);
}

// Infix: Indexing: `array[index]`
std::unique_ptr<Expr> Parser::parse_index_expr(std::unique_ptr<Expr> left) {
    SourceLocation loc = peek().location;
    advance(); // consume '['
    
    std::unique_ptr<Expr> idx = parse_expression(Precedence::LOWEST);
    consume(TokenType::RBRACKET, "Expected ']' after index expression.");
    
    return std::make_unique<IndexExpr>(std::move(left), std::move(idx), loc);
}

// Precedence Mapping Table
Precedence Parser::get_precedence(TokenType type) const {
    switch (type) {
        case TokenType::EQUAL:
        case TokenType::PLUS_EQUAL:
        case TokenType::MINUS_EQUAL:
        case TokenType::STAR_EQUAL:
        case TokenType::SLASH_EQUAL:
            return Precedence::ASSIGN;
            
        case TokenType::OR_OR:
            return Precedence::LOGICAL_OR;
            
        case TokenType::AND_AND:
            return Precedence::LOGICAL_AND;
            
        case TokenType::EQUAL_EQUAL:
        case TokenType::BANG_EQUAL:
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL:
            return Precedence::COMPARISON;
            
        case TokenType::PLUS:
        case TokenType::MINUS:
            return Precedence::SUM;
            
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            return Precedence::PRODUCT;
            
        case TokenType::LPAREN:
            return Precedence::CALL;
            
        case TokenType::DOT:
            return Precedence::MEMBER;
            
        case TokenType::LBRACKET:
            return Precedence::INDEX;
            
        // .nvw Python/React precedence (lower than regular expressions)
        case TokenType::KW_PYTHON:
        case TokenType::KW_JSX:
        case TokenType::KW_CSS:
        case TokenType::KW_HTML:
        case TokenType::KW_IMPORT_PYTHON:
        case TokenType::KW_EXPORT_JS:
            return Precedence::LOWEST;
            
        default:
            return Precedence::LOWEST;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 6: .nvw Python/React Parsing Helpers
// ═══════════════════════════════════════════════════════════════════════════

// Python FFI block: `python: ...` or `python: import "module"`
std::unique_ptr<Stmt> Parser::parse_python_ffi_block() {
    SourceLocation loc = previous().location;
    // Parse the python block body - contains import statements
    std::unique_ptr<BlockStmt> body = parse_block_stmt("python FFI block");
    std::vector<std::unique_ptr<Stmt>> imports;
    if (body) {
        imports = std::move(body->statements);
    }
    return std::make_unique<PythonFFIBlockStmt>(std::move(imports), loc);
}

// JSX expression: `<div>Hello</div>`, `<Component/>, or `{expr}`
std::unique_ptr<Expr> Parser::parse_jsx_expr() {
    SourceLocation loc = previous().location;
    // JSX: <Tag attr={val}>children</Tag>  or  {expression}
    if (check(TokenType::LBRACE)) {
        // JSX expression block: {expr}
        advance(); // consume {
        auto expr = parse_expression(Precedence::LOWEST);
        if (!match(TokenType::RBRACE)) {
            error(peek(), "Expected '}' after JSX expression.");
        }
        return std::make_unique<JSXExprExpr>(std::move(expr), loc);
    }

    if (match(TokenType::LESS)) {
        // JSX tag: <div> or <Component/>
        Token tag_token;
        if (check(TokenType::IDENTIFIER)) {
            tag_token = advance();
        } else {
            error(peek(), "Expected tag name in JSX.");
            tag_token = Token{TokenType::IDENTIFIER, "div", previous().location};
        }

        bool self_closing = false;
        std::unique_ptr<Expr> children = nullptr;

        // Parse attributes until '>' or '/>'
        while (!is_at_end() && !check(TokenType::GREATER) && !check(TokenType::SLASH_GREATER)) {
            if (check(TokenType::IDENTIFIER)) {
                advance(); // attribute name
                if (match(TokenType::EQUAL)) {
                    parse_expression(Precedence::LOWEST); // attribute value
                }
            } else {
                advance();
            }
        }

        if (match(TokenType::SLASH_GREATER)) {
            self_closing = true;
        } else if (match(TokenType::GREATER)) {
            // Parse children until the closing tag </name>
            if (!check(TokenType::LESS)) {
                Token child = advance();
                children = std::make_unique<LiteralExpr>(child);
            }
            // Consume closing tag: </name> (or </name)
            match(TokenType::LESS);
            match(TokenType::SLASH);
            if (check(TokenType::IDENTIFIER)) advance();
            match(TokenType::GREATER);
        }

        return std::make_unique<JSXTagExpr>(tag_token.value, std::move(children), self_closing, loc);
    }

    error(peek(), "Expected '<' or '{' after 'jsx'.");
    return nullptr;
}

// CSS-in-JS styles: `css: ...`
std::unique_ptr<Stmt> Parser::parse_css_styles() {
    SourceLocation loc = previous().location;
    // Parse CSS content - for .nvw CSS-in-JS
    std::string css_content;
    // Simplified: read until semicolon or newline
    if (check(TokenType::STRING_LITERAL)) {
        css_content = advance().value;
    }
    return std::make_unique<CSSStylesStmt>(css_content, loc);
}

// HTML template: `html: ...`
std::unique_ptr<Stmt> Parser::parse_html_template() {
    SourceLocation loc = previous().location;
    // Parse HTML template content
    std::string html_content;
    if (check(TokenType::STRING_LITERAL)) {
        html_content = advance().value;
    }
    return std::make_unique<HTMLTemplateStmt>(html_content, loc);
}

// Python import from .nvw: `import_python "module"`
std::unique_ptr<Stmt> Parser::parse_python_import() {
    SourceLocation loc = previous().location;
    // Parse the module name
    std::string module_name;
    if (check(TokenType::STRING_LITERAL)) {
        module_name = advance().value;
    } else if (check(TokenType::IDENTIFIER)) {
        module_name = advance().value;
    }
    return std::make_unique<PythonImportStmt>(module_name, loc);
}

// JS export: `export_js fn name`
std::unique_ptr<Stmt> Parser::parse_js_export() {
    SourceLocation loc = previous().location;
    // Parse the function name to export
    std::string func_name;
    if (check(TokenType::IDENTIFIER)) {
        func_name = advance().value;
    }
    return std::make_unique<JSExportStmt>(func_name, loc);
}

// Mojo-style public visibility declaration
std::unique_ptr<Stmt> Parser::parse_pub_decl() {
    SourceLocation loc = previous().location;
    // Mojo: `pub` can modify following declarations
    // For now, just consume and return empty
    return std::make_unique<EmptyStmt>(loc);
}

// Mojo-style struct declaration
std::unique_ptr<Stmt> Parser::parse_struct_decl() {
    SourceLocation loc = previous().location;
    Token name_token = consume(TokenType::IDENTIFIER, "Expected struct name.");
    std::string name = name_token.value;
    
    // Parse fields
    std::vector<FunctionParam> fields;
    if (check(TokenType::LBRACE)) {
        advance(); // consume {
        while (!check(TokenType::RBRACE) && !is_at_end()) {
            if (match(TokenType::KW_OWN) || match(TokenType::KW_BORROW) || match(TokenType::KW_MUT)) {
                // Parse field with ownership
                // ...
            }
            // Parse field name
            Token field_name = consume(TokenType::IDENTIFIER, "Expected field name.");
            // Parse field type
            // ...
            // Skip to next field or semicolon
            if (!match(TokenType::COMMA)) break;
        }
        consume(TokenType::RBRACE, "Expected '}' at the end of struct body.");
    } else {
        // Struct without braces: `struct Name field: type, field2: type`
        // Parse fields until we hit a semicolon or another keyword
        while (!check(TokenType::SEMICOLON) && !is_at_end() &&
               !check(TokenType::KW_FN) && !check(TokenType::KW_CLASS) &&
               !check(TokenType::KW_INTERFACE) && !check(TokenType::KW_LET) &&
               !check(TokenType::KW_VAR) && !check(TokenType::KW_IF) &&
               !check(TokenType::KW_WHILE) && !check(TokenType::KW_FOR) &&
               !check(TokenType::KW_RETURN) && !check(TokenType::KW_MATCH) &&
               !check(TokenType::KW_TRY) && !check(TokenType::KW_GO) &&
               !check(TokenType::KW_DEFER) && !check(TokenType::KW_UNSAFE)) {
            // Parse field
            Token field_name = consume(TokenType::IDENTIFIER, "Expected field name.");
            // Skip type annotation if present
            match(TokenType::COLON);
            // Skip until comma or semicolon
            if (check(TokenType::COMMA)) advance();
            else if (check(TokenType::SEMICOLON)) break;
        }
        consume(TokenType::SEMICOLON, "Expected ';' at the end of struct declaration.");
    }
    
    return std::make_unique<EmptyStmt>(loc);
}

// Mojo-style enum declaration
std::unique_ptr<Stmt> Parser::parse_enum_decl() {
    SourceLocation loc = previous().location;
    Token name_token = consume(TokenType::IDENTIFIER, "Expected enum name.");
    std::string name = name_token.value;
    
    // Parse enum values
    // For now, just consume and return
    consume(TokenType::COLON, "Expected ':' to start enum body.");
    consume(TokenType::NEWLINE, "Expected newline after enum declaration.");
    consume(TokenType::INDENT, "Expected indented block for enum body.");
    
    // Skip enum body
    while (!check(TokenType::DEDENT) && !is_at_end()) {
        if (match(TokenType::NEWLINE)) continue;
        // Skip identifiers (enum values)
        if (check(TokenType::IDENTIFIER)) advance();
    }
    consume(TokenType::DEDENT, "Expected dedent at the end of enum body.");
    
    return std::make_unique<EmptyStmt>(loc);
}

// Mojo-style using declaration
std::unique_ptr<Stmt> Parser::parse_using_decl() {
    SourceLocation loc = previous().location;
    // Mojo: `using` can bring types into scope
    // For now, consume until semicolon
    if (check(TokenType::SEMICOLON)) {
        return std::make_unique<EmptyStmt>(loc);
    }
    // Skip until semicolon
    while (!check(TokenType::SEMICOLON) && !is_at_end()) advance();
    consume(TokenType::SEMICOLON, "Expected ';' at the end of using declaration.");
    return std::make_unique<EmptyStmt>(loc);
}

// Python-style with statement: `with resource: ...`
std::unique_ptr<Stmt> Parser::parse_with_stmt() {
    SourceLocation loc = previous().location;
    // Parse the context expression(s) up to the block
    while (!is_at_end() && !check(TokenType::COLON) && !check(TokenType::LBRACE)) {
        advance();
    }
    // Consume the body so we stay synchronized; execution semantics are
    // handled by the interpreter/codegen layers.
    std::unique_ptr<BlockStmt> body = parse_block_stmt("with block");
    if (!body) return nullptr;
    return std::make_unique<EmptyStmt>(loc);
}

// Mojo-style type cast expression
std::unique_ptr<Expr> Parser::parse_cast_expr() {
    SourceLocation loc = previous().location;
    // `cast type(expression)`
    std::string target_type;
    if (check(TokenType::IDENTIFIER)) {
        target_type = advance().value;
    } else {
        error(peek(), "Expected type name after 'cast'.");
        return nullptr;
    }
    if (!check(TokenType::LPAREN)) {
        error(peek(), "Expected '(' after type name in 'cast'.");
        return nullptr;
    }
    advance(); // consume (
    auto expr = parse_expression(Precedence::LOWEST);
    consume(TokenType::RPAREN, "Expected ')' after expression in 'cast'.");
    return std::make_unique<CastExpr>(target_type, std::move(expr), loc);
}

// Mojo-style sizeof expression
std::unique_ptr<Expr> Parser::parse_sizeof_expr() {
    SourceLocation loc = previous().location;
    // `sizeof(type)` or `sizeof type` — emit call to builtin size_of
    std::unique_ptr<Expr> inner = nullptr;
    if (match(TokenType::LPAREN)) {
        inner = parse_expression(Precedence::LOWEST);
        consume(TokenType::RPAREN, "Expected ')' after expression in 'sizeof'.");
    } else if (check(TokenType::IDENTIFIER)) {
        inner = std::make_unique<IdentifierExpr>(advance().value, previous().location);
    }
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(std::move(inner));
    return std::make_unique<CallExpr>(
        std::make_unique<IdentifierExpr>("size_of", loc), std::move(args), loc);
}

// Mojo-style alignof expression
std::unique_ptr<Expr> Parser::parse_alignof_expr() {
    SourceLocation loc = previous().location;
    // `alignof(type)` or `alignof type` — emit call to builtin align_of
    std::unique_ptr<Expr> inner = nullptr;
    if (match(TokenType::LPAREN)) {
        inner = parse_expression(Precedence::LOWEST);
        consume(TokenType::RPAREN, "Expected ')' after expression in 'alignof'.");
    } else if (check(TokenType::IDENTIFIER)) {
        inner = std::make_unique<IdentifierExpr>(advance().value, previous().location);
    }
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(std::move(inner));
    return std::make_unique<CallExpr>(
        std::make_unique<IdentifierExpr>("align_of", loc), std::move(args), loc);
}

// Mojo tensor type declaration
std::unique_ptr<Stmt> Parser::parse_tensor_type() {
    SourceLocation loc = previous().location;
    // `Tensor[shape]` or `Tensor`
    // For now, just consume and return empty
    // Full implementation would parse tensor dimensions
    return std::make_unique<EmptyStmt>(loc);
}

// Mojo matrix type declaration
std::unique_ptr<Stmt> Parser::parse_matrix_type() {
    SourceLocation loc = previous().location;
    // `Matrix[shape]` or `Matrix`
    // For now, just consume and return empty
    // Full implementation would parse matrix dimensions
    return std::make_unique<EmptyStmt>(loc);
}

// ── End Mojo Compatibility Parsing ──────────────────────────────────────────
// Python/React Bridge Built-in Registration

} // namespace novium

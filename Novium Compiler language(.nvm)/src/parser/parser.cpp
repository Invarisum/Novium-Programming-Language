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
    if (match(TokenType::KW_EXTERN)) {
        is_extern = true;
        if (check(TokenType::STRING_LITERAL)) {
            extern_name = advance().value;
        }
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

    return std::make_unique<FunctionDeclStmt>(name, std::move(params), ret_type, has_ret, std::move(body), is_async, is_extern, extern_name, loc);
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

        methods.push_back(std::make_unique<FunctionDeclStmt>(m_name, std::move(params), ret_type, has_ret, nullptr, is_async, false, "", method_name_tok.location));
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
                t == TokenType::KW_GO) {
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
            
        default:
            return Precedence::LOWEST;
    }
}

} // namespace novium

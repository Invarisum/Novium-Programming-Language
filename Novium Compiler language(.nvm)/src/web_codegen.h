// ============================================================================
// web_codegen.h — Novium Web Code Generator (Sprint 7)
// ============================================================================
//
// Transpiles Novium code to WebAssembly/JavaScript compatible formats.
// Supports: function translation, type conversion, and JS glue code generation.
//
// ============================================================================

#pragma once

#include <string>
#include <memory>
#include <vector>
#include "parser/ast.h"

namespace novium {

// ============================================================================
// Web Code Gen Config
// ============================================================================

struct WebCodeGenConfig {
    bool emit_source_map = true;
    bool optimize_output = true;
    std::string target = "javascript"; // "javascript" or "wasm"
    std::string module_system = "IIFE"; // "IIFE", "ESModule", "CommonJS"
};

// ============================================================================
// Web Code Gen Result
// ============================================================================

struct WebCodeGenResult {
    // Generated output code
    std::string code;
    // Source map path (if emitted)
    std::string source_map;
    // Any error that occurred
    std::string error_message;
    // Success flag
    bool success = false;

    // Check if code generation succeeded
    explicit operator bool() const { return success; }

    WebCodeGenResult() : success(false) {}
    WebCodeGenResult(std::string code, std::string map, std::string err, bool succ)
        : code(std::move(code)), source_map(std::move(map)),
          error_message(std::move(err)), success(succ) {}
};

// ============================================================================
// Web Code Generator Class
// ============================================================================

class WebCodeGen {
public:
    WebCodeGen(const WebCodeGenConfig& config = WebCodeGenConfig());

    // Main entry point: generate web-compatible code from a parsed program
    WebCodeGenResult generate(const std::vector<std::unique_ptr<novium::Stmt>>& program);

    // Get the generated code
    const std::string& get_code() const { return result_.code; }

    // Get any error that occurred
    const std::string& get_error() const { return result_.error_message; }

private:
    WebCodeGenConfig config_;
    WebCodeGenResult result_;

    // Generate the entire program code
    void generate_program(const std::vector<std::unique_ptr<novium::Stmt>>& program);

    // Generate function declarations
    void generate_function(novium::FunctionDeclStmt* fn);

    // Generate variable declarations
    void generate_var_decl(novium::VarDeclStmt* stmt);

    // Generate print/println statements
    void generate_print(novium::PrintStmt* stmt);
    void generate_println(novium::PrintLnStmt* stmt);

    // Generate expression code
    std::string generate_expr(novium::Expr* expr);

    // Generate literal values
    std::string generate_literal(novium::LiteralExpr* expr);

    // Generate identifier references
    std::string generate_identifier(novium::IdentifierExpr* expr);

    // Generate binary operation code
    std::string generate_binary(novium::BinaryExpr* expr);

    // Generate if statements
    std::string generate_if(novium::IfStmt* stmt);

    // Generate while loops
    std::string generate_while(novium::WhileStmt* stmt);

    // Generate match statements
    std::string generate_match(novium::MatchStmt* stmt);

    // Generate return statements
    std::string generate_return(novium::ReturnStmt* stmt);

    // Get JS type for a Novium type
    std::string get_js_type(novium::TypePtr type);

    // Get JS operator for a Novium operator
    std::string get_js_operator(novium::TokenType op);

    // Emit a function wrapper for JS interop
    std::string emit_js_wrapper(const std::string& func_name, const std::string& func_body);

    // Emit module boilerplate
    std::string emit_module_boilerplate(const std::string& code);
};

// ============================================================================
// Inline Utility Functions
// ============================================================================

// Convert Novium TypeKind to JavaScript type string
inline std::string web_js_type(novium::TypeKind kind) {
    switch (kind) {
        case novium::TypeKind::VOID: return "void";
        case novium::TypeKind::BOOL: return "boolean";
        case novium::TypeKind::INT: return "number";
        case novium::TypeKind::FLOAT: return "number";
        case novium::TypeKind::STRING: return "string";
        case novium::TypeKind::CHAR: return "string";
        case novium::TypeKind::NEVER: return "never";
        default: return "any";
    }
}

// Convert Novium TokenType to JavaScript operator
inline std::string web_js_operator(novium::TokenType op) {
    switch (op) {
        case novium::TokenType::PLUS: return "+";
        case novium::TokenType::MINUS: return "-";
        case novium::TokenType::STAR: return "*";
        case novium::TokenType::SLASH: return "/";
        case novium::TokenType::PERCENT: return "%";
        case novium::TokenType::EQUAL_EQUAL: return "==";
        case novium::TokenType::BANG_EQUAL: return "!=";
        case novium::TokenType::LESS: return "<";
        case novium::TokenType::LESS_EQUAL: return "<=";
        case novium::TokenType::GREATER: return ">";
        case novium::TokenType::GREATER_EQUAL: return ">=";
        case novium::TokenType::AND_AND: return "&&";
        case novium::TokenType::OR_OR: return "||";
        case novium::TokenType::EQUAL: return "=";
        default: return "=";
    }
}

} // namespace novium
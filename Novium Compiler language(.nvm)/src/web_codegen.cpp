// ============================================================================
// web_codegen.cpp — Novium Web Code Generator Implementation (Sprint 7)
// ============================================================================
//
// Transpiles Novium code to JavaScript/WASM compatible formats.
// Supports: function translation, type conversion, and JS glue code generation.
//
// ============================================================================

#include "web_codegen.h"
#include "parser/ast.h"
#include "sema/types.h"

#include <sstream>
#include <algorithm>
#include <iostream>

namespace novium {

// ============================================================================
// WebCodeGen Constructor
// ============================================================================

WebCodeGen::WebCodeGen(const WebCodeGenConfig& config)
    : config_(config),
      result_(WebCodeGenResult()) {}

// ============================================================================
// Main Entry Point: Generate Web Code from Program
// ============================================================================

WebCodeGenResult WebCodeGen::generate(const std::vector<std::unique_ptr<novium::Stmt>>& program) {
    try {
        result_ = WebCodeGenResult();

        // Generate the program code
        generate_program(program);

        if (!result_.success && !result_.error_message.empty()) {
            return result_;
        }

        result_.success = true;
        return result_;
    } catch (const std::exception& e) {
        result_.error_message = "Web code generation exception: " + std::string(e.what());
        return result_;
    } catch (...) {
        result_.error_message = "Unknown web code generation error";
        return result_;
    }
}

// ============================================================================
// Generate Program Code
// ============================================================================

void WebCodeGen::generate_program(const std::vector<std::unique_ptr<novium::Stmt>>& program) {
    std::ostringstream ss;

    // Generate module boilerplate based on target and module system
    ss << emit_module_boilerplate("novium");

    // Generate all top-level declarations
    for (const auto& stmt : program) {
        if (auto* fn = dynamic_cast<novium::FunctionDeclStmt*>(stmt.get())) {
            ss << generate_function(fn) << "\n";
        } else if (auto* cls = dynamic_cast<novium::ClassDeclStmt*>(stmt.get())) {
            ss << generate_class(cls) << "\n";
        } else if (auto* iface = dynamic_cast<novium::InterfaceDeclStmt*>(stmt.get())) {
            ss << generate_interface(iface) << "\n";
        } else if (auto* var = dynamic_cast<novium::VarDeclStmt*>(stmt.get())) {
            ss << generate_var_decl(var) << "\n";
        }
    }

    // End module
    ss << "// End of Novium transpiled code\n";

    result_.code = ss.str();
}

// ============================================================================
// Generate Function Declaration
// ============================================================================

std::string WebCodeGen::generate_function(novium::FunctionDeclStmt* fn) {
    std::ostringstream ss;

    ss << "function " << fn->name << "(";

    for (size_t i = 0; i < fn->params.size(); ++i) {
        const auto& param = fn->params[i];
        ss << param.name;
        if (i < fn->params.size() - 1) {
            ss << ", ";
        }
    }

    ss << ") {\n";

    // Generate function body
    if (fn->body) {
        ss << generate_block(fn->body.get());
    }

    ss << "}\n";
    return ss.str();
}

// ============================================================================
// Generate Block Statements
// ============================================================================

std::string WebCodeGen::generate_block(novium::BlockStmt* block) {
    std::ostringstream ss;

    if (!block || block->statements.empty()) {
        ss << "  // empty block\n";
        return ss.str();
    }

    for (const auto& stmt : block->statements) {
        if (auto* var = dynamic_cast<novium::VarDeclStmt*>(stmt.get())) {
            ss << "  " << generate_var_decl(var) << "\n";
        } else if (auto* expr = dynamic_cast<novium::ExpressionStmt*>(stmt.get())) {
            ss << "  " << generate_expr(expr->expression.get()) << "\n";
        } else if (auto* print = dynamic_cast<novium::PrintStmt*>(stmt.get())) {
            ss << "  " << generate_print(print) << "\n";
        } else if (auto* println = dynamic_cast<novium::PrintLnStmt*>(stmt.get())) {
            ss << "  " << generate_println(println) << "\n";
        } else if (auto* ret = dynamic_cast<novium::ReturnStmt*>(stmt.get())) {
            ss << "  " << generate_return(ret) << "\n";
        } else if (auto* cond = dynamic_cast<novium::IfStmt*>(stmt.get())) {
            ss << "  " << generate_if(cond) << "\n";
        } else if (auto* wh = dynamic_cast<novium::WhileStmt*>(stmt.get())) {
            ss << "  " << generate_while(wh) << "\n";
        } else if (auto* match = dynamic_cast<novium::MatchStmt*>(stmt.get())) {
            ss << "  " << generate_match(match) << "\n";
        }
    }

    return ss.str();
}

// ============================================================================
// Generate Variable Declaration
// ============================================================================

std::string WebCodeGen::generate_var_decl(novium::VarDeclStmt* stmt) {
    std::ostringstream ss;

    // Determine mutability
    std::string keyword = stmt->is_mutable ? "let" : "const";

    // Add type annotation if present
    if (stmt->has_type_annotation) {
        ss << keyword << " " << stmt->name << ": " << get_js_type(/* type */ nullptr);
        if (stmt->initializer) {
            ss << " = " << generate_expr(stmt->initializer.get());
        }
    } else {
        // Inferred type from initializer
        if (stmt->initializer) {
            ss << keyword << " " << stmt->name << " = " << generate_expr(stmt->initializer.get());
        } else {
            ss << keyword << " " << stmt->name << " = null";
        }
    }

    return ss.str();
}

// ============================================================================
// Generate Print Statement
// ============================================================================

std::string WebCodeGen::generate_print(novium::PrintStmt* stmt) {
    std::ostringstream ss;

    if (stmt->value) {
        ss << "console.log(" << generate_expr(stmt->value.get()) << ")";
    } else {
        ss << "console.log()";
    }

    return ss.str();
}

// ============================================================================
// Generate PrintLn Statement
// ============================================================================

std::string WebCodeGen::generate_println(novium::PrintLnStmt* stmt) {
    std::ostringstream ss;

    if (stmt->value) {
        ss << "console.log(" << generate_expr(stmt->value.get()) << ")";
        ss << "; // newline added by println";
    } else {
        ss << "console.log(); // newline";
    }

    return ss.str();
}

// ============================================================================
// Generate Expression
// ============================================================================

std::string WebCodeGen::generate_expr(novium::Expr* expr) {
    if (!expr) return "null";

    if (auto* id = dynamic_cast<novium::IdentifierExpr*>(expr)) {
        return generate_identifier(id);
    }
    if (auto* lit = dynamic_cast<novium::LiteralExpr*>(expr)) {
        return generate_literal(lit);
    }
    if (auto* bin = dynamic_cast<novium::BinaryExpr*>(expr)) {
        return generate_binary(bin);
    }
    if (auto* call = dynamic_cast<novium::CallExpr*>(expr)) {
        return "(" + generate_call(call) + ")";
    }
    if (auto* un = dynamic_cast<novium::UnaryExpr*>(expr)) {
        return "(" + generate_unary(un) + ")";
    }
    if (auto* mem = dynamic_cast<novium::MemberAccessExpr*>(expr)) {
        return "(" + generate_member_access(mem) + ")";
    }
    if (auto* idx = dynamic_cast<novium::IndexExpr*>(expr)) {
        return "(" + generate_index(idx) + ")";
    }
    if (auto* cast = dynamic_cast<novium::CastExpr*>(expr)) {
        // JS has no explicit casts; emit the expression with a comment
        return "(/* cast to " + cast->target_type + " */ " +
               generate_expr(cast->expression.get()) + ")";
    }
    if (auto* aw = dynamic_cast<novium::AwaitExpr*>(expr)) {
        return "(await " + generate_expr(aw->value.get()) + ")";
    }
    if (auto* jsx = dynamic_cast<novium::JSXTagExpr*>(expr)) {
        std::string out = "<" + jsx->tag_name;
        if (jsx->is_self_closing) {
            out += " />";
        } else {
            out += ">";
            if (jsx->children) {
                out += generate_expr(jsx->children.get());
            }
            out += "</" + jsx->tag_name + ">";
        }
        return out;
    }
    if (auto* jsx_expr = dynamic_cast<novium::JSXExprExpr*>(expr)) {
        return "{ " + generate_expr(jsx_expr->expression.get()) + " }";
    }

    return "null";
}

// ============================================================================
// Generate Literal
// ============================================================================

std::string WebCodeGen::generate_literal(novium::LiteralExpr* expr) {
    switch (expr->token.type) {
        case novium::TokenType::INTEGER_LITERAL:
            return expr->token.value; // Already a number literal

        case novium::TokenType::FLOAT_LITERAL:
            return expr->token.value; // Already a float literal

        case novium::TokenType::STRING_LITERAL: {
            std::string val = expr->token.value;
            // Remove surrounding quotes
            if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                val = val.substr(1, val.size() - 2);
            }
            return "\"" + val + "\""; // JavaScript string literal
        }

        case novium::TokenType::KW_TRUE:
            return "true";

        case novium::TokenType::KW_FALSE:
            return "false";

        case novium::TokenType::KW_NULL:
            return "null";

        default:
            return "null";
    }
}

// ============================================================================
// Generate Identifier
// ============================================================================

std::string WebCodeGen::generate_identifier(novium::IdentifierExpr* expr) {
    return expr->name;
}

// ============================================================================
// Generate Binary Operation
// ============================================================================

std::string WebCodeGen::generate_binary(novium::BinaryExpr* expr) {
    std::ostringstream ss;

    std::string left = generate_expr(expr->left.get());
    std::string op = web_js_operator(expr->op.type);
    std::string right = generate_expr(expr->right.get());

    ss << left << " " << op << " " << right;
    return ss.str();
}

// ============================================================================
// Generate Call Expression
// ============================================================================

std::string WebCodeGen::generate_call(novium::CallExpr* expr) {
    std::ostringstream ss;

    // Get the callee name
    if (auto* id = dynamic_cast<novium::IdentifierExpr*>(expr->callee.get())) {
        ss << id->name << "(";

        for (size_t i = 0; i < expr->arguments.size(); ++i) {
            ss << generate_expr(expr->arguments[i].get());
            if (i < expr->arguments.size() - 1) {
                ss << ", ";
            }
        }

        ss << ")";
    } else {
        // Complex callee (member access, etc.)
        ss << "(" << generate_expr(expr->callee.get()) << "(";

        for (size_t i = 0; i < expr->arguments.size(); ++i) {
            ss << generate_expr(expr->arguments[i].get());
            if (i < expr->arguments.size() - 1) {
                ss << ", ";
            }
        }

        ss << "))";
    }

    return ss.str();
}

// ============================================================================
// Generate Unary Expression
// ============================================================================

std::string WebCodeGen::generate_unary(novium::UnaryExpr* expr) {
    std::ostringstream ss;

    std::string op;
    switch (expr->op.type) {
        case novium::TokenType::MINUS: op = "-"; break;
        case novium::TokenType::BANG: op = "!"; break;
        case novium::TokenType::KW_AWAIT: op = "await"; break;
        default: op = "?"; break;
    }

    std::string right = generate_expr(expr->right.get());

    ss << op << right;
    return ss.str();
}

// ============================================================================
// Generate Member Access
// ============================================================================

std::string WebCodeGen::generate_member_access(novium::MemberAccessExpr* expr) {
    std::ostringstream ss;

    if (auto* id = dynamic_cast<novium::IdentifierExpr*>(expr->object.get())) {
        // Handle "self.name" pattern
        if (id->name == "self") {
            ss << "this." << expr->member_name;
        } else {
            ss << id->name << "." << expr->member_name;
        }
    } else {
        ss << generate_expr(expr->object.get()) << "." << expr->member_name;
    }

    return ss.str();
}

// ============================================================================
// Generate Index Expression
// ============================================================================

std::string WebCodeGen::generate_index(novium::IndexExpr* expr) {
    std::ostringstream ss;

    ss << generate_expr(expr->object.get()) << "[" << generate_expr(expr->index.get()) << "]";
    return ss.str();
}

// ============================================================================
// Generate Return Statement
// ============================================================================

std::string WebCodeGen::generate_return(novium::ReturnStmt* stmt) {
    std::ostringstream ss;

    if (stmt->value) {
        ss << "return " << generate_expr(stmt->value.get()) << ";";
    } else {
        ss << "return;";
    }

    return ss.str();
}

// ============================================================================
// Generate If Statement
// ============================================================================

std::string WebCodeGen::generate_if(novium::IfStmt* stmt) {
    std::ostringstream ss;

    std::string condition = generate_expr(stmt->condition.get());
    ss << "if (" << condition << ") {\n";

    // Then branch
    if (stmt->then_branch) {
        ss << generate_block(stmt->then_branch.get());
    }

    ss << "}\n";

    // Elif branches
    for (const auto& branch : stmt->elif_branches) {
        ss << "else if (" << generate_expr(branch.condition.get()) << ") {\n";
        if (branch.block) {
            ss << generate_block(branch.block.get());
        }
        ss << "}\n";
    }

    // Else branch
    if (stmt->else_branch) {
        ss << "else {\n";
        if (stmt->else_branch) {
            ss << generate_block(stmt->else_branch.get());
        }
        ss << "}\n";
    }

    return ss.str();
}

// ============================================================================
// Generate While Loop
// ============================================================================

std::string WebCodeGen::generate_while(novium::WhileStmt* stmt) {
    std::ostringstream ss;

    std::string condition = generate_expr(stmt->condition.get());
    ss << "while (" << condition << ") {\n";

    if (stmt->body) {
        ss << generate_block(stmt->body.get());
    }

    ss << "}\n";
    return ss.str();
}

// ============================================================================
// Generate Match Statement
// ============================================================================

std::string WebCodeGen::generate_match(novium::MatchStmt* stmt) {
    std::ostringstream ss;

    std::string subject = generate_expr(stmt->subject.get());
    ss << "switch(" << subject << ") {\n";

    for (const auto& arm : stmt->arms) {
        std::string pattern = generate_expr(arm.pattern.get());
        ss << "case " << pattern << ":\n";
        if (arm.body) {
            if (auto* block = dynamic_cast<novium::BlockStmt*>(arm.body.get())) {
                ss << generate_block(block);
            } else if (auto* ret = dynamic_cast<novium::ReturnStmt*>(arm.body.get())) {
                ss << "  " << generate_return(ret) << "\n";
            } else if (auto* expr_stmt = dynamic_cast<novium::ExpressionStmt*>(arm.body.get())) {
                ss << "  " << generate_expr(expr_stmt->expression.get()) << ";\n";
            }
        }
        ss << "break;\n";
    }

    // Default case (wildcard _)
    ss << "default:\n";
    // Would need a default body - for now just fall through
    ss << "// no default case handled\n";

    ss << "}\n";
    return ss.str();
}

// ============================================================================
// Get JS Type for Novium Type
// ============================================================================

std::string WebCodeGen::get_js_type(novium::TypePtr type) {
    if (!type) return "any";

    return novium::web_js_type(type->kind);
}

// ============================================================================
// Get JS Operator
// ============================================================================

std::string WebCodeGen::get_js_operator(novium::TokenType op) {
    return novium::web_js_operator(op);
}

// ============================================================================
// Emit JS Wrapper
// ============================================================================

std::string WebCodeGen::emit_js_wrapper(const std::string& func_name, const std::string& func_body) {
    std::ostringstream ss;

    ss << "// Auto-generated wrapper for: " << func_name << "\n";
    ss << "function " << func_name << "() {\n";
    ss << func_body;
    ss << "}\n";

    return ss.str();
}

// ============================================================================
// Emit Module Boilerplate
// ============================================================================

std::string WebCodeGen::emit_module_boilerplate(const std::string& code) {
    std::ostringstream ss;

    if (config_.target == "javascript") {
        // IIFE (Immediately Invoked Function Expression) pattern
        ss << "(function() {\n";
        ss << "  // Novium transpiled code\n";
        ss << code;
        ss << "\n})();\n";
    } else if (config_.target == "wasm") {
        // For WASM, we emit ES module compatible code that can be loaded by Wasm runtime
        ss << "// WASM module - Novium transpiled code\n";
        ss << "export function noviumMain() {\n";
        ss << code;
        ss << "\n}\n";
        // Note: In a full WASM implementation, this would be compiled to .wasm
        // using the WebAssembly binary toolkit (WAT/WASM format)
    }

    return ss.str();
}

// ============================================================================
// Generate Class Declaration (stub)
// ============================================================================

std::string WebCodeGen::generate_class(novium::ClassDeclStmt* cls) {
    std::ostringstream ss;

    ss << "// Class transpilation not fully implemented in Sprint 7\n";
    ss << "// Would generate JavaScript class syntax\n";
    ss << "class " << cls->name << " {\n";
    ss << "  constructor() {\n";
    ss << "    // Initialize class fields\n";
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

// ============================================================================
// Generate Interface Declaration (stub)
// ============================================================================

std::string WebCodeGen::generate_interface(novium::InterfaceDeclStmt* iface) {
    std::ostringstream ss;

    ss << "// Interface transpilation not fully implemented in Sprint 7\n";
    ss << "// Would generate JavaScript interface/type definitions\n";
    ss << "// For now, export as type annotations\n";
    ss << "// interface " << iface->name << " {\n";
    ss << "//   // method signatures\n";
    ss << "// }\n";

    return ss.str();
}

} // namespace novium
// ============================================================================
// migratir.h — Migration Intermediate Representation for Novium
// ============================================================================
//
// Defines a language-agnostic Intermediate Representation (IR) that serves as
// the common backbone for bidirectional translation between Novium and other
// languages (C++, Go, Rust, Python).
//
// ARCHITECTURE:
//   Source Language Parser  -->  Migration IR  -->  Target Language Generator
//   Novium AST              -->  Migration IR  -->  Target Language Generator
//
// This design decouples source/target language knowledge, allowing new
// language translations to be added without modifying existing code paths.
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "parser/ast.h"

namespace novium::migratir {

// ============================================================================
// Node Types in the Migration IR
// ============================================================================

// Base class for all IR nodes
class IRNode {
public:
    virtual ~IRNode() = default;
    virtual std::string kind() const = 0;
    virtual std::string to_string() const = 0;
};

// Module node - top-level container
class IRModule : public IRNode {
public:
    std::string name;
    std::vector<std::unique_ptr<IRNode>> items;

    IRModule(std::string name) : name(std::move(name)) {}

    std::string kind() const override { return "module"; }
    std::string to_string() const override;
};

// Function node
class IRFunc : public IRNode {
public:
    std::string name;
    std::vector<std::string> param_names;
    std::vector<std::string> param_types;
    std::string return_type;
    std::vector<std::unique_ptr<IRStmt>> body;
    bool is_async;

    IRFunc(std::string name, bool is_async = false)
        : name(std::move(name)), is_async(is_async) {}

    std::string kind() const override { return "function"; }
    std::string to_string() const override;
};

// Parameter node
class IRParam : public IRNode {
public:
    std::string name;
    std::string type; // Type as string in IR

    IRParam(std::string name, std::string type) : name(std::move(name)), type(std::move(type)) {}

    std::string kind() const override { return "param"; }
    std::string to_string() const override;
};

// Statement types in IR
class IRStmt : public IRNode {
public:
    virtual ~IRStmt() = default;
    std::string kind() const override = 0;
};

// Block statement (sequence)
class IRBlock : public IRStmt {
public:
    std::vector<std::unique_ptr<IRStmt>> stmts;

    IRBlock() = default;
    std::string kind() const override { return "block"; }
    std::string to_string() const override;
};

// Expression statement
class IREXPRStmt : public IRStmt {
public:
    std::unique_ptr<IRExpr> expr;

    IREXPRStmt(std::unique_ptr<IRExpr> expr) : expr(std::move(expr)) {}
    std::string kind() const override { return "expr_stmt"; }
    std::string to_string() const override;
};

// Return statement
class IRReturn : public IRStmt {
public:
    std::unique_ptr<IRExpr> value;

    IRReturn(std::unique_ptr<IRExpr> value = nullptr) : value(std::move(value)) {}
    std::string kind() const override { return "return"; }
    std::string to_string() const override;
};

// If statement
class IRIf : public IRStmt {
public:
    std::unique_ptr<IRExpr> condition;
    std::unique_ptr<IRStmt> then_branch;
    std::unique_ptr<IRStmt> else_branch;

    IRIf(std::unique_ptr<IRExpr> cond, std::unique_ptr<IRStmt> then_b,
         std::unique_ptr<IRStmt> else_b = nullptr)
        : condition(std::move(cond)), then_branch(std::move(then_b)),
          else_branch(std::move(else_b)) {}
    std::string kind() const override { return "if"; }
    std::string to_string() const override;
};

// While statement
class IRWhile : public IRStmt {
public:
    std::unique_ptr<IRExpr> condition;
    std::unique_ptr<IRStmt> body;

    IRWhile(std::unique_ptr<IRExpr> cond, std::unique_ptr<IRStmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    std::string kind() const override { return "while"; }
    std::string to_string() const override;
};

// For statement (C-style)
class IRFor : public IRStmt {
public:
    std::unique_ptr<IRExpr> init;
    std::unique_ptr<IRExpr> cond;
    std::unique_ptr<IRExpr> inc;
    std::unique_ptr<IRStmt> body;

    IRFor(std::unique_ptr<IRExpr> init, std::unique_ptr<IRExpr> cond,
          std::unique_ptr<IRExpr> inc, std::unique_ptr<IRStmt> body)
        : init(std::move(init)), cond(std::move(cond)),
          inc(std::move(inc)), body(std::move(body)) {}
    std::string kind() const override { return "for"; }
    std::string to_string() const override;
};

// Expression types in IR
class IRExpr : public IRNode {
public:
    virtual ~IRExpr() = default;
    std::string kind() const override = 0;
};

// Identifier expression
class IRIdentifier : public IRExpr {
public:
    std::string name;

    IRIdentifier(std::string name) : name(std::move(name)) {}
    std::string kind() const override { return "identifier"; }
    std::string to_string() const override;
};

// Integer literal
class IRIntLiteral : public IRExpr {
public:
    int64_t value;

    IRIntLiteral(int64_t value) : value(value) {}
    std::string kind() const override { return "int_literal"; }
    std::string to_string() const override;
};

// Float literal
class IRFloatLiteral : public IRExpr {
public:
    double value;

    IRFloatLiteral(double value) : value(value) {}
    std::string kind() const override { return "float_literal"; }
    std::string to_string() const override;
};

// String literal
class IRStringLiteral : public IRExpr {
public:
    std::string value;

    IRStringLiteral(std::string value) : value(std::move(value)) {}
    std::string kind() const override { return "string_literal"; }
    std::string to_string() const override;
};

// Binary operation
class IRBinaryOp : public IRExpr {
public:
    std::string op; // +, -, *, /, ==, !=, <, >, &&
    std::unique_ptr<IRExpr> left;
    std::unique_ptr<IRExpr> right;

    IRBinaryOp(std::string op, std::unique_ptr<IRExpr> left,
               std::unique_ptr<IRExpr> right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}
    std::string kind() const override { return "binary_op"; }
    std::string to_string() const override;
};

// Function call
IRCallExpr : public IRExpr {
public:
    std::string callee;
    std::vector<std::unique_ptr<IRExpr>> args;

    IRCallExpr(std::string callee, std::vector<std::unique_ptr<IRExpr>> args)
        : callee(std::move(callee)), args(std::move(args)) {}
    std::string kind() const override { return "call"; }
    std::string to_string() const override;
};

// Variable declaration
class IRVarDecl : public IRStmt {
public:
    std::string name;
    std::string type;
    bool is_mutable;

    IRVarDecl(std::string name, std::string type, bool is_mutable = false)
        : name(std::move(name)), type(std::move(type)), is_mutable(is_mutable) {}
    std::string kind() const override { return "var_decl"; }
    std::string to_string() const override;
};

// ============================================================================
// Translation Helpers
// ============================================================================

// Convert Novium AST to Migration IR
std::unique_ptr<IRModule> novium_ast_to_ir(const std::vector<std::unique_ptr<novium::Stmt>>& program);

// Convert Migration IR back to Novium AST (partial - round-trip)
std::vector<std::unique_ptr<novium::Stmt>> ir_to_novium_ast(const IRModule& module);

// Get IR type string from Novium TypePtr
std::string type_to_ir(novium::TypePtr type);

// Get IR type string from Novium TypeAnnotation
std::string annot_to_ir(novium::TypeAnnotation annot);

// Map Novium TokenType to IR operator string
std::string token_to_ir_op(novium::TokenType type);

// ============================================================================
// Target Language Code Generation
// ============================================================================

// Generate C++ code from Migration IR
std::string ir_to_cpp(const IRModule& module);

// Generate Go code from Migration IR
std::string ir_to_go(const IRModule& module);

// Generate Python code from Migration IR
std::string ir_to_python(const IRModule& module);

// ============================================================================
// Source Language Parsers (partial skeletons)
// ============================================================================

// Parse Go source code into Migration IR (subset: functions, expressions)
std::unique_ptr<IRModule> go_source_to_ir(const std::string& source);

// Parse Python source code into Migration IR (subset: functions, expressions)
std::unique_ptr<IRModule> python_source_to_ir(const std::string& source);

// Parse Rust source code into Migration IR (subset: functions, expressions)
std::unique_ptr<IRModule> rust_source_to_ir(const std::string& source);

// ============================================================================

} // namespace novium::migratir
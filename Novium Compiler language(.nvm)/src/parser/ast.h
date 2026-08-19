// ============================================================================
// ast.h — Abstract Syntax Tree Node Definitions for Novium
// ============================================================================
//
// WHAT THE AST IS:
// The Abstract Syntax Tree (AST) is the middle-ground representation of a
// program. While tokens are a flat stream of words, the AST represents the
// hierarchical grammar structure of code.
//
// Example: "let x = 1 + 2" becomes:
//
//         VarDeclStmt ("x")
//             └── BinaryExpr (+)
//                   ├── Literal (1)
//                   └── Literal (2)
//
// MEMORY MANAGEMENT:
// We use std::unique_ptr for parent-child links. Since the AST is a strict
// tree, each node has exactly one parent. unique_ptr guarantees that deleting
// the root node (the Program node) automatically deletes the entire tree
// without memory leaks.
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "lexer/token.h"

namespace novium {

// Forward declarations
struct ASTVisitor;
class Stmt;
class Expr;

// ── Type Annotation ──────────────────────────────────────────────────────────
// Represents a type written in source code, e.g., "own string?" or "&mut int"

struct TypeAnnotation {
    std::string name;             // Base name of the type (e.g. "int", "string")
    bool is_nullable = false;      // True if it ends with "?" (e.g. "int?")
    bool is_owned = false;         // True if prefixed with "own"
    bool is_borrowed = false;      // True if prefixed with "&"
    bool is_mutable_borrow = false;// True if prefixed with "&mut"

    std::string to_string() const {
        std::string s;
        if (is_owned) s += "own ";
        if (is_borrowed) {
            s += "&";
            if (is_mutable_borrow) s += "mut ";
        }
        s += name;
        if (is_nullable) s += "?";
        return s;
    }
};

// ── Base AST Node ────────────────────────────────────────────────────────────

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor* visitor) = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
// EXPRESSIONS (Evaluate to a value)
// ═══════════════════════════════════════════════════════════════════════════

class Expr : public ASTNode {
public:
    virtual ~Expr() = default;
    virtual std::unique_ptr<Expr> clone() const = 0;
};

// Identifier: e.g. "x", "my_var"
class IdentifierExpr : public Expr {
public:
    std::string name;
    SourceLocation location;

    IdentifierExpr(std::string name, SourceLocation loc)
        : name(std::move(name)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        return std::make_unique<IdentifierExpr>(name, location);
    }
};

// Literal values: e.g. "42", "3.14", "\"hello\"", "true", "null"
class LiteralExpr : public Expr {
public:
    Token token; // Holds the literal token (INTEGER_LITERAL, STRING_LITERAL, etc.)

    explicit LiteralExpr(Token tok) : token(std::move(tok)) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        return std::make_unique<LiteralExpr>(token);
    }
};

// Unary expression: e.g. "-x", "!flag", "&mut value"
class UnaryExpr : public Expr {
public:
    Token op; // Operator token (MINUS, BANG, AMPERSAND, etc.)
    std::unique_ptr<Expr> right;

    UnaryExpr(Token op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        return std::make_unique<UnaryExpr>(op, right ? right->clone() : nullptr);
    }
};

// Binary expression: e.g. "a + b", "x * y", "count == 10"
class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op; // Operator token (PLUS, STAR, EQUAL_EQUAL, etc.)
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        return std::make_unique<BinaryExpr>(
            left ? left->clone() : nullptr, op, right ? right->clone() : nullptr);
    }
};

// Function/Method call: e.g. "print(x, y)", "calculate()"
class CallExpr : public Expr {
public:
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    SourceLocation rparen_location; // Location of ")" for error reporting

    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args, SourceLocation rparen)
        : callee(std::move(callee)), arguments(std::move(args)), rparen_location(rparen) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        std::vector<std::unique_ptr<Expr>> cloned_args;
        cloned_args.reserve(arguments.size());
        for (const auto& arg : arguments) {
            cloned_args.push_back(arg ? arg->clone() : nullptr);
        }
        return std::make_unique<CallExpr>(
            callee ? callee->clone() : nullptr, std::move(cloned_args), rparen_location);
    }
};

// Member access: e.g. "object.field", "self.width"
class MemberAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string member_name;
    SourceLocation member_location;

    MemberAccessExpr(std::unique_ptr<Expr> obj, std::string name, SourceLocation loc)
        : object(std::move(obj)), member_name(std::move(name)), member_location(loc) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        return std::make_unique<MemberAccessExpr>(
            object ? object->clone() : nullptr, member_name, member_location);
    }
};

// Await expression: e.g. "await fetch()"
class AwaitExpr : public Expr {
public:
    std::unique_ptr<Expr> value;
    SourceLocation location;

    AwaitExpr(std::unique_ptr<Expr> val, SourceLocation loc)
        : value(std::move(val)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        return std::make_unique<AwaitExpr>(
            value ? value->clone() : nullptr, location);
    }
};

// Array index: e.g. "arr[0]" or "matrix[i]"
class IndexExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::unique_ptr<Expr> index;
    SourceLocation location;

    IndexExpr(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx, SourceLocation loc)
        : object(std::move(obj)), index(std::move(idx)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
    std::unique_ptr<Expr> clone() const override {
        return std::make_unique<IndexExpr>(
            object ? object->clone() : nullptr, index ? index->clone() : nullptr, location);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// STATEMENTS (Perform actions, do not evaluate to values)
// ═══════════════════════════════════════════════════════════════════════════

class Stmt : public ASTNode {};

// Block statement: e.g. a sequence of statements inside an indented block or braces
class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    SourceLocation location;

    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts, SourceLocation loc)
        : statements(std::move(stmts)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Variable declaration: e.g. "let x = 5" or "var count: int = 0"
class VarDeclStmt : public Stmt {
public:
    std::string name;
    bool is_mutable;               // true if declared with 'var', false if 'let'
    TypeAnnotation type;          // Optional type annotation (checked during analysis)
    bool has_type_annotation;
    std::unique_ptr<Expr> initializer; // Optional initializer expression
    SourceLocation location;

    VarDeclStmt(std::string name, bool is_mut, TypeAnnotation ty, bool has_ty,
                std::unique_ptr<Expr> init, SourceLocation loc)
        : name(std::move(name)), is_mutable(is_mut), type(std::move(ty))
        , has_type_annotation(has_ty), initializer(std::move(init)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Expression statement: e.g. "x = 10" or "print(x)" executed as a statement
class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    explicit ExpressionStmt(std::unique_ptr<Expr> expr)
        : expression(std::move(expr)) {}

    void accept(ASTVisitor* visitor) override;
};

// Print statement: e.g. "print(x)" - prints value(s) with spaces between
class PrintStmt : public Stmt {
public:
    std::unique_ptr<Expr> value; // What to print

    explicit PrintStmt(std::unique_ptr<Expr> val)
        : value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override;
};

// Print ln statement: e.g. "println(x)" - prints value(s) with newline
class PrintLnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value; // What to print

    explicit PrintLnStmt(std::unique_ptr<Expr> val)
        : value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override;
};

// Empty statement: e.g. ";" - no-op, used for standalone semicolons
class EmptyStmt : public Stmt {
public:
    EmptyStmt() {}

    void accept(ASTVisitor* visitor) override;
};

// Function Parameter: e.g. "name string" or "own data Buffer"
struct FunctionParam {
    std::string name;
    TypeAnnotation type;
    SourceLocation location;
};

// Function declaration: e.g. "fn add(a int, b int) int:"
class FunctionDeclStmt : public Stmt {
public:
    std::string name;
    std::vector<FunctionParam> params;
    TypeAnnotation return_type;
    bool has_return_type;
    std::unique_ptr<BlockStmt> body;
    bool is_async;
    bool is_extern;
    std::string extern_name; // C ABI function name (empty = use Novium name)
    SourceLocation location;

    FunctionDeclStmt(std::string name, std::vector<FunctionParam> params,
                     TypeAnnotation ret_type, bool has_ret,
                     std::unique_ptr<BlockStmt> body, bool is_async, bool is_extern,
                     std::string extern_name, SourceLocation loc)
        : name(std::move(name)), params(std::move(params)), return_type(std::move(ret_type))
        , has_return_type(has_ret), body(std::move(body)), is_async(is_async),
        is_extern(is_extern), extern_name(std::move(extern_name)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Class Field: e.g. "x float" inside class
struct ClassField {
    std::string name;
    TypeAnnotation type;
    SourceLocation location;
};

// Class declaration: e.g. "class Circle extends Shape implements Drawable:"
class ClassDeclStmt : public Stmt {
public:
    std::string name;
    std::string base_class; // Empty if no inheritance
    bool has_base_class;
    std::vector<std::string> interfaces;
    std::vector<ClassField> fields;
    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
    SourceLocation location;

    ClassDeclStmt(std::string name, std::string base, bool has_base,
                  std::vector<std::string> ifaces, std::vector<ClassField> fields,
                  std::vector<std::unique_ptr<FunctionDeclStmt>> methods, SourceLocation loc)
        : name(std::move(name)), base_class(std::move(base)), has_base_class(has_base)
        , interfaces(std::move(ifaces)), fields(std::move(fields))
        , methods(std::move(methods)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Interface declaration: e.g. "interface Drawable:"
class InterfaceDeclStmt : public Stmt {
public:
    std::string name;
    // Interface methods are declarations without bodies
    std::vector<std::unique_ptr<FunctionDeclStmt>> methods;
    SourceLocation location;

    InterfaceDeclStmt(std::string name, std::vector<std::unique_ptr<FunctionDeclStmt>> methods, SourceLocation loc)
        : name(std::move(name)), methods(std::move(methods)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// If branch struct for "elif" chaining
struct ElifBranch {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> block;
};

// If statement: e.g. "if x > 0: ... elif x == 0: ... else: ..."
class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> then_branch;
    std::vector<ElifBranch> elif_branches;
    std::unique_ptr<BlockStmt> else_branch; // Nullable
    SourceLocation location;

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> then_b,
           std::vector<ElifBranch> elif_b, std::unique_ptr<BlockStmt> else_b, SourceLocation loc)
        : condition(std::move(cond)), then_branch(std::move(then_b))
        , elif_branches(std::move(elif_b)), else_branch(std::move(else_b)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// While loop: e.g. `while count < 10: ...`
class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;
    SourceLocation location;

    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> loop_body, SourceLocation loc)
        : condition(std::move(cond)), body(std::move(loop_body)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Match Arm: e.g. "Circle(r) => print(r)"
struct MatchArm {
    std::unique_ptr<Expr> pattern; // Value literal, variant type, or identifier (e.g. wildcard `_`)
    std::unique_ptr<Stmt> body;    // Statement executed on match
};

// Match statement: e.g. "match shape:"
class MatchStmt : public Stmt {
public:
    std::unique_ptr<Expr> subject;
    std::vector<MatchArm> arms;
    SourceLocation location;

    MatchStmt(std::unique_ptr<Expr> subj, std::vector<MatchArm> arms, SourceLocation loc)
        : subject(std::move(subj)), arms(std::move(arms)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Return statement: e.g. "return a + b"
class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value; // Nullable (e.g. return void)
    SourceLocation location;

    ReturnStmt(std::unique_ptr<Expr> val, SourceLocation loc)
        : value(std::move(val)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Catch block: e.g. "catch IOError as e:"
struct CatchBlock {
    std::string exception_type; // e.g. "IOError" (or empty for match-all)
    bool has_exception_type;
    std::string exception_var;  // e.g. "e" (or empty if unused)
    bool has_exception_var;
    std::unique_ptr<BlockStmt> body;
    SourceLocation location;
};

// Try-Catch-Finally statement: e.g. "try: ... catch e: ... finally: ..."
class TryCatchStmt : public Stmt {
public:
    std::unique_ptr<BlockStmt> try_block;
    std::vector<CatchBlock> catch_blocks;
    std::unique_ptr<BlockStmt> finally_block; // Nullable
    SourceLocation location;

    TryCatchStmt(std::unique_ptr<BlockStmt> try_b, std::vector<CatchBlock> catch_b,
                 std::unique_ptr<BlockStmt> finally_b, SourceLocation loc)
        : try_block(std::move(try_b)), catch_blocks(std::move(catch_b))
        , finally_block(std::move(finally_b)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Go statement (starts lightweight thread): e.g. "go calculate(x)"
class GoStmt : public Stmt {
public:
    std::unique_ptr<Expr> call; // Must be a CallExpr or Lambda expression
    SourceLocation location;

    GoStmt(std::unique_ptr<Expr> call, SourceLocation loc)
        : call(std::move(call)), location(loc) {}

    void accept(ASTVisitor* visitor) override;
};

// Panic statement: e.g. "panic('error message')" or "panic"
class PanicStmt : public Stmt {
public:
    std::unique_ptr<Expr> message; // Optional error message

    explicit PanicStmt(std::unique_ptr<Expr> msg = nullptr)
        : message(std::move(msg)) {}

    void accept(ASTVisitor* visitor) override;
};

// ═══════════════════════════════════════════════════════════════════════════
// VISITOR PATTERN INTERFACE
// ═══════════════════════════════════════════════════════════════════════════
// The Visitor Pattern separates data structures (AST nodes) from the
// algorithms acting on them (Type checking, IR Codegen, AST printing).
// This prevents us from cluttering the AST classes with compiler-logic code.

struct ASTVisitor {
    virtual ~ASTVisitor() = default;

    // Expressions
    virtual void visit(IdentifierExpr* expr) = 0;
    virtual void visit(LiteralExpr* expr) = 0;
    virtual void visit(UnaryExpr* expr) = 0;
    virtual void visit(BinaryExpr* expr) = 0;
    virtual void visit(CallExpr* expr) = 0;
    virtual void visit(MemberAccessExpr* expr) = 0;
    virtual void visit(AwaitExpr* expr) = 0;
    virtual void visit(IndexExpr* expr) = 0;

    // Statements
    virtual void visit(BlockStmt* stmt) = 0;
    virtual void visit(VarDeclStmt* stmt) = 0;
    virtual void visit(ExpressionStmt* stmt) = 0;
    virtual void visit(PrintStmt* stmt) = 0;
    virtual void visit(PrintLnStmt* stmt) = 0;
    virtual void visit(EmptyStmt* stmt) = 0;
    virtual void visit(FunctionDeclStmt* stmt) = 0;
    virtual void visit(ClassDeclStmt* stmt) = 0;
    virtual void visit(InterfaceDeclStmt* stmt) = 0;
    virtual void visit(IfStmt* stmt) = 0;
    virtual void visit(WhileStmt* stmt) = 0;
    virtual void visit(MatchStmt* stmt) = 0;
    virtual void visit(ReturnStmt* stmt) = 0;
    virtual void visit(TryCatchStmt* stmt) = 0;
    virtual void visit(GoStmt* stmt) = 0;
    virtual void visit(PanicStmt* stmt) = 0;
};

} // namespace novium

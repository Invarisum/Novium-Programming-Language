// ============================================================================
// type_checker.h — Type Checker for Novium
// ============================================================================
//
// Performs semantic analysis and type checking on the AST.
// Features:
// - Bidirectional type checking (inference + checking)
// - Hindley-Milner style let-polymorphism for local bindings
// - Ownership and borrowing tracking (simplified for v0.1)
// - Pattern matching exhaustiveness checking
// - Generic function/type instantiation
// - Trait/interface constraint resolution
// - Comprehensive error reporting with suggestions
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <variant>
#include <stack>
#include "sema/types.h"
#include "sema/symbol_table.h"
#include "parser/ast.h"
#include "lexer/token.h"

namespace novium {

// ── Type Checker Configuration ────────────────────────────────────────────────

struct TypeCheckConfig {
    bool check_ownership = true;
    bool check_borrowing = true;
    bool warn_unused = true;
    bool warn_unreachable = true;
    bool require_exhaustive_match = true;
    bool infer_generics = true;
    int max_recursion_depth = 1000;
};

// ── Inference Context ─────────────────────────────────────────────────────────
// Tracks state during type inference for a single expression/function

struct InferenceContext {
    int level = 0; // Generalization level (0 = global, >0 = local)
    std::vector<TypeVariable*> generalized_vars; // Variables to generalize at let-binding
    std::optional<TypePtr> expected_type; // Expected type from context (for checking mode)
    bool in_return_position = false;
    bool in_loop = false;
    bool in_async = false;
    std::string current_function;
    std::vector<std::string> generic_params_in_scope;
    
    InferenceContext() = default;
};

// ── Type Checker Main Class ───────────────────────────────────────────────────

class TypeChecker : public ASTVisitor {
public:
    TypeChecker(TypeInterner& interner, SymbolTable& symbols, const TypeCheckConfig& config = {});
    
    // Main entry point
    void check_program(const std::vector<std::unique_ptr<Stmt>>& program);
    
    // Error handling
    bool has_errors() const { return !errors_.empty(); }
    const std::vector<TypeError>& errors() const { return errors_; }
    std::vector<TypeError>& errors() { return errors_; }
    
    // Type queries
    TypePtr get_expr_type(const Expr* expr) const;
    void set_expr_type(const Expr* expr, TypePtr type);
    
    // Visitor methods (ASTVisitor interface)
    // Expressions
    void visit(IdentifierExpr* expr) override;
    void visit(LiteralExpr* expr) override;
    void visit(UnaryExpr* expr) override;
    void visit(BinaryExpr* expr) override;
    void visit(CallExpr* expr) override;
    void visit(MemberAccessExpr* expr) override;
    void visit(AwaitExpr* expr) override;
    void visit(IndexExpr* expr) override;
    
    // Statements
    void visit(BlockStmt* stmt) override;
    void visit(VarDeclStmt* stmt) override;
    void visit(ExpressionStmt* stmt) override;
    void visit(FunctionDeclStmt* stmt) override;
    void visit(ClassDeclStmt* stmt) override;
    void visit(InterfaceDeclStmt* stmt) override;
    void visit(IfStmt* stmt) override;
    void visit(WhileStmt* stmt) override;
    void visit(MatchStmt* stmt) override;
    void visit(ReturnStmt* stmt) override;
    void visit(TryCatchStmt* stmt) override;
    void visit(GoStmt* stmt) override;

private:
    TypeInterner& interner_;
    SymbolTable& symbols_;
    TypeCheckConfig config_;
    InferenceContext ctx_;
    std::vector<TypeError> errors_;
    
    // Type storage for expressions (key = pointer address)
    std::unordered_map<const Expr*, TypePtr> expr_types_;
    
    // Return type tracking for current function
    std::optional<TypePtr> current_return_type_;
    bool current_function_has_return_ = false;
    
    // Loop stack for break/continue
    std::vector<bool> loop_stack_;
    
    // ── Error Reporting ────────────────────────────────────────────────────────
    void error(TypeError::Kind kind, const std::string& message, const SourceLocation& loc,
               TypePtr expected = nullptr, TypePtr actual = nullptr);
    void error(const Token& token, TypeError::Kind kind, const std::string& message,
               TypePtr expected = nullptr, TypePtr actual = nullptr);
    void note(const std::string& message, const SourceLocation& loc);
    
    // ── Phase 1: Signature Collection ──────────────────────────────────────────
    void collect_function_signature(FunctionDeclStmt* fn);
    void collect_class_signature(ClassDeclStmt* cls);
    void collect_interface_signature(InterfaceDeclStmt* iface);
    
    // ── Type Inference Helpers ────────────────────────────────────────────────
    TypePtr fresh_type_var();
    TypePtr fresh_type_var(int level);
    
    // Unification
    bool unify(TypePtr a, TypePtr b, const SourceLocation& loc, const char* context = "");
    bool unify_with_expected(TypePtr actual, TypePtr expected, const SourceLocation& loc);
    
    // Type instantiation (for generics)
    TypePtr instantiate_type(const TypePtr& type, const std::vector<TypePtr>& type_args);
    TypePtr instantiate_function_type(const TypePtr& fn_type, const std::vector<TypePtr>& type_args);
    
    // Constraint solving
    bool check_constraints(const TypePtr& type, const std::vector<TypePtr>& constraints, const SourceLocation& loc);
    
    // ── Expression Type Checking ──────────────────────────────────────────────
    TypePtr check_expr(Expr* expr, std::optional<TypePtr> expected = std::nullopt);
    TypePtr synth_expr(Expr* expr); // Synthesize type (inference mode)
    
    // Binary operations
    TypePtr check_binary_op(const Token& op, TypePtr left, TypePtr right, const SourceLocation& loc);
    TypePtr check_comparison_op(const Token& op, TypePtr left, TypePtr right, const SourceLocation& loc);
    TypePtr check_assignment_op(const Token& op, TypePtr left, TypePtr right, const SourceLocation& loc);
    
    // Function calls
    TypePtr check_call(CallExpr* call, std::optional<TypePtr> expected);
    TypePtr resolve_call_target(Expr* callee, const std::vector<std::unique_ptr<Expr>>& args, const SourceLocation& loc);
    
    // Member access
    TypePtr check_member_access(MemberAccessExpr* expr, std::optional<TypePtr> expected);
    
    // Index access
    TypePtr check_index(IndexExpr* expr, std::optional<TypePtr> expected);
    
    // ── Statement Type Checking ───────────────────────────────────────────────
    void check_var_decl(VarDeclStmt* stmt);
    void check_function_decl(FunctionDeclStmt* stmt);
    void check_class_decl(ClassDeclStmt* stmt);
    void check_interface_decl(InterfaceDeclStmt* stmt);
    void check_if_stmt(IfStmt* stmt);
    void check_while_stmt(WhileStmt* stmt);
    void check_match_stmt(MatchStmt* stmt);
    void check_return_stmt(ReturnStmt* stmt);
    void check_try_catch_stmt(TryCatchStmt* stmt);
    void check_go_stmt(GoStmt* stmt);
    
    // Block checking
    void check_block(BlockStmt* block, std::optional<TypePtr> expected_return = std::nullopt);
    
    // Pattern matching
    TypePtr check_pattern(Expr* pattern, TypePtr subject_type, const SourceLocation& loc);
    bool is_irrefutable_pattern(Expr* pattern);
    bool check_match_exhaustiveness(MatchStmt* stmt, TypePtr subject_type);
    
    // ── Ownership/Borrowing ───────────────────────────────────────────────────
    void check_ownership_transfer(Expr* expr, TypePtr type, const SourceLocation& loc);
    void check_borrow(Expr* expr, TypePtr type, bool mutable_borrow, const SourceLocation& loc);
    bool is_move_operation(const Token& op) const;
    
    // ── Utility ───────────────────────────────────────────────────────────────
    TypePtr get_common_type(TypePtr a, TypePtr b);
    void check_statement(Stmt* stmt);
    TypePtr annotation_to_type(const TypeAnnotation& ann, const SourceLocation& loc);
    bool is_numeric_type(TypePtr t) const;
    bool is_comparable_type(TypePtr t) const;
    bool is_callable_type(TypePtr t) const;
    TypePtr get_element_type(TypePtr container) const;
    std::string format_type(TypePtr t) const;
    
    // Built-in function handling
    TypePtr check_builtin_call(const std::string& name, const std::vector<std::unique_ptr<Expr>>& args, 
                               const SourceLocation& loc, std::optional<TypePtr> expected);
    
    // Scope management helpers
    void enter_function_scope(FunctionDeclStmt* fn);
    void exit_function_scope();
};

// ── Type Checking Result ──────────────────────────────────────────────────────

struct TypeCheckResult {
    bool success;
    std::vector<TypeError> errors;
    std::vector<std::string> warnings;
    
    TypeCheckResult(bool s = true) : success(s) {}
};

} // namespace novium
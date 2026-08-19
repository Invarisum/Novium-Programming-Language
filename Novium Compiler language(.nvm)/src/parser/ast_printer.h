// ============================================================================
// ast_printer.h — Visitor-based AST visualizer for Novium
// ============================================================================

#pragma once

#include <iostream>
#include <string>
#include "parser/ast.h"

namespace novium {

class ASTPrinter : public ASTVisitor {
public:
    ASTPrinter(std::ostream& os = std::cout);

    void print(ASTNode* node);

    // Expression visits
    void visit(IdentifierExpr* expr) override;
    void visit(LiteralExpr* expr) override;
    void visit(UnaryExpr* expr) override;
    void visit(BinaryExpr* expr) override;
    void visit(CallExpr* expr) override;
    void visit(MemberAccessExpr* expr) override;
    void visit(AwaitExpr* expr) override;
    void visit(IndexExpr* expr) override;
    void visit(CastExpr* expr) override;

    // Statement visits
    void visit(BlockStmt* stmt) override;
    void visit(VarDeclStmt* stmt) override;
    void visit(ExpressionStmt* stmt) override;
    void visit(PrintStmt* stmt) override;
    void visit(PrintLnStmt* stmt) override;
    void visit(EmptyStmt* stmt) override;
    void visit(FunctionDeclStmt* stmt) override;
    void visit(ClassDeclStmt* stmt) override;
    void visit(InterfaceDeclStmt* stmt) override;
    void visit(IfStmt* stmt) override;
    void visit(WhileStmt* stmt) override;
    void visit(MatchStmt* stmt) override;
    void visit(ReturnStmt* stmt) override;
    void visit(TryCatchStmt* stmt) override;
    void visit(GoStmt* stmt) override;
    void visit(DeferStmt* stmt) override;
    void visit(UnsafeBlockStmt* stmt) override;
    void visit(PanicStmt* stmt) override;
    void visit(PythonFFIBlockStmt* stmt) override;
    void visit(JSXExprExpr* expr) override;
    void visit(JSXTagExpr* expr) override;
    void visit(CSSStylesStmt* stmt) override;
    void visit(HTMLTemplateStmt* stmt) override;
    void visit(PythonImportStmt* stmt) override;
    void visit(JSExportStmt* stmt) override;

private:
    std::ostream& os_;
    int indent_level_;

    void indent();
    void increment_indent();
    void decrement_indent();
};

} // namespace novium

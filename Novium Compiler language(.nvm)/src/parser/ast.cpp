// ============================================================================
// ast.cpp — AST Node Accept Method Implementations
// ============================================================================

#include "parser/ast.h"

namespace novium {

void IdentifierExpr::accept(ASTVisitor* visitor) { visitor->visit(this); }
void LiteralExpr::accept(ASTVisitor* visitor)    { visitor->visit(this); }
void UnaryExpr::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void BinaryExpr::accept(ASTVisitor* visitor)     { visitor->visit(this); }
void CallExpr::accept(ASTVisitor* visitor)       { visitor->visit(this); }
void MemberAccessExpr::accept(ASTVisitor* visitor) { visitor->visit(this); }
void AwaitExpr::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void IndexExpr::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void CastExpr::accept(ASTVisitor* visitor)       { visitor->visit(this); }

void BlockStmt::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void VarDeclStmt::accept(ASTVisitor* visitor)    { visitor->visit(this); }
void ExpressionStmt::accept(ASTVisitor* visitor) { visitor->visit(this); }
void PrintStmt::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void PrintLnStmt::accept(ASTVisitor* visitor)    { visitor->visit(this); }
void EmptyStmt::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void FunctionDeclStmt::accept(ASTVisitor* visitor) { visitor->visit(this); }
void ClassDeclStmt::accept(ASTVisitor* visitor)  { visitor->visit(this); }
void InterfaceDeclStmt::accept(ASTVisitor* visitor) { visitor->visit(this); }
void IfStmt::accept(ASTVisitor* visitor)         { visitor->visit(this); }
void WhileStmt::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void MatchStmt::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void ReturnStmt::accept(ASTVisitor* visitor)     { visitor->visit(this); }
void TryCatchStmt::accept(ASTVisitor* visitor)   { visitor->visit(this); }
void GoStmt::accept(ASTVisitor* visitor)         { visitor->visit(this); }
void DeferStmt::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void UnsafeBlockStmt::accept(ASTVisitor* visitor){ visitor->visit(this); }
void PanicStmt::accept(ASTVisitor* visitor)      { visitor->visit(this); }
void PythonFFIBlockStmt::accept(ASTVisitor* visitor) { visitor->visit(this); }

void JSXExprExpr::accept(ASTVisitor* visitor)    { visitor->visit(this); }
void JSXTagExpr::accept(ASTVisitor* visitor)     { visitor->visit(this); }
void CSSStylesStmt::accept(ASTVisitor* visitor)  { visitor->visit(this); }
void HTMLTemplateStmt::accept(ASTVisitor* visitor) { visitor->visit(this); }
void PythonImportStmt::accept(ASTVisitor* visitor) { visitor->visit(this); }
void JSExportStmt::accept(ASTVisitor* visitor)   { visitor->visit(this); }

} // namespace novium

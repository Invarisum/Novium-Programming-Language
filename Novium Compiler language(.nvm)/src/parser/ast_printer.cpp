// ============================================================================
// ast_printer.cpp — AST Printer visitor implementation
// ============================================================================

#include "parser/ast_printer.h"

namespace novium {

ASTPrinter::ASTPrinter(std::ostream& os) : os_(os), indent_level_(0) {}

void ASTPrinter::print(ASTNode* node) {
    if (node) {
        node->accept(this);
    } else {
        os_ << "<null ASTNode>\n";
    }
}

void ASTPrinter::indent() {
    os_ << std::string(indent_level_ * 2, ' ');
}

void ASTPrinter::increment_indent() {
    indent_level_++;
}

void ASTPrinter::decrement_indent() {
    if (indent_level_ > 0) indent_level_--;
}

// ── Expression Visits ────────────────────────────────────────────────────────

void ASTPrinter::visit(IdentifierExpr* expr) {
    os_ << "Identifier \"" << expr->name << "\"\n";
}

void ASTPrinter::visit(LiteralExpr* expr) {
    os_ << "Literal (" << token_type_to_string(expr->token.type) << ") \"" << expr->token.value << "\"\n";
}

void ASTPrinter::visit(UnaryExpr* expr) {
    os_ << "UnaryExpr (" << expr->op.value << ")\n";
    increment_indent();
    indent();
    print(expr->right.get());
    decrement_indent();
}

void ASTPrinter::visit(BinaryExpr* expr) {
    os_ << "BinaryExpr (" << expr->op.value << ")\n";
    increment_indent();
    
    indent();
    os_ << "Left: ";
    print(expr->left.get());
    
    indent();
    os_ << "Right: ";
    print(expr->right.get());
    
    decrement_indent();
}

void ASTPrinter::visit(CallExpr* expr) {
    os_ << "CallExpr\n";
    increment_indent();
    
    indent();
    os_ << "Callee: ";
    print(expr->callee.get());
    
    if (!expr->arguments.empty()) {
        indent();
        os_ << "Arguments:\n";
        increment_indent();
        for (const auto& arg : expr->arguments) {
            indent();
            print(arg.get());
        }
        decrement_indent();
    }
    
    decrement_indent();
}

void ASTPrinter::visit(MemberAccessExpr* expr) {
    os_ << "MemberAccessExpr (." << expr->member_name << ")\n";
    increment_indent();
    indent();
    os_ << "Object: ";
    print(expr->object.get());
    decrement_indent();
}

void ASTPrinter::visit(AwaitExpr* expr) {
    os_ << "AwaitExpr\n";
    increment_indent();
    indent();
    print(expr->value.get());
    decrement_indent();
}

void ASTPrinter::visit(IndexExpr* expr) {
    os_ << "IndexExpr\n";
    increment_indent();
    indent();
    os_ << "Object: ";
    print(expr->object.get());
    indent();
    os_ << "Index: ";
    print(expr->index.get());
    decrement_indent();
}

// ── Statement Visits ─────────────────────────────────────────────────────────

void ASTPrinter::visit(BlockStmt* stmt) {
    os_ << "Block\n";
    increment_indent();
    for (const auto& s : stmt->statements) {
        indent();
        print(s.get());
    }
    decrement_indent();
}

void ASTPrinter::visit(VarDeclStmt* stmt) {
    os_ << "VarDeclStmt \"" << stmt->name << "\" (mutable=" << (stmt->is_mutable ? "true" : "false") << ")\n";
    increment_indent();
    if (stmt->has_type_annotation) {
        indent();
        os_ << "Type: " << stmt->type.to_string() << "\n";
    }
    if (stmt->initializer) {
        indent();
        os_ << "Initializer: ";
        print(stmt->initializer.get());
    }
    decrement_indent();
}

void ASTPrinter::visit(ExpressionStmt* stmt) {
    os_ << "ExpressionStmt: ";
    print(stmt->expression.get());
}

void ASTPrinter::visit(FunctionDeclStmt* stmt) {
    os_ << "FunctionDeclStmt \"" << stmt->name << "\" (async=" << (stmt->is_async ? "true" : "false") << ")\n";
    increment_indent();
    
    if (!stmt->params.empty()) {
        indent();
        os_ << "Parameters:\n";
        increment_indent();
        for (const auto& param : stmt->params) {
            indent();
            os_ << param.name << " : " << param.type.to_string() << "\n";
        }
        decrement_indent();
    }
    
    if (stmt->has_return_type) {
        indent();
        os_ << "ReturnType: " << stmt->return_type.to_string() << "\n";
    }
    
    if (stmt->body) {
        indent();
        os_ << "Body: ";
        print(stmt->body.get());
    }
    
    decrement_indent();
}

void ASTPrinter::visit(ClassDeclStmt* stmt) {
    os_ << "ClassDeclStmt \"" << stmt->name << "\"\n";
    increment_indent();
    
    if (stmt->has_base_class) {
        indent();
        os_ << "Extends: " << stmt->base_class << "\n";
    }
    
    if (!stmt->interfaces.empty()) {
        indent();
        os_ << "Implements: ";
        for (size_t i = 0; i < stmt->interfaces.size(); i++) {
            os_ << stmt->interfaces[i] << (i == stmt->interfaces.size() - 1 ? "" : ", ");
        }
        os_ << "\n";
    }
    
    if (!stmt->fields.empty()) {
        indent();
        os_ << "Fields:\n";
        increment_indent();
        for (const auto& field : stmt->fields) {
            indent();
            os_ << field.name << " : " << field.type.to_string() << "\n";
        }
        decrement_indent();
    }
    
    if (!stmt->methods.empty()) {
        indent();
        os_ << "Methods:\n";
        increment_indent();
        for (const auto& method : stmt->methods) {
            indent();
            print(method.get());
        }
        decrement_indent();
    }
    
    decrement_indent();
}

void ASTPrinter::visit(InterfaceDeclStmt* stmt) {
    os_ << "InterfaceDeclStmt \"" << stmt->name << "\"\n";
    increment_indent();
    if (!stmt->methods.empty()) {
        indent();
        os_ << "Methods:\n";
        increment_indent();
        for (const auto& method : stmt->methods) {
            indent();
            print(method.get());
        }
        decrement_indent();
    }
    decrement_indent();
}

void ASTPrinter::visit(IfStmt* stmt) {
    os_ << "IfStmt\n";
    increment_indent();
    
    indent();
    os_ << "Condition: ";
    print(stmt->condition.get());
    
    indent();
    os_ << "Then Branch: ";
    print(stmt->then_branch.get());
    
    for (const auto& branch : stmt->elif_branches) {
        indent();
        os_ << "Elif Branch Condition: ";
        print(branch.condition.get());
        indent();
        os_ << "Elif Branch Body: ";
        print(branch.block.get());
    }
    
    if (stmt->else_branch) {
        indent();
        os_ << "Else Branch: ";
        print(stmt->else_branch.get());
    }
    
    decrement_indent();
}

void ASTPrinter::visit(WhileStmt* stmt) {
    os_ << "WhileStmt\n";
    increment_indent();
    indent(); os_ << "Condition: "; print(stmt->condition.get());
    indent(); os_ << "Body: "; print(stmt->body.get());
    decrement_indent();
}

void ASTPrinter::visit(MatchStmt* stmt) {
    os_ << "MatchStmt\n";
    increment_indent();
    indent();
    os_ << "Subject: ";
    print(stmt->subject.get());
    
    indent();
    os_ << "Arms:\n";
    increment_indent();
    for (const auto& arm : stmt->arms) {
        indent();
        os_ << "Pattern: ";
        print(arm.pattern.get());
        indent();
        os_ << "Body: ";
        print(arm.body.get());
    }
    decrement_indent();
    decrement_indent();
}

void ASTPrinter::visit(ReturnStmt* stmt) {
    os_ << "ReturnStmt\n";
    if (stmt->value) {
        increment_indent();
        indent();
        print(stmt->value.get());
        decrement_indent();
    }
}

void ASTPrinter::visit(TryCatchStmt* stmt) {
    os_ << "TryCatchStmt\n";
    increment_indent();
    
    indent();
    os_ << "Try Block: ";
    print(stmt->try_block.get());
    
    for (const auto& catch_b : stmt->catch_blocks) {
        indent();
        os_ << "Catch";
        if (catch_b.has_exception_type) os_ << " Type: " << catch_b.exception_type;
        if (catch_b.has_exception_var) os_ << " Var: " << catch_b.exception_var;
        os_ << ": ";
        print(catch_b.body.get());
    }
    
    if (stmt->finally_block) {
        indent();
        os_ << "Finally Block: ";
        print(stmt->finally_block.get());
    }
    
    decrement_indent();
}

void ASTPrinter::visit(GoStmt* stmt) {
    os_ << "GoStmt\n";
    increment_indent();
    indent();
    print(stmt->call.get());
    decrement_indent();
}

} // namespace novium

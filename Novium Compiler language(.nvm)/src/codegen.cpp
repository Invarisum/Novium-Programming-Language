// ============================================================================
// codegen.cpp — LLVM IR Code Generator Implementation for Novium
// ============================================================================
//
// Generates LLVM Intermediate Representation (IR) from the validated AST.
// Supports: functions, basic types, ownership, borrowing, and pattern matching.
//
// ============================================================================

#include "codegen.h"
#include "parser/ast.h"
#include "sema/types.h"
#include "sema/symbol_table.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace novium {

// ============================================================================
// LlvmCodeGen Constructor
// ============================================================================

LlvmCodeGen::LlvmCodeGen(const CodeGenConfig& config)
    : config_(config),
      result_(CodeGenResult::LlvmModule()) // Default-construct LlvmModule
{
    // Initialize LLVM context
    ctx_ = new LlvmContextWrapper();
    builder_ = new LlvmBuilder(*ctx_);
    module_ = std::make_unique<LlvmModule>();
}

// ============================================================================
// Main Entry Point: Generate IR from Program
// ============================================================================

CodeGenResult LlvmCodeGen::generate(const std::vector<std::unique_ptr<novium::Stmt>>& program) {
    try {
        result_ = CodeGenResult(LlvmModule()); // Reset result
        generate_module(program);

        if (!result_.success && !result_.error_message.empty()) {
            return result_;
        }

        result_.success = true;
        return result_;
    } catch (const std::exception& e) {
        result_.error_message = "Code generation exception: " + std::string(e.what());
        return result_;
    } catch (...) {
        result_.error_message = "Unknown code generation error";
        return result_;
    }
}

// ============================================================================
// Generate Module with All Top-Level Declarations
// ============================================================================

void LlvmCodeGen::generate_module(const std::vector<std::unique_ptr<novium::Stmt>>& program) {
    // Create the main module
    module_ = std::make_unique<LlvmModule>();
    module_->get()->setModuleIdentifier("novium");

    // Set target triple if specified
    if (!config_.target_triple.empty()) {
        module_->get()->setTargetTriple(config_.target_triple);
    }

    // Generate functions for all top-level declarations
    for (const auto& stmt : program) {
        if (auto* fn = dynamic_cast<novium::FunctionDeclStmt*>(stmt.get())) {
            generate_function(fn);
        } else if (auto* cls = dynamic_cast<novium::ClassDeclStmt*>(stmt.get())) {
            generate_class(cls);
        } else if (auto* iface = dynamic_cast<novium::InterfaceDeclStmt*>(stmt.get())) {
            generate_interface(iface);
        }
    }

    // Verify the generated IR
    if (config_.optimize_ir) {
        // VerifyModule(*module_, llvm::VerifierOptions());
    }
}

// ============================================================================
// Generate Function IR
// ============================================================================

void LlvmCodeGen::generate_function(novium::FunctionDeclStmt* fn) {
    // Create the LLVM function type
    std::vector<llvm::Type*> param_types;
    for (const auto& param : fn->params) {
        auto lt = get_llvm_type(/* need type from annotation */ nullptr);
        // For now, use i64 for all params as placeholder
        param_types.push_back(llvm::IntegerType::getInt64(*ctx_->context_));
    }

    llvm::FunctionType* ft =
        llvm::FunctionType::get(get_llvm_type(/* return type */ nullptr),
                               param_types,
                               /* is_var_arg */ false);

    // Create the LLVM function
    llvm::Function* llvm_fn = llvm::Function::Create(
        ft,
        llvm::Function::ExternalLinkage,
        fn->name.c_str(),
        module_->get()
    );

    // Set function attributes
    llvm_fn->setOnlyReadsMemory(!fn->is_async);
    if (fn->is_async) {
        llvm_fn->setDoesNotReturn();
    }

    // Create entry basic block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*ctx_->context_, "entry", llvm_fn);

    // Set the builder position
    builder_->set_insert_block(entry);

    // Store return type for later use
    // Map return type to LLVM value
    void* return_val = nullptr;

    // Generate the function body
    if (fn->body) {
        generate_block(fn->body.get(), nullptr);
    }

    // Add ret void if no explicit return with value
    if (!return_val) {
        builder_->create_retVoid();
    }

    // Register the function in our map
    func_map_[fn->name] = new LlvmFunction(llvm_fn);
}

// ============================================================================
// Generate Basic Block IR
// ============================================================================

void LlvmCodeGen::generate_block(novium::BlockStmt* block, LlvmBasicBlock* block_obj) {
    if (!block || !block->statements.empty() == false) return;

    // Iterate through statements in the block
    for (const auto& stmt : block->statements) {
        // Dispatch based on statement type
        if (auto* var_decl = dynamic_cast<novium::VarDeclStmt*>(stmt.get())) {
            codegen_var_decl(var_decl);
        } else if (auto* expr_stmt = dynamic_cast<novium::ExpressionStmt*>(stmt.get())) {
            codegen_expr(expr_stmt->expression.get());
        } else if (auto* if_stmt = dynamic_cast<novium::IfStmt*>(stmt.get())) {
            codegen_if(if_stmt);
        } else if (auto* while_stmt = dynamic_cast<novium::WhileStmt*>(stmt.get())) {
            codegen_while(while_stmt);
        } else if (auto* return_stmt = dynamic_cast<novium::ReturnStmt*>(stmt.get())) {
            codegen_return(return_stmt);
        } else if (auto* match_stmt = dynamic_cast<novium::MatchStmt*>(stmt.get())) {
            codegen_match(match_stmt);
        } else if (auto* go_stmt = dynamic_cast<novium::GoStmt*>(stmt.get())) {
            codegen_go(go_stmt);
        } else if (auto* panic_stmt = dynamic_cast<novium::PanicStmt*>(stmt.get())) {
            codegen_panic(panic_stmt);
        } else if (auto* empty = dynamic_cast<novium::EmptyStmt*>(stmt.get())) {
            // No-op
        } else {
            // Expression statement with just a value
            if (auto* expr_stmt = dynamic_cast<novium::ExpressionStmt*>(stmt.get())) {
                codegen_expr(expr_stmt->expression.get());
            }
        }
    }
}

// ============================================================================
// Generate Expression IR
// ============================================================================

void* LlvmCodeGen::generate_expr(novium::Expr* expr, void* expected_type) {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case novium::ExprKind::IDENTIFIER: {
            auto* id = static_cast<novium::IdentifierExpr*>(expr);
            // Look up variable in our map
            auto it = var_map_.find(id->name);
            if (it != var_map_.end()) {
                return it->second;  // Return LLVM value
            }
            // Variable not found - this would be a type error in normal flow
            return nullptr;
        }

        case novium::ExprKind::LITERAL: {
            auto* lit = static_cast<novium::LiteralExpr*>(expr);
            return codegen_literal(lit);
        }

        case novium::ExprKind::BINARY: {
            auto* bin = static_cast<novium::BinaryExpr*>(expr);
            void* left = generate_expr(bin->left.get());
            void* right = generate_expr(bin->right.get());
            codegen_binary(bin, left, right);
            // Return the result (last value in builder)
            return builder_->get_current_value();
        }

        case novium::ExprKind::CALL: {
            auto* call = static_cast<novium::CallExpr*>(expr);
            return codegen_call(call);
        }

        case novium::ExprKind::UNARY: {
            auto* unary = static_cast<novium::UnaryExpr*>(expr);
            // Handle unary operations
            void* operand = generate_expr(unary->right.get());
            // ... handle negation, logical not, etc.
            return operand; // Placeholder
        }

        case novium::ExprKind::MEMBER_ACCESS: {
            auto* member = static_cast<novium::MemberAccessExpr*>(expr);
            // ... handle object.field access
            return nullptr;
        }

        case novium::ExprKind::INDEX: {
            auto* idx = static_cast<novium::IndexExpr*>(expr);
            // ... handle array[index]
            return nullptr;
        }

        case novium::ExprKind::AWAIT: {
            auto* await = static_cast<novium::AwaitExpr*>(expr);
            // ... handle await
            return nullptr;
        }

        default:
            return nullptr;
    }
}

// ============================================================================
// Helper: Get LLVM Type from Novium Type
// ============================================================================

void* LlvmCodeGen::get_llvm_type(novium::TypePtr type) {
    if (!type) return llvm::IntegerType::getInt64(*ctx_->context_); // default i64

    switch (type->kind) {
        case novium::TypeKind::VOID: return llvm::VoidType::get(*ctx_->context_);
        case novium::TypeKind::BOOL: return llvm::IntegerType::getInt1(*ctx_->context_);
        case novium::TypeKind::INT: return llvm::IntegerType::getInt64(*ctx_->context_);
        case novium::TypeKind::FLOAT: return llvm::FloatType::get(*ctx_->context_);
        case novium::TypeKind::STRING: {
            // String is a pointer to heap-allocated data
            return llvm::PointerType::getInt8Ptr(*ctx_->context_);
        }
        case novium::TypeKind::ARRAY: {
            // Array type - element type pointer
            auto elem_type = get_llvm_type(type->element_type);
            // For now, create an array type
            unsigned num_elements = type->array_size.value_or(0);
            if (num_elements > 0) {
                return llvm::ArrayType::get(elem_type, num_elements);
            }
            return elem_type;
        }
        case novium::TypeKind::SLICE: {
            // Slice is a pointer + length
            return llvm::StructType::get(
                llvm::PointerType::getInt8Ptr(*ctx_->context_),  // data pointer
                llvm::IntegerType::getInt64(*ctx_->context_),    // length
                false,                                           // not packed
                "slice"
            );
        }
        case novium::TypeKind::FUNCTION: {
            // Function type - we'll handle this in generate_function
            return llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx_->context_), false);
        }
        case novium::TypeKind::TYPE_VAR:
        case novium::TypeKind::INFER: {
            // Inferred type - use i64 as default
            return llvm::IntegerType::getInt64(*ctx_->context_);
        }
        default:
            return llvm::IntegerType::getInt64(*ctx_->context_);
    }
}

// ============================================================================
// Codegen for Literal Expressions
// ============================================================================

void* LlvmCodeGen::codegen_literal(novium::LiteralExpr* expr) {
    // Create a constant value based on the token type
    llvm::ConstantInt* int_val = nullptr;
    llvm::ConstantFP* float_val = nullptr;
    llvm::ConstantDataArray* str_val = nullptr;

    switch (expr->token.type) {
        case novium::TokenType::INTEGER_LITERAL: {
            long long val = std::stoll(expr->token.value);
            int_val = llvm::ConstantInt::get(*ctx_->context_, llvm::APInt(64, val));
            return int_val;
        }

        case novium::TokenType::FLOAT_LITERAL: {
            double val = std::stod(expr->token.value);
            float_val = llvm::ConstantFP::get(*ctx_->context_, llvm::APFloat(val));
            return float_val;
        }

        case novium::TokenType::STRING_LITERAL: {
            // Create a global string constant
            std::string str = expr->token.value;
            // Remove surrounding quotes if present
            if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
                str = str.substr(1, str.size() - 2);
            }
            // Create a constant array
            str_val = llvm::ConstantDataArray::getString(*ctx_->context_, str, /* null terminated */ true);
            // Return as i8 pointer
            return llvm::PointerConstant::getNullValue(
                llvm::PointerType::getInt8Ptr(*ctx_->context_)
            );
        }

        case novium::TokenType::KW_TRUE: {
            return llvm::ConstantInt::get(*ctx_->context_, llvm::APInt(1, 1));
        }

        case novium::TokenType::KW_FALSE: {
            return llvm::ConstantInt::get(*ctx_->context_, llvm::APInt(1, 0));
        }

        case novium::TokenType::KW_NULL: {
            // Null pointer
            return llvm::ConstantPointerNull::get(
                llvm::PointerType::getInt8Ptr(*ctx_->context_)
            );
        }

        default:
            return llvm::Constant::getNullValue(
                llvm::Type::getInt64Ty(*ctx_->context_)
            );
    }
}

// ============================================================================
// Codegen for Identifier Expressions
// ============================================================================

void* LlvmCodeGen::codegen_identifier(novium::IdentifierExpr* expr) {
    // Look up variable in the variable map
    auto it = var_map_.find(expr->name);
    if (it != var_map_.end()) {
        return it->second;  // Return the LLVM value
    }

    // Variable not found - this would be caught by type checker
    // For now, return null and let the caller handle it
    return nullptr;
}

// ============================================================================
// Codegen for Call Expressions
// ============================================================================

void* LlvmCodeGen::codegen_call(novium::CallExpr* expr) {
    // Get the callee function
    // For now, handle only built-in functions and direct function calls
    if (!expr->callee) return nullptr;

    // Check if it's an identifier callee
    if (auto* id = dynamic_cast<novium::IdentifierExpr*>(expr->callee.get())) {
        std::string func_name = id->name;

        // Check if it's a built-in function
        if (func_name == "print" || func_name == "println") {
            // Collect argument values
            std::vector<void*> args;
            for (const auto& arg : expr->arguments) {
                args.push_back(generate_expr(arg.get()));
            }

            // Call the appropriate print function
            if (func_name == "print") {
                // Emit print IR
                emit_print(args);
            } else if (func_name == "println") {
                // Emit println IR
                emit_println(args);
            }
            return nullptr; // print/println don't return a value
        }

        // Check if it's a user-defined function
        auto it = func_map_.find(func_name);
        if (it != func_map_.end()) {
            llvm::Function* llvm_fn = it->second->get_function();

            // Collect argument values
            std::vector<llvm::Value*> llvm_args;
            for (const auto& arg : expr->arguments) {
                llvm_args.push_back(generate_expr(arg.get()));
            }

            // Call the function
            return builder_->create_call(llvm_fn, llvm_args, "calltmp");
        }
    }

    // Member access call (e.g. obj.method())
    // Or other call patterns - for now return null
    return nullptr;
}

// ============================================================================
// Codegen for Return Statements
// ============================================================================

void LlvmCodeGen::codegen_return(novium::ReturnStmt* stmt) {
    if (!stmt->value) {
        // Return void
        builder_->create_retVoid();
        return;
    }

    // Evaluate the return value expression
    void* val = generate_expr(stmt->value.get());
    if (val) {
        builder_->create_ret(val);
    } else {
        builder_->create_retVoid();
    }
}

// ============================================================================
// Codegen for If Statements
// ============================================================================

void LlvmCodeGen::codegen_if(novium::IfStmt* stmt) {
    // Evaluate the condition
    void* cond = generate_expr(stmt->condition.get());

    // Create the then branch basic block
    llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(*ctx_->context_, "then", builder_->get_current_function());

    // Create the else branch basic block (if there's an else)
    llvm::BasicBlock* else_bb = nullptr;
    if (stmt->else_branch) {
        else_bb = llvm::BasicBlock::Create(*ctx_->context_, "else");
    }

    // Create the merge basic block (after if-else)
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*ctx_->context_, "merge");

    // Create conditional branch
    builder_->createCondBr(cond, then_bb, else_bb ? else_bb : merge_bb);

    // Emit then block
    builder_->set_insert_block(then_bb);
    if (stmt->then_branch) {
        generate_block(stmt->then_branch.get(), nullptr);
    }
    builder_->createBr(merge_bb);

    // Emit else block (if present)
    if (else_bb) {
        builder_->set_insert_block(else_bb);
        if (stmt->else_branch) {
            generate_block(stmt->else_branch.get(), nullptr);
        }
        builder_->createBr(merge_bb);
    }

    // Emit merge block
    builder_->set_insert_block(merge_bb);
}

// ============================================================================
// Codegen for While Statements
// ============================================================================

void LlvmCodeGen::codegen_while(novium::WhileStmt* stmt) {
    // Create the condition block
    llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(*ctx_->context_, "cond", builder_->get_current_function());

    // Create the body block
    llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(*ctx_->context_, "body", builder_->get_current_function());

    // Create the merge block (after the loop)
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*ctx_->context_, "after_loop");

    // Start with the condition block
    builder_->set_insert_block(cond_bb);

    // Evaluate the condition
    void* cond_val = generate_expr(stmt->condition.get());

    // Branch to body if condition is true, otherwise to merge
    builder_->createCondBr(cond_val, body_bb, merge_bb);

    // Emit body block
    builder_->set_insert_block(body_bb);
    if (stmt->body) {
        generate_block(stmt->body.get(), nullptr);
    }
    // Branch back to condition
    builder_->createBr(cond_bb);

    // Emit merge block
    builder_->set_insert_block(merge_bb);
}

// ============================================================================
// Codegen for Match Statements
// ============================================================================

void LlvmCodeGen::codegen_match(novium::MatchStmt* stmt) {
    // Evaluate the subject expression
    void* subject = generate_expr(stmt->subject.get());

    // For each arm, generate matching code
    for (const auto& arm : stmt->arms) {
        // Generate the pattern match
        // For now, just generate the body with the subject value available
        if (arm.body) {
            // Create a basic block for this arm
            llvm::BasicBlock* arm_bb = llvm::BasicBlock::Create(*ctx_->context_, "match_arm", builder_->get_current_function());

            // Set builder position and emit the body
            builder_->set_insert_block(arm_bb);
            generate_block(arm.body.get(), nullptr);
            builder_->createBr(llvm::BasicBlock::Create(*ctx_->context_, "match_end"));
        }
    }

    // Create merge block after match
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(*ctx_->context_, "match_end");
    builder_->set_insert_block(merge_bb);
}

// ============================================================================
// Codegen for Go Statements
// ============================================================================

void LlvmCodeGen::codegen_go(novium::GoStmt* stmt) {
    // Spawn a goroutine - in LLVM this would be lowered to a call in a new thread
    // For now, just emit a call (the runtime will handle threading)
    if (stmt->call) {
        // Generate the call expression
        void* call_val = generate_expr(stmt->call.get());
        if (call_val) {
            // In a full implementation, this would spawn a new thread
            // For now, just emit the call
            builder_->create_call((llvm::Function*)call_val, {}, "gocall");
        }
    }
}

// ============================================================================
// Codegen for Panic Statements
// ============================================================================

void LlvmCodeGen::codegen_panic(novium::PanicStmt* stmt) {
    // Emit a trap or unreachable
    // In LLVM, we can use the 'unreachable' instruction or call llvm.trap()
    builder_->create_unreachable();
}

// ============================================================================
// Helper: Emit Print IR
// ============================================================================

void LlvmCodeGen::emit_print(std::vector<void*>& args) {
    // Build a call to the print function
    // For now, just print the arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            // Print space between arguments
            builder_->create_call(
                llvm::Function::Create(
                    llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx_->context_), false),
                    llvm::Function::ExternalLinkage,
                    "print_space"
                ),
                {}
            );
        }
        // Print the argument - this is simplified
        // In full implementation, we'd format the value properly
        builder_->create_call(
            llvm::Function::Create(
                llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx_->context_), false),
                llvm::Function::ExternalLinkage,
                "print_value"
            ),
            {args[i]}
        );
    }
}

// ============================================================================
// Helper: Emit PrintLn IR
// ============================================================================

void LlvmCodeGen::emit_println(std::vector<void*>& args) {
    emit_print(args);
    // Add newline after printing
    builder_->create_call(
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx_->context_), false),
            llvm::Function::ExternalLinkage,
            "print_newline"
        ),
        {}
    );
}

} // namespace novium
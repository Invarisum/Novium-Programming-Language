// ============================================================================
// codegen.h — LLVM IR Code Generator for Novium
// ============================================================================
//
// Generates LLVM Intermediate Representation (IR) from the validated AST.
// Supports: functions, basic types, ownership, borrowing, and pattern matching.
//
// ============================================================================

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <unordered_map>
#include "parser/ast.h"
#include "sema/types.h"

namespace novium {

// Forward declaration
class LLVMContextWrapper;

// ============================================================================
// CodeGen Config
// ============================================================================

struct CodeGenConfig {
    enum class OptimizationLevel { NONE, BASIC, AGGRESSIVE };
    OptimizationLevel optimization_level = OptimizationLevel::BASIC;
    bool emit_debug_info = true;
    std::string target_triple = "";
    std::string cpu = "generic";
    bool enable_fp_contract = false;
};

// ============================================================================
// CodeGen Result
// ============================================================================

struct CodeGenResult {
    // Owned LLVM Module (IR code). Caller takes ownership.
    // Module owns all functions, basic blocks, and instructions.
    class LlvmModule {
    public:
        ~LlvmModule() = default;
        // Delete copy/move to prevent double-free
        LlvmModule(const LlvmModule&) = delete;
        LlvmModule& operator=(const LlvmModule&) = delete;
        LlvmModule(LlvmModule&&) = default;
        LlvmModule& operator=(LlvmModule&&) = default;

        // Access the underlying LLVM Module
        void* get() const { return module_; }
        // Get the module name
        std::string name() const { return "novium-module"; }

    private:
        // Raw pointer to the llvm::Module (owned by LLVM's context)
        void* module_ = nullptr;
        // Friend codegen class for creation
        friend class LlvmCodeGen;

    public:
        LlvmModule() = default;
    };

    LlvmModule module;
    std::string error_message;
    bool success = false;

    // Check if code generation succeeded
    explicit operator bool() const { return success; }

    CodeGenResult() : success(false) {}
    CodeGenResult(LlvmModule m, std::string err, bool succ)
        : module(std::move(m)), error_message(std::move(err)), success(succ) {}
};

// ============================================================================
// LLVM Code Generator Class
// ============================================================================

class LlvmCodeGen {
public:
    LlvmCodeGen(const CodeGenConfig& config = CodeGenConfig());

    // Main entry point: generate IR from a parsed and type-checked program
    CodeGenResult generate(const std::vector<std::unique_ptr<novium::Stmt>>& program);

    // Generate C ABI header file from type information
    // Writes the header to the given output path
    bool generate_abi_header(const std::string& output_path);

    // Get the generated module
    const CodeGenResult::LlvmModule& get_module() const { return result_.module; }

    // Get any error that occurred
    const std::string& get_error() const { return result_.error_message; }

private:
    CodeGenConfig config_;
    CodeGenResult result_;

    // Generate the entire module including all top-level declarations
    void generate_module(const std::vector<std::unique_ptr<novium::Stmt>>& program);

    // Generate function IR
    void generate_function(novium::FunctionDeclStmt* fn);

    // Generate basic block IR
    void generate_block(novium::BlockStmt* block, class LlvmBasicBlock* block_obj);

    // Generate expression IR
    // Returns the LLVM Value representing the expression result
    void* generate_expr(novium::Expr* expr, void* expected_type = nullptr);

    // Helper: get or create LLVM type from Novium type
    void* get_llvm_type(novium::TypePtr type);

    // Helper: create entry block for function
    void* create_entry_block(novium::FunctionDeclStmt* fn, void* func);

    // Codegen for variable declarations
    void codegen_var_decl(novium::VarDeclStmt* stmt);

    // Codegen for binary operations
    void codegen_binary(novium::BinaryExpr* expr, void* left, void* right);

    // Codegen for literal expressions
    void* codegen_literal(novium::LiteralExpr* expr);

    // Codegen for identifier expressions
    void* codegen_identifier(novium::IdentifierExpr* expr);

    // Codegen for call expressions
    void* codegen_call(novium::CallExpr* expr);

    // Codegen for return statements
    void codegen_return(novium::ReturnStmt* stmt);

    // Codegen for if statements
    void codegen_if(novium::IfStmt* stmt);

    // Codegen for while statements
    void codegen_while(novium::WhileStmt* stmt);

    // Codegen for match statements
    void codegen_match(novium::MatchStmt* stmt);

    // Codegen for variable assignment
    void codegen_assignment(novium::BinaryExpr* expr);

    // Print IR builder helpers
    void emit_print(void* value);
    void emit_println(void* value);

    // LLVM context and builder management (opaque handles; LLVM types live
    // only inside codegen.cpp when built with -DNOVIUM_WITH_LLVM=ON)
    class LlvmContextWrapper* ctx_;
    // IR builder (positioned at end of basic block)
    class LlvmBuilder* builder_;
    // Module being built (opaque llvm::Module*)
    void* module_ = nullptr;
    // Map from Novium variable names to LLVM values
    std::unordered_map<std::string, void*> var_map_;
    // Map from function names to LLVM functions
    std::unordered_map<std::string, void*> func_map_;
};

// ============================================================================
// Inline Utility Functions
// ============================================================================

// Convert Novium TypeKind to LLVM TypeKind (rough mapping)
inline unsigned llvm_type_kind(novium::TypeKind kind) {
    switch (kind) {
        case novium::TypeKind::VOID: return 0;
        case novium::TypeKind::BOOL: return 1;
        case novium::TypeKind::INT8: return 2;
        case novium::TypeKind::INT16: return 3;
        case novium::TypeKind::INT32: return 4;
        case novium::TypeKind::INT: return 5;
        case novium::TypeKind::UINT8: return 6;
        case novium::TypeKind::UINT16: return 7;
        case novium::TypeKind::UINT32: return 8;
        case novium::TypeKind::UINT: return 9;
        case novium::TypeKind::FLOAT16: return 10;
        case novium::TypeKind::FLOAT: return 11;
        case novium::TypeKind::STRING: return 12;
        case novium::TypeKind::NEVER: return 13;
        default: return 0;
    }
}

// Get LLVM type size in bits for a given type kind
inline unsigned llvm_type_size_bits(novium::TypeKind kind) {
    switch (kind) {
        case novium::TypeKind::VOID: return 0;
        case novium::TypeKind::BOOL: return 1;
        case novium::TypeKind::INT8: return 8;
        case novium::TypeKind::INT16: return 16;
        case novium::TypeKind::INT32: return 32;
        case novium::TypeKind::INT: return 64;  // i64
        case novium::TypeKind::UINT8: return 8;
        case novium::TypeKind::UINT16: return 16;
        case novium::TypeKind::UINT32: return 32;
        case novium::TypeKind::UINT: return 64;  // u64
        case novium::TypeKind::FLOAT16: return 16;
        case novium::TypeKind::FLOAT: return 64;  // f64
        case novium::TypeKind::STRING: return 0;  // pointer-sized
        case novium::TypeKind::NEVER: return 0;
        default: return 32;
    }
}

} // namespace novium
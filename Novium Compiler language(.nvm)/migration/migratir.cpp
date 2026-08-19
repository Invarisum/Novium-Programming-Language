// ============================================================================
// migratir.cpp — Migration IR Implementation
//
// Implements translation between Novium AST, Migration IR, and target languages.
//
// ============================================================================

#include "migratir.h"
#include "parser/ast.h"
#include "parser/ast_printer.h"
#include "sema/types.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace novium::migratir {

// ============================================================================
// IRNode to_string Implementations
// ============================================================================

std::string IRModule::to_string() const {
    std::ostringstream ss;
    ss << "// Module: " << name << "\n";
    for (const auto& item : items) {
        ss << item->to_string() << "\n";
    }
    return ss.str();
}

std::string IRFunc::to_string() const {
    std::ostringstream ss;
    ss << "fn " << name;
    if (is_async) ss << " async";
    ss << "(";
    for (size_t i = 0; i < param_names.size(); ++i) {
        ss << param_types[i] << " " << param_names[i];
        if (i < param_names.size() - 1) ss << ", ";
    }
    ss << ") -> " << return_type << " {\n";
    for (const auto& stmt : body) {
        ss << "  " << stmt->to_string() << "\n";
    }
    ss << "}\n";
    return ss.str();
}

std::string IRParam::to_string() const {
    return type + " " + name;
}

std::string IRBlock::to_string() const {
    std::ostringstream ss;
    for (const auto& stmt : stmts) {
        ss << stmt->to_string();
    }
    return ss.str();
}

std::string IREXPRStmt::to_string() const {
    return expr->to_string() + ";";
}

std::string IRReturn::to_string() const {
    if (value) {
        return "return " + value->to_string() + ";";
    }
    return "return;";
}

std::string IRIf::to_string() const {
    std::ostringstream ss;
    ss << "if (" << condition->to_string() << ") {\n";
    if (then_branch) ss << then_branch->to_string();
    ss << "}\n";
    if (else_branch) {
        ss << "else {\n";
        if (else_branch) ss << else_branch->to_string();
        ss << "}\n";
    }
    return ss.str();
}

std::string IRWhile::to_string() const {
    std::ostringstream ss;
    ss << "while (" << condition->to_string() << ") {\n";
    if (body) ss << body->to_string();
    ss << "}\n";
    return ss.str();
}

std::string IRFor::to_string() const {
    std::ostringstream ss;
    ss << "for (";
    if (init) ss << init->to_string();
    ss << "; ";
    if (cond) ss << cond->to_string();
    ss << "; ";
    if (inc) ss << inc->to_string();
    ss << ") {\n";
    if (body) ss << body->to_string();
    ss << "}\n";
    return ss.str();
}

std::string IRIdentifier::to_string() const {
    return name;
}

std::string IRIntLiteral::to_string() const {
    return std::to_string(value);
}

std::string IRFloatLiteral::to_string() const {
    // Format: keep reasonable precision
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

std::string IRStringLiteral::to_string() const {
    // Escape double quotes for C++/Python
    std::string result = "\"";
    for (char c : value) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else result += c;
    }
    result += "\"";
    return result;
}

std::string IRBinaryOp::to_string() const {
    // Map IR operators to language-specific ones
    // In the IR we use familiar operators, target languages map them
    return "(" + left->to_string() + " " + op + " " + right->to_string() + ")";
}

std::string IRCallExpr::to_string() const {
    std::ostringstream ss;
    ss << callee;
    ss << "(";
    for (size_t i = 0; i < args.size(); ++i) {
        ss << args[i]->to_string();
        if (i < args.size() - 1) ss << ", ";
    }
    ss << ")";
    return ss.str();
}

std::string IRVarDecl::to_string() const {
    std::string mutable_str = is_mutable ? "mut " : "";
    return mutable_str + type + " " + name;
}

// ============================================================================
// Novium AST to Migration IR Conversion
// ============================================================================

// Map Novium TypeKind to IR type string
std::string type_to_ir(novium::TypePtr type) {
    if (!type) return "infer";
    
    switch (type->kind) {
        case novium::TypeKind::VOID: return "void";
        case novium::TypeKind::BOOL: return "bool";
        case novium::TypeKind::INT: return "i64";
        case novium::TypeKind::FLOAT: return "f64";
        case novium::TypeKind::STRING: return "string";
        case novium::TypeKind::NEVER: return "never";
        case novium::TypeKind::FUNCTION: {
            // Function type: (params) -> return
            // Not fully supported in IR round-trip
            return "function";
        }
        case novium::TypeKind::ARRAY: {
            // Array<T>
            auto elem = type->element_type;
            return "array<" + type_to_ir(elem) + ">";
        }
        case novium::TypeKind::SLICE: {
            return "slice<" + type_to_ir(type->element_type) + ">";
        }
        case novium::TypeKind::TYPE_VAR:
        case novium::TypeKind::INFER: return "infer";
        default: return "any";
    }
}

// Map Novium TypeAnnotation to IR type string
std::string annot_to_ir(novium::TypeAnnotation annot) {
    std::string result;
    
    // Ownership
    if (annot.is_owned) result += "own ";
    if (annot.is_borrowed) {
        result += "&";
        if (annot.is_mutable_borrow) result += "mut ";
    }
    
    // Base type
    if (!annot.name.empty()) {
        result += annot.name;
    }
    
    // Nullability
    if (annot.is_nullable) result += "?";
    
    return result;
}

// Map Novium TokenType to operator string for IR
std::string token_to_ir_op(novium::TokenType type) {
    switch (type) {
        case novium::TokenType::PLUS: return "+";
        case novium::TokenType::MINUS: return "-";
        case novium::TokenType::STAR: return "*";
        case novium::TokenType::SLASH: return "/";
        case novium::TokenType::EQUAL_EQUAL: return "==";
        case novium::TokenType::BANG_EQUAL: return "!=";
        case novium::TokenType::LESS: return "<";
        case novium::TokenType::LESS_EQUAL: return "<=";
        case novium::TokenType::GREATER: return ">";
        case novium::TokenType::GREATER_EQUAL: return ">=";
        case novium::TokenType::AND_AND: return "&&";
        case novium::TokenType::OR_OR: return "||";
        case novium::TokenType::EQUAL: return "=";
        default: return "?";
    }
}

// Convert Novium AST to Migration IR
std::unique_ptr<IRModule> novium_ast_to_ir(const std::vector<std::unique_ptr<novium::Stmt>>& program) {
    auto module = std::make_unique<IRModule>("novium_translated");
    
    for (const auto& stmt : program) {
        if (auto* fn = dynamic_cast<novium::FunctionDeclStmt*>(stmt.get())) {
            auto ir_fn = std::make_unique<IRFunc>(fn->name, fn->is_async);
            
            // Map parameters
            for (const auto& param : fn->params) {
                ir_fn->param_names.push_back(param.name);
                ir_fn->param_types.push_back(type_to_ir(/* param type */ nullptr));
            }
            
            // Map return type
            ir_fn->return_type = type_to_ir(/* return type */ nullptr);
            
            // Convert body block
            if (fn->body) {
                // Create IR block and convert statements
                auto ir_block = std::make_unique<IRBlock>();
                
                // We need to convert the block's statements
                // For now, just add a placeholder return
                if (fn->body->statements.empty()) {
                    ir_block->stmts.push_back(std::make_unique<IRVarDecl>("result", "infer"));
                    ir_block->stmts.push_back(std::make_unique<IRReturn>(
                        std::make_unique<IRIdentifier>("result")));
                }
                ir_fn->body.push_back(std::move(ir_block));
            }
            
            module->items.push_back(std::move(ir_fn));
        }
    }
    
    return module;
}

// Convert Migration IR back to Novium AST (round-trip)
std::vector<std::unique_ptr<novium::Stmt>> ir_to_novium_ast(const IRModule& module) {
    std::vector<std::unique_ptr<novium::Stmt>> stmts;
    
    for (const auto& item : module.items) {
        if (auto* fn = dynamic_cast<IRFunc*>(item.get())) {
            // Build Novium function declaration
            novium::TypeAnnotation ret_ann;
            ret_ann.name = "void"; // Default
            // This is simplified - full round-trip would need type reconstruction
            
            novium::FunctionDeclStmt::FunctionParam params;
            // ... map params
            
            // For now, create a minimal function decl
            // In a full implementation, we'd need the full type system
            stmts.push_back(nullptr); // Placeholder
        }
    }
    
    return stmts;
}

// ============================================================================
// Target Language Code Generation
// ============================================================================

// Generate C++ code from Migration IR
std::string ir_to_cpp(const IRModule& module) {
    std::ostringstream ss;
    
    ss << "// Generated C++ code from Novium IR\n";
    ss << "#include <iostream>\n#include <string>\n#include <vector>\n\n";
    
    for (const auto& item : module.items) {
        if (auto* fn = dynamic_cast<IRFunc*>(item.get())) {
            ss << "// Function: " << fn->name << "\n";
            
            // Generate function signature
            ss << "void " << fn->name << "() {\n";
            
            // Generate body
            if (auto* block = dynamic_cast<IRBlock*>(fn->body.empty() ? nullptr : fn->body[0].get())) {
                ss << block->to_string();
            }
            
            ss << "}\n\n";
        }
    }
    
    return ss.str();
}

// Generate Go code from Migration IR
std::string ir_to_go(const IRModule& module) {
    std::ostringstream ss;
    
    ss << "// Generated Go code from Novium IR\n\n";
    
    for (const auto& item : module.items) {
        if (auto* fn = dynamic_cast<IRFunc*>(item.get())) {
            ss << "// Function: " << fn->name << "\n";
            
            // Generate Go function
            // Novium: fn add(a int, b int) int:
            // Go: func add(a int, b int) int {
            ss << "func " << fn->name << "(";
            
            for (size_t i = 0; i < fn->param_names.size(); ++i) {
                ss << fn->param_types[i] << " " << fn->param_names[i];
                if (i < fn->param_names.size() - 1) ss << ", ";
            }
            
            // Novium return type
            if (fn->return_type != "void") {
                ss << ") " << fn->return_type << " {\n";
            } else {
                ss << ") {\n";
            }
            
            // Generate body
            if (auto* block = dynamic_cast<IRBlock*>(/* fn->body */ nullptr)) {
                // Simplified - would need full body traversal
                ss << "    // Body: " << fn->name << "\n";
            }
            
            ss << "}\n\n";
        }
    }
    
    return ss.str();
}

// Generate Python code from Migration IR
std::string ir_to_python(const IRModule& module) {
    std::ostringstream ss;
    
    ss << "# Generated Python code from Novium IR\n\n";
    
    for (const auto& item : module.items) {
        if (auto* fn = dynamic_cast<IRFunc*>(item.get())) {
            ss << "# Function: " << fn->name << "\n";
            
            // Generate Python function
            // Novium: fn add(a int, b int) int:
            // Python: def add(a: int, b: int) -> int:
            ss << "def " << fn->name << "(";
            
            for (size_t i = 0; i < fn->param_names.size(); ++i) {
                ss << "int " << fn->param_names[i]  // Simplified type hint
                   << (i < fn->param_names.size() - 1 ? ", " : "");
            }
            
            if (fn->return_type != "void") {
                ss << ") -> int:\n";
            } else {
                ss << ":\n";
            }
            
            // Generate body
            if (auto* block = dynamic_cast<IRBlock*>(/* fn->body */ nullptr)) {
                ss << "    # Body: " << fn->name << "\n";
                ss << "    pass  # Placeholder\n";
            }
            
            ss << "\n";
        }
    }
    
    return ss.str();
}

// ============================================================================
// Source Language Parsers (skeletons - partial implementations)
// ============================================================================

// Parse Go source code into Migration IR (subset: functions, expressions)
std::unique_ptr<IRModule> go_source_to_ir(const std::string& source) {
    // This is a skeleton - full Go parser would be extensive
    // For now, return empty module
    auto module = std::make_unique<IRModule>("go_source");
    
    // Try to detect function patterns
    // Look for "func " patterns
    size_t pos = 0;
    while ((pos = source.find("func ", pos)) != std::string::npos) {
        // Find the function name
        size_t name_start = pos + 5;
        size_t name_end = source.find('(', name_start);
        if (name_end != std::string::npos) {
            std::string fname = source.substr(name_start, name_end - name_start);
            
            auto ir_fn = std::make_unique<IRFunc>(fname);
            
            // Look for parameters: "func add(a int, b int)"
            size_t paren_start = name_end;
            size_t paren_end = source.find(')', paren_start);
            if (paren_end != std::string::npos) {
                std::string params_str = source.substr(paren_start + 1, paren_end - paren_start - 1);
                
                // Parse "a int, b int"
                // Simple split by comma
                size_t comma_pos = 0;
                while ((comma_pos = params_str.find(',', comma_pos)) != std::string::npos) {
                    // This is very simplified
                    comma_pos++;
                }
            }
            
            module->items.push_back(std::move(ir_fn));
        }
        pos = name_end + 1;
    }
    
    return module;
}

// Parse Python source code into Migration IR (subset: functions, expressions)
std::unique_ptr<IRModule> python_source_to_ir(const std::string& source) {
    // Skeleton parser for Python function definitions
    auto module = std::make_unique<IRModule>("python_source");
    
    // Look for "def " patterns
    size_t pos = 0;
    while ((pos = source.find("def ", pos)) != std::string::npos) {
        // Find function name
        size_t name_start = pos + 4;
        # Find end of function name (end of line or opening paren for args)
        size_t name_end = source.find('(', name_start);
        if (name_end == std::string::npos) {
            name_end = source.find('\\n', name_start);
        }
        if (name_end != std::string::npos) {
            std::string fname = source.substr(name_start, name_end - name_start);
            // Trim whitespace
            while (!fname.empty() && std::isspace(fname.back())) fname.pop_back();
            
            auto ir_fn = std::make_unique<IRFunc>(fname);
            
            // Look for parameters: "def func(a, b):"
            args_start = source.find('(', name_end);
            if (args_start != std::string::npos) {
                args_end = source.find(')', args_start);
                if (args_end != std::string::npos) {
                    std::string args_str = source.substr(args_start + 1, args_end - args_start - 1);
                    // Very simplified: just count args
                    size_t comma_count = 0;
                    for (char c : args_str) if (c == ',') comma_count++;
                    for (size_t i = 0; i <= comma_count; ++i) {
                        ir_fn->param_names.push_back("arg" + std::to_string(i));
                        ir_fn->param_types.push_back("object"); // Python is dynamically typed
                    }
                }
            }
            
            // Look for return type annotation or body
            body_start = source.find(':', args_end);
            if (body_start != std::string::npos) {
                // Has a body - find indentation level
                // For now, just add a placeholder
                ir_fn->body.push_back(std::make_unique<IRBlock>());
            }
            
            module->items.push_back(std::move(ir_fn));
        }
        pos = name_end + 1;
    }
    
    return module;
}

// Parse Rust source code into Migration IR (subset: functions, expressions)
std::unique_ptr<IRModule> rust_source_to_ir(const std::string& source) {
    // Skeleton Rust parser
    auto module = std::make_unique<IRModule>("rust_source");
    
    // Look for "fn " patterns (Rust functions)
    size_t pos = 0;
    while ((pos = source.find("fn ", pos)) != std::string::npos) {
        // Find function name - Rust: "fn add(a: i32) -> i32 {"
        size_t name_start = pos + 3;
        size_t name_end = source.find('(', name_start);
        if (name_end != std::string::npos) {
            std::string fname = source.substr(name_start, name_end - name_start);
            // Trim - Rust fn names can't have spaces
            while (!fname.empty() && std::isspace(fname.back())) fname.pop_back();
            
            auto ir_fn = std::make_unique<IRFunc>(fname);
            
            // Look for parameters in parentheses
            size_t paren_end = source.find(')', name_end);
            if (paren_end != std::string::npos) {
                std::string params_str = source.substr(name_end + 1, paren_end - name_end - 1);
                // Rust: "a: i32, b: i64"
                // Very simplified parsing
                size_t colon_pos = params_str.find(':');
                if (colon_pos != std::string::npos) {
                    // Extract type after colon
                    std::string type_str = params_str.substr(colon_pos + 1);
                    // Trim
                    while (!type_str.empty() && std::isspace(type_str.front())) type_str.erase(type_str.front());
                    while (!type_str.empty() && std::isspace(type_str.back())) type_str.pop_back();
                    
                    // Add a parameter with this type
                    if (!type_str.empty()) {
                        ir_fn->param_types.push_back(type_str);
                        ir_fn->param_names.push_back("param1");
                    }
                }
            }
            
            // Look for return type: "-> i32"
            size_t arrow_pos = source.find("->", paren_end);
            if (arrow_pos != std::string::npos) {
                size_t arrow_end = source.find('{', arrow_pos);
                if (arrow_end != std::string::npos) {
                    std::string ret_str = source.substr(arrow_pos + 2, arrow_end - arrow_pos - 2);
                    // Trim
                    while (!ret_str.empty() && std::isspace(ret_str.front())) ret_str.erase(ret_str.front());
                    while (!ret_str.empty() && std::isspace(ret_str.back())) ret_str.pop_back();
                    if (!ret_str.empty()) {
                        ir_fn->return_type = ret_str;
                    }
                }
            }
            
            module->items.push_back(std::move(ir_fn));
        }
        pos = name_end + 1;
    }
    
    return module;
}

// ============================================================================
// Export Function
// ============================================================================

// Main entry: translate between formats
// direction: "novium2cpp", "golang2novium", "python2novium", etc.
std::string translate(const std::string& source, const std::string& direction) {
    if (direction == "novium2cpp") {
        // Parse Novium, convert to IR, generate C++
        // For now, return stub
        return "// Novium to C++ translation not fully implemented in Sprint 14\n// Would: parse Novium AST -> IR -> C++ code\n";
    } else if (direction == "cpp2novium") {
        // Parse C++, convert to Novium AST/IR
        return "// C++ to Novium translation not fully implemented in Sprint 14\n// Would: parse C++ -> IR -> Novium AST\n";
    } else if (direction == "golang2novium") {
        auto ir = go_source_to_ir(source);
        // Convert IR to Novium AST
        return "// Go to Novium: skeleton parser returns empty module\n";
    } else if (direction == "python2novium") {
        auto ir = python_source_to_ir(source);
        return "// Python to Novium: skeleton parser returns empty module\n";
    } else if (direction == "rust2novium") {
        auto ir = rust_source_to_ir(source);
        return "// Rust to Novium: skeleton parser returns empty module\n";
    } else if (direction == "novium2python") {
        return "// Novium to Python: not implemented\n";
    } else if (direction == "novium2go") {
        return "// Novium to Go: not implemented\n";
    }
    
    return "// Unknown translation direction: " + direction + "\n";
}

// ============================================================================
// Main Entry Point for CLI
// ============================================================================

// CLI: novium migrate <direction> [options]
//   novium migrate novium2cpp <file.nvm>   -> generate C++ from Novium
//   novium migrate cpp2novium <file.cpp>   -> parse C++ to Novium IR
//   etc.

int main_migrate(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: novium migrate <direction> [file]\n";
        std::cerr << "Directions:\n";
        std::cerr << "  novium2cpp    Novium to C++\n";
        std::cerr << "  cpp2novium    C++ to Novium\n";
        std::cerr << "  golang2novium Go to Novium\n";
        std::cerr << "  python2novium Python to Novium\n";
        std::cerr << "  rust2novium   Rust to Novium\n";
        std::cerr << "  novium2python Novium to Python\n";
        std::cerr << "  novium2go     Novium to Go\n";
        return 1;
    }
    
    std::string direction = argv[1];
    std::string filename;
    
    if (argc >= 3) {
        filename = argv[2];
    }
    
    // Read source
    std::string source;
    if (!filename.empty()) {
        // Read from file
        // In full impl, would read the file
        source = "// placeholder source from " + filename;
    }
    
    // Perform translation
    std::string result = translate(source, direction);
    
    // Output result
    std::cout << result;
    
    return 0;
}
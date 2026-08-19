// ============================================================================
// symbol_table.h — Symbol Table and Scope Management for Novium
// ============================================================================
//
// Manages variable bindings, function declarations, type definitions,
// and generic parameters across nested scopes. Supports:
// - Lexical scoping with shadowing
// - Separate namespaces for values, types, and macros
// - Generic parameter tracking
// - Module-level exports
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <variant>
#include "sema/types.h"
#include "parser/ast.h"

namespace novium {

// ── Symbol Kinds ──────────────────────────────────────────────────────────────
enum class SymbolKind {
    VARIABLE,        // let/var binding
    FUNCTION,        // fn declaration
    METHOD,          // method on a type
    TYPE_DEF,        // class/interface/struct/enum
    TYPE_PARAMETER,  // Generic type parameter (T, U, etc.)
    TYPE_CLASS,      // Trait/interface constraint
    MODULE,          // Imported module
    CONSTANT,        // Compile-time constant
    MACRO,           // Macro definition
    LABEL            // Loop label for break/continue
};

// ── Symbol ────────────────────────────────────────────────────────────────────
struct Symbol {
    SymbolKind kind;
    std::string name;
    TypePtr type; // For variables, functions, type params
    TypeDefinition* type_def = nullptr; // For type definitions
    TypeClass* type_class = nullptr; // For type classes
    bool is_mutable = false;
    bool is_initialized = false;
    bool is_used = false;
    bool is_public = false;
    bool is_generic = false;
    SourceLocation location;
    std::vector<TypePtr> generic_params; // For generic functions/types
    int scope_depth = 0;
    
    // For functions
    bool is_async = false;
    bool is_extern = false;
    std::string extern_name; // For C ABI
    
    // For variables
    bool is_captured = false; // Captured by closure
    
    Symbol(SymbolKind k, std::string n, TypePtr t, SourceLocation loc, int depth)
        : kind(k), name(std::move(n)), type(std::move(t)), location(loc), scope_depth(depth) {}

    Symbol(SymbolKind k, std::string n, std::nullptr_t, SourceLocation loc, int depth)
        : kind(k), name(std::move(n)), type(nullptr), location(loc), scope_depth(depth) {}
    
    Symbol(SymbolKind k, std::string n, TypeDefinition* td, SourceLocation loc, int depth)
        : kind(k), name(std::move(n)), type_def(td), location(loc), scope_depth(depth) {}
    
    Symbol(SymbolKind k, std::string n, TypeClass* tc, SourceLocation loc, int depth)
        : kind(k), name(std::move(n)), type_class(tc), location(loc), scope_depth(depth) {}
};

// ── Scope ─────────────────────────────────────────────────────────────────────
class Scope {
public:
    Scope* parent = nullptr;
    int depth = 0;
    bool is_function_scope = false;
    bool is_loop_scope = false;
    bool is_type_scope = false; // For class/struct/enum bodies
    
    // Separate namespaces
    std::unordered_map<std::string, std::unique_ptr<Symbol>> values;     // Variables, functions, constants
    std::unordered_map<std::string, std::unique_ptr<Symbol>> types;      // Type definitions
    std::unordered_map<std::string, std::unique_ptr<Symbol>> macros;     // Macros
    std::unordered_map<std::string, std::unique_ptr<Symbol>> labels;     // Loop labels
    
    // Track order for diagnostics
    std::vector<std::string> declaration_order;
    
    Scope(Scope* p = nullptr, bool fn_scope = false)
        : parent(p), depth(p ? p->depth + 1 : 0), is_function_scope(fn_scope) {}
    
    // Lookup in this scope only
    Symbol* lookup_local(const std::string& name, SymbolKind kind);
    
    // Lookup in this scope and parents
    Symbol* lookup(const std::string& name, SymbolKind kind);
    
    // Declare in this scope
    bool declare(std::unique_ptr<Symbol> sym);
    
    // Get all symbols of a kind in this scope
    std::vector<Symbol*> get_all(SymbolKind kind);
    
    // Mark symbol as used
    void mark_used(const std::string& name, SymbolKind kind);
};

// ── Symbol Table ──────────────────────────────────────────────────────────────
class SymbolTable {
public:
    SymbolTable();
    
    // Scope management
    void enter_scope(bool is_function = false);
    void enter_loop_scope();
    void enter_type_scope();
    void exit_scope();
    
    Scope* current_scope() { return current_scope_; }
    Scope* global_scope() { return scopes_.front().get(); }
    
    // Declaration
    bool declare_variable(std::string name, TypePtr type, bool mutable_, SourceLocation loc, bool is_public = false);
    bool declare_function(std::string name, TypePtr type, SourceLocation loc, bool is_async = false, bool is_public = false, bool is_extern = false, std::string extern_name = "");
    bool declare_type(std::string name, TypeDefinition* def, SourceLocation loc, bool is_public = false);
    bool declare_type_parameter(std::string name, TypePtr type, SourceLocation loc);
    bool declare_type_class(std::string name, TypeClass* tc, SourceLocation loc, bool is_public = false);
    bool declare_constant(std::string name, TypePtr type, SourceLocation loc, bool is_public = false);
    bool declare_label(std::string name, SourceLocation loc);
    
    // Lookup
    Symbol* lookup_variable(const std::string& name);
    Symbol* lookup_function(const std::string& name);
    Symbol* lookup_type(const std::string& name);
    Symbol* lookup_type_parameter(const std::string& name);
    Symbol* lookup_type_class(const std::string& name);
    Symbol* lookup_constant(const std::string& name);
    Symbol* lookup_label(const std::string& name);
    Symbol* lookup_macro(const std::string& name);
    
    // Lookup in current scope only
    Symbol* lookup_variable_local(const std::string& name);
    Symbol* lookup_function_local(const std::string& name);
    Symbol* lookup_type_local(const std::string& name);
    
    // Mutation
    bool mark_initialized(const std::string& name);
    bool mark_used(const std::string& name);
    bool mark_captured(const std::string& name);
    
    // Query
    bool is_in_function_scope() const;
    bool is_in_loop_scope() const;
    bool is_in_type_scope() const;
    int current_depth() const { return current_scope_->depth; }
    
    // Generic parameter tracking
    void push_generic_params(const std::vector<TypeParameter>& params);
    void pop_generic_params();
    const std::vector<TypeParameter>& current_generic_params() const;
    
    // Diagnostics
    void check_unused() const;
    std::vector<std::string> get_unused_warnings() const;

private:
    std::vector<std::unique_ptr<Scope>> scopes_; // Flat stack; root is scopes_[0]
    Scope* current_scope_ = nullptr;
    std::vector<std::vector<TypeParameter>> generic_param_stack_;
};

// ── Built-in Types and Functions Registration ─────────────────────────────────
void register_builtins(SymbolTable& table, TypeInterner& interner);

} // namespace novium
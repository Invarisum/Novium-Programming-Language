// ============================================================================
// symbol_table.cpp — Symbol Table Implementation
// ============================================================================

#include "sema/symbol_table.h"
#include <iostream>

namespace novium {

// ── Scope Implementation ──────────────────────────────────────────────────────

Symbol* Scope::lookup_local(const std::string& name, SymbolKind kind) {
    switch (kind) {
        case SymbolKind::VARIABLE:
        case SymbolKind::FUNCTION:
        case SymbolKind::METHOD:
        case SymbolKind::CONSTANT:
        case SymbolKind::MODULE:
            if (auto it = values.find(name); it != values.end()) return it->second.get();
            break;
        case SymbolKind::TYPE_DEF:
        case SymbolKind::TYPE_PARAMETER:
        case SymbolKind::TYPE_CLASS:
            if (auto it = types.find(name); it != types.end()) return it->second.get();
            break;
        case SymbolKind::MACRO:
            if (auto it = macros.find(name); it != macros.end()) return it->second.get();
            break;
        case SymbolKind::LABEL:
            if (auto it = labels.find(name); it != labels.end()) return it->second.get();
            break;
    }
    return nullptr;
}

Symbol* Scope::lookup(const std::string& name, SymbolKind kind) {
    Symbol* found = lookup_local(name, kind);
    if (found) return found;
    if (parent) return parent->lookup(name, kind);
    return nullptr;
}

bool Scope::declare(std::unique_ptr<Symbol> sym) {
    const std::string& name = sym->name;
    auto* slot = (sym->kind == SymbolKind::MACRO) ? &macros
               : (sym->kind == SymbolKind::LABEL) ? &labels
               : (sym->kind == SymbolKind::TYPE_DEF ||
                  sym->kind == SymbolKind::TYPE_PARAMETER ||
                  sym->kind == SymbolKind::TYPE_CLASS) ? &types
               : &values;
    if (slot->find(name) != slot->end()) return false;
    (*slot)[name] = std::move(sym);
    declaration_order.push_back(name);
    return true;
}

std::vector<Symbol*> Scope::get_all(SymbolKind kind) {
    std::vector<Symbol*> result;
    auto collect = [&](const std::unordered_map<std::string, std::unique_ptr<Symbol>>& map, SymbolKind target) {
        for (auto& [_, sym] : map) {
            if (sym->kind == target) result.push_back(sym.get());
        }
    };
    collect(values, kind);
    collect(types, kind);
    collect(macros, kind);
    collect(labels, kind);
    return result;
}

void Scope::mark_used(const std::string& name, SymbolKind kind) {
    Symbol* sym = lookup_local(name, kind);
    if (sym) sym->is_used = true;
}

// ── SymbolTable Implementation ────────────────────────────────────────────────
// Scopes are owned by a flat stack of unique_ptrs; parent links are raw
// pointers into earlier stack entries. This guarantees no double-frees and
// O(1) enter/exit.

SymbolTable::SymbolTable() {
    auto root = std::make_unique<Scope>(nullptr, false);
    current_scope_ = root.get();
    scopes_.push_back(std::move(root));
}

void SymbolTable::enter_scope(bool is_function) {
    auto scope = std::make_unique<Scope>(current_scope_, is_function);
    current_scope_ = scope.get();
    scopes_.push_back(std::move(scope));
}

void SymbolTable::enter_loop_scope() {
    enter_scope(false);
    current_scope_->is_loop_scope = true;
}

void SymbolTable::enter_type_scope() {
    enter_scope(false);
    current_scope_->is_type_scope = true;
}

void SymbolTable::exit_scope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
        current_scope_ = scopes_.back().get();
    }
}

bool SymbolTable::declare_variable(std::string name, TypePtr type, bool mutable_, SourceLocation loc, bool is_public) {
    auto sym = std::make_unique<Symbol>(SymbolKind::VARIABLE, name, type, loc, current_depth());
    sym->is_mutable = mutable_;
    sym->is_public = is_public;
    return current_scope_->declare(std::move(sym));
}

bool SymbolTable::declare_function(std::string name, TypePtr type, SourceLocation loc, bool is_async, bool is_public, bool is_extern, std::string extern_name) {
    auto sym = std::make_unique<Symbol>(SymbolKind::FUNCTION, name, type, loc, current_depth());
    sym->is_async = is_async;
    sym->is_public = is_public;
    sym->is_extern = is_extern;
    sym->extern_name = std::move(extern_name);
    return current_scope_->declare(std::move(sym));
}

bool SymbolTable::declare_type(std::string name, TypeDefinition* def, SourceLocation loc, bool is_public) {
    auto sym = std::make_unique<Symbol>(SymbolKind::TYPE_DEF, name, def, loc, current_depth());
    sym->is_public = is_public;
    return current_scope_->declare(std::move(sym));
}

bool SymbolTable::declare_type_parameter(std::string name, TypePtr type, SourceLocation loc) {
    auto sym = std::make_unique<Symbol>(SymbolKind::TYPE_PARAMETER, name, type, loc, current_depth());
    return current_scope_->declare(std::move(sym));
}

bool SymbolTable::declare_type_class(std::string name, TypeClass* tc, SourceLocation loc, bool is_public) {
    auto sym = std::make_unique<Symbol>(SymbolKind::TYPE_CLASS, name, tc, loc, current_depth());
    sym->is_public = is_public;
    return current_scope_->declare(std::move(sym));
}

bool SymbolTable::declare_constant(std::string name, TypePtr type, SourceLocation loc, bool is_public) {
    auto sym = std::make_unique<Symbol>(SymbolKind::CONSTANT, name, type, loc, current_depth());
    sym->is_public = is_public;
    return current_scope_->declare(std::move(sym));
}

bool SymbolTable::declare_label(std::string name, SourceLocation loc) {
    auto sym = std::make_unique<Symbol>(SymbolKind::LABEL, name, nullptr, loc, current_depth());
    return current_scope_->declare(std::move(sym));
}

Symbol* SymbolTable::lookup_variable(const std::string& name) {
    return current_scope_->lookup(name, SymbolKind::VARIABLE);
}

Symbol* SymbolTable::lookup_function(const std::string& name) {
    return current_scope_->lookup(name, SymbolKind::FUNCTION);
}

Symbol* SymbolTable::lookup_type(const std::string& name) {
    return current_scope_->lookup(name, SymbolKind::TYPE_DEF);
}

Symbol* SymbolTable::lookup_type_parameter(const std::string& name) {
    return current_scope_->lookup(name, SymbolKind::TYPE_PARAMETER);
}

Symbol* SymbolTable::lookup_type_class(const std::string& name) {
    return current_scope_->lookup(name, SymbolKind::TYPE_CLASS);
}

Symbol* SymbolTable::lookup_constant(const std::string& name) {
    return current_scope_->lookup(name, SymbolKind::CONSTANT);
}

Symbol* SymbolTable::lookup_label(const std::string& name) {
    // Labels only visible in current function scope
    Scope* scope = current_scope_;
    while (scope) {
        if (auto* sym = scope->lookup_local(name, SymbolKind::LABEL)) return sym;
        if (scope->is_function_scope) break;
        scope = scope->parent;
    }
    return nullptr;
}

Symbol* SymbolTable::lookup_macro(const std::string& name) {
    return current_scope_->lookup(name, SymbolKind::MACRO);
}

Symbol* SymbolTable::lookup_variable_local(const std::string& name) {
    return current_scope_->lookup_local(name, SymbolKind::VARIABLE);
}

Symbol* SymbolTable::lookup_function_local(const std::string& name) {
    return current_scope_->lookup_local(name, SymbolKind::FUNCTION);
}

Symbol* SymbolTable::lookup_type_local(const std::string& name) {
    return current_scope_->lookup_local(name, SymbolKind::TYPE_DEF);
}

bool SymbolTable::mark_initialized(const std::string& name) {
    Symbol* sym = lookup_variable(name);
    if (sym) {
        sym->is_initialized = true;
        return true;
    }
    return false;
}

bool SymbolTable::mark_used(const std::string& name) {
    Symbol* sym = lookup_variable(name);
    if (sym) { sym->is_used = true; return true; }
    sym = lookup_function(name);
    if (sym) { sym->is_used = true; return true; }
    sym = lookup_constant(name);
    if (sym) { sym->is_used = true; return true; }
    return false;
}

bool SymbolTable::mark_captured(const std::string& name) {
    Symbol* sym = lookup_variable(name);
    if (sym) {
        sym->is_captured = true;
        return true;
    }
    return false;
}

bool SymbolTable::is_in_function_scope() const {
    Scope* scope = current_scope_;
    while (scope) {
        if (scope->is_function_scope) return true;
        scope = scope->parent;
    }
    return false;
}

bool SymbolTable::is_in_loop_scope() const {
    Scope* scope = current_scope_;
    while (scope) {
        if (scope->is_loop_scope) return true;
        scope = scope->parent;
    }
    return false;
}

bool SymbolTable::is_in_type_scope() const {
    Scope* scope = current_scope_;
    while (scope) {
        if (scope->is_type_scope) return true;
        scope = scope->parent;
    }
    return false;
}

void SymbolTable::push_generic_params(const std::vector<TypeParameter>& params) {
    generic_param_stack_.push_back(params);
}

void SymbolTable::pop_generic_params() {
    if (!generic_param_stack_.empty()) {
        generic_param_stack_.pop_back();
    }
}

const std::vector<TypeParameter>& SymbolTable::current_generic_params() const {
    static const std::vector<TypeParameter> empty;
    if (generic_param_stack_.empty()) return empty;
    return generic_param_stack_.back();
}

void SymbolTable::check_unused() const {
    // Reserved for future warning emission
}

std::vector<std::string> SymbolTable::get_unused_warnings() const {
    std::vector<std::string> warnings;
    auto check_scope = [&](const Scope* scope) {
        for (auto& [name, sym] : scope->values) {
            if (!sym->is_used && name.rfind("_", 0) != 0 &&
                sym->kind == SymbolKind::VARIABLE) {
                warnings.push_back("Unused variable: " + name + " at " + sym->location.filename + ":" +
                                   std::to_string(sym->location.line));
            }
        }
    };
    for (const auto& scope : scopes_) {
        check_scope(scope.get());
    }
    return warnings;
}

// ── Built-in Registration ─────────────────────────────────────────────────────

void register_builtins(SymbolTable& table, TypeInterner& interner) {
    // print : (any) -> void
    auto print_type = Type::make_function({interner.get_infer()}, interner.get_void());
    table.declare_function("print", print_type, {"builtin", 1, 1}, false, true);
    table.declare_function("println", print_type, {"builtin", 1, 1}, false, true);

    // panic : (string) -> Never
    auto panic_type = Type::make_function({interner.get_string()}, interner.get_never());
    table.declare_function("panic", panic_type, {"builtin", 1, 1}, false, true);

    // assert : (bool, string?) -> void
    auto assert_type = Type::make_function({interner.get_bool(), interner.get_string()->with_nullable()}, interner.get_void());
    table.declare_function("assert", assert_type, {"builtin", 1, 1}, false, true);

    // Type conversion functions
    table.declare_function("int_to_float", Type::make_function({interner.get_int()}, interner.get_float()), {"builtin", 1, 1}, false, true);
    table.declare_function("float_to_int", Type::make_function({interner.get_float()}, interner.get_int()), {"builtin", 1, 1}, false, true);
    table.declare_function("to_string", Type::make_function({interner.get_infer()}, interner.get_string()), {"builtin", 1, 1}, false, true);
}

} // namespace novium
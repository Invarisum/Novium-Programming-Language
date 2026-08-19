// ============================================================================
// types.cpp � Type System Implementation
// ============================================================================

#include "sema/types.h"
#include <sstream>
#include <unordered_set>

namespace novium {

// -- Type::to_string -----------------------------------------------------------

std::string Type::to_string() const {
    std::unordered_set<const Type*> visited;
    return to_string_impl(visited);
}

std::string Type::to_string_impl(std::unordered_set<const Type*>& visited) const {
    if (visited.find(this) != visited.end()) return "...";
    visited.insert(this);

    std::string prefix;
    switch (ownership) {
        case Ownership::OWN: prefix = "own "; break;
        case Ownership::BORROW: prefix = "&"; break;
        case Ownership::BORROW_MUT: prefix = "&mut "; break;
        case Ownership::NONE: break;
    }

    std::string base;
    switch (kind) {
        case TypeKind::VOID: base = "void"; break;
        case TypeKind::BOOL: base = "bool"; break;
        case TypeKind::INT: base = "int"; break;
        case TypeKind::FLOAT: base = "float"; break;
        case TypeKind::STRING: base = "string"; break;
        case TypeKind::CHAR: base = "char"; break;
        case TypeKind::NEVER: base = "never"; break;
        case TypeKind::ERROR: base = "<error>"; break;
        case TypeKind::INFER: base = "_"; break;
        case TypeKind::TYPE_VAR: {
            if (type_var) {
                if (type_var->is_bound()) {
                    base = type_var->get_binding()->to_string();
                } else {
                    base = "?T" + std::to_string(type_var->id);
                }
            } else {
                base = "?";
            }
            break;
        }
        case TypeKind::FUNCTION: {
            std::stringstream ss;
            ss << (is_async ? "async " : "") << "fn(";
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << param_types[i]->to_string();
            }
            ss << ") -> " << (return_type ? return_type->to_string() : "void");
            base = ss.str();
            break;
        }
        case TypeKind::TUPLE: {
            std::stringstream ss;
            ss << "(";
            for (size_t i = 0; i < element_types.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << element_types[i]->to_string();
            }
            ss << ")";
            base = ss.str();
            break;
        }
        case TypeKind::ARRAY: {
            if (array_size.has_value()) {
                base = "[" + element_type->to_string() + "; " + std::to_string(*array_size) + "]";
            } else {
                base = "[]" + element_type->to_string();
            }
            break;
        }
        case TypeKind::SLICE: {
            base = "[]" + element_type->to_string();
            break;
        }
        case TypeKind::RAW_PTR: {
            std::stringstream ss;
            ss << "*" << (is_mutable_ptr ? "mut " : "const ") << (type_args.empty() ? "void" : type_args[0]->to_string());
            base = ss.str();
            break;
        }
        case TypeKind::CLASS:
        case TypeKind::INTERFACE:
        case TypeKind::STRUCT:
        case TypeKind::ENUM: {
            if (definition) {
                base = definition->name;
                if (!type_args.empty()) {
                    base += "<";
                    for (size_t i = 0; i < type_args.size(); ++i) {
                        if (i > 0) base += ", ";
                        base += type_args[i]->to_string();
                    }
                    base += ">";
                }
            } else {
                base = "<anonymous>";
            }
            break;
        }
    }

    visited.erase(this);
    if (nullable) return prefix + base + "?";
    return prefix + base;
}

// -- Type::equals --------------------------------------------------------------

bool Type::equals(const TypePtr& other) const {
    const Type* o = other.get();
    if (this == o) return true;
    if (!o) return false;

    // Follow type variable bindings
    if (kind == TypeKind::TYPE_VAR && type_var && type_var->is_bound()) {
        return type_var->get_binding()->equals(other);
    }
    if (o->kind == TypeKind::TYPE_VAR && o->type_var && o->type_var->is_bound()) {
        return equals(o->type_var->get_binding());
    }
    if (kind == TypeKind::TYPE_VAR || o->kind == TypeKind::TYPE_VAR) {
        return kind == o->kind;
    }

    if (kind != o->kind) return false;
    if (ownership != o->ownership) return false;
    if (nullable != o->nullable) return false;

    switch (kind) {
        case TypeKind::VOID:
        case TypeKind::BOOL:
        case TypeKind::INT:
        case TypeKind::FLOAT:
        case TypeKind::STRING:
        case TypeKind::CHAR:
        case TypeKind::NEVER:
        case TypeKind::ERROR:
        case TypeKind::INFER:
            return true;
        case TypeKind::TYPE_VAR:
            return type_var == o->type_var;
        case TypeKind::FUNCTION:
            if (param_types.size() != o->param_types.size()) return false;
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (!param_types[i]->equals(o->param_types[i])) return false;
            }
            return return_type->equals(o->return_type) && is_async == o->is_async;
        case TypeKind::TUPLE:
            if (element_types.size() != o->element_types.size()) return false;
            for (size_t i = 0; i < element_types.size(); ++i) {
                if (!element_types[i]->equals(o->element_types[i])) return false;
            }
            return true;
        case TypeKind::ARRAY:
        case TypeKind::SLICE:
            return element_type->equals(o->element_type) && array_size == o->array_size;
        case TypeKind::RAW_PTR:
            return is_mutable_ptr == o->is_mutable_ptr &&
                   (type_args.size() == o->type_args.size() &&
                    (type_args.empty() || type_args[0]->equals(o->type_args[0])));
        case TypeKind::CLASS:
        case TypeKind::INTERFACE:
        case TypeKind::STRUCT:
        case TypeKind::ENUM:
            if (definition != o->definition) return false;
            if (type_args.size() != o->type_args.size()) return false;
            for (size_t i = 0; i < type_args.size(); ++i) {
                if (!type_args[i]->equals(o->type_args[i])) return false;
            }
            return true;
    }
    return false;
}

// -- Type::is_subtype_of -------------------------------------------------------

bool Type::is_subtype_of(const TypePtr& other, TypeInterner& interner) const {
    const Type* o = other.get();

    // Dereference type variables
    if (kind == TypeKind::TYPE_VAR && type_var && type_var->is_bound()) {
        return type_var->get_binding()->is_subtype_of(other, interner);
    }
    if (o->kind == TypeKind::TYPE_VAR && o->type_var && o->type_var->is_bound()) {
        return shared_from_this()->is_subtype_of(o->type_var->get_binding(), interner);
    }

    // Never is a subtype of everything
    if (kind == TypeKind::NEVER) return true;
    if (o->kind == TypeKind::NEVER) return false;

    // Error type is a subtype of everything (recovery)
    if (kind == TypeKind::ERROR || o->kind == TypeKind::ERROR) return true;

    // Same type is subtype of itself
    if (equals(other)) return true;

    // int is a subtype of float (numeric promotion)
    if (kind == TypeKind::INT && o->kind == TypeKind::FLOAT) return true;

    // Nullable: T is subtype of T?
    if (!nullable && other->nullable) {
        auto non_null_other = other->with_nullable(false);
        return equals(non_null_other);
    }

    // Class inheritance: S <: T if S extends T
    if (kind == TypeKind::CLASS && o->kind == TypeKind::CLASS && definition && o->definition) {
        // Check direct base
        if (definition->base_class.has_value()) {
            TypePtr base = definition->base_class.value();
            if (base->equals(other)) return true;
            // Recursive check up the hierarchy
            if (base->definition && base->is_subtype_of(other, interner)) return true;
        }
        // Check interfaces
        for (const auto& iface : definition->implemented_interfaces) {
            if (iface->equals(other)) return true;
        }
    }

    // Interface conformance: class/struct implementing interface
    if (o->kind == TypeKind::INTERFACE && o->definition) {
        if (definition) {
            for (const auto& iface : definition->implemented_interfaces) {
                if (iface->equals(other)) return true;
            }
        }
    }

    // Generic instantiation subtyping (covariance for classes is unsafe, so only exact match)
    return false;
}

// -- Type::unify ---------------------------------------------------------------

bool Type::unify(TypePtr other, TypeInterner& interner) const {
    // Handle type variables first
    if (kind == TypeKind::TYPE_VAR && type_var) {
        if (type_var->is_bound()) {
            return type_var->get_binding()->unify(other, interner);
        }
        // Occurs check
        if (other->kind == TypeKind::TYPE_VAR && other->type_var == type_var) return true;
        type_var->bind(other);
        return true;
    }
    if (other->kind == TypeKind::TYPE_VAR && other->type_var) {
        if (other->type_var->is_bound()) {
            return other->type_var->get_binding()->unify(shared_from_this(), interner);
        }
        if (kind == TypeKind::TYPE_VAR && type_var == other->type_var) return true;
        other->type_var->bind(shared_from_this());
        return true;
    }

    // INFER unifies with anything
    if (kind == TypeKind::INFER) return true;
    if (other->kind == TypeKind::INFER) return true;

    // ERROR unifies with anything (recovery)
    if (kind == TypeKind::ERROR || other->kind == TypeKind::ERROR) return true;

    // NEVER unifies with anything (divergence)
    if (kind == TypeKind::NEVER || other->kind == TypeKind::NEVER) return true;

    if (kind != other->kind) return false;

    switch (kind) {
        case TypeKind::FUNCTION:
            if (param_types.size() != other->param_types.size()) return false;
            if (!return_type->unify(other->return_type, interner)) return false;
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (!param_types[i]->unify(other->param_types[i], interner)) return false;
            }
            return true;
        case TypeKind::TUPLE:
            if (element_types.size() != other->element_types.size()) return false;
            for (size_t i = 0; i < element_types.size(); ++i) {
                if (!element_types[i]->unify(other->element_types[i], interner)) return false;
            }
            return true;
        case TypeKind::ARRAY:
        case TypeKind::SLICE:
            if (array_size != other->array_size) return false;
            return element_type->unify(other->element_type, interner);
        case TypeKind::RAW_PTR:
            if (is_mutable_ptr != other->is_mutable_ptr) return false;
            if (type_args.size() != other->type_args.size()) return false;
            for (size_t i = 0; i < type_args.size(); ++i) {
                if (!type_args[i]->unify(other->type_args[i], interner)) return false;
            }
            return true;
        case TypeKind::CLASS:
        case TypeKind::INTERFACE:
        case TypeKind::STRUCT:
        case TypeKind::ENUM:
            if (definition != other->definition) return false;
            if (type_args.size() != other->type_args.size()) return false;
            for (size_t i = 0; i < type_args.size(); ++i) {
                if (!type_args[i]->unify(other->type_args[i], interner)) return false;
            }
            return true;
        default:
            return equals(other);
    }
}

// -- TypeInterner --------------------------------------------------------------

TypePtr TypeInterner::intern(TypePtr t) {
    for (const auto& existing : canonical_types) {
        if (existing->equals(t)) return existing;
    }
    canonical_types.push_back(t);
    return t;
}

TypePtr TypeInterner::fresh_type_var(int level) {
    auto var = new TypeVariable(type_var_counter++, level);
    return Type::make_type_var(var);
}

void TypeInterner::register_type_def(std::unique_ptr<TypeDefinition> def) {
    type_defs[def->name] = std::move(def);
}

TypeDefinition* TypeInterner::find_type_def(const std::string& name) const {
    auto it = type_defs.find(name);
    return it == type_defs.end() ? nullptr : it->second.get();
}

void TypeInterner::register_type_class(std::unique_ptr<TypeClass> tc) {
    type_classes[tc->name] = std::move(tc);
}

TypeClass* TypeInterner::find_type_class(const std::string& name) const {
    auto it = type_classes.find(name);
    return it == type_classes.end() ? nullptr : it->second.get();
}

bool TypeInterner::satisfies_constraints(const TypePtr& type, const std::vector<TypePtr>& constraints) const {
    for (const auto& constraint : constraints) {
        // Constraint must be an interface type
        if (constraint->kind != TypeKind::INTERFACE) return false;
        if (!type->is_subtype_of(constraint, *const_cast<TypeInterner*>(this))) return false;
    }
    return true;
}

} // namespace novium
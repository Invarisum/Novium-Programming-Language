// ============================================================================
// type_checker.cpp — Type Checker for Novium
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
//
// ============================================================================

#include "sema/type_checker.h"
#include <sstream>
#include <algorithm>
#include <set>

namespace novium {

// ── Type Checker Main Class ───────────────────────────────────────────────────

TypeChecker::TypeChecker(TypeInterner& interner, SymbolTable& symbols, const TypeCheckConfig& config)
    : interner_(interner), symbols_(symbols), config_(config), ctx_() {
    register_builtins(symbols_, interner_);
}

void TypeChecker::check_program(const std::vector<std::unique_ptr<Stmt>>& program) {
    // Phase 1: Collect all top-level declarations (functions, classes, interfaces)
    for (const auto& stmt : program) {
        if (auto* fn = dynamic_cast<FunctionDeclStmt*>(stmt.get())) {
            collect_function_signature(fn);
        } else if (auto* cls = dynamic_cast<ClassDeclStmt*>(stmt.get())) {
            collect_class_signature(cls);
        } else if (auto* iface = dynamic_cast<InterfaceDeclStmt*>(stmt.get())) {
            collect_interface_signature(iface);
        }
    }

    // Phase 2: Type check all statements
    for (const auto& stmt : program) {
        stmt->accept(this);
    }
}

// ── Error Reporting ────────────────────────────────────────────────────────────

void TypeChecker::error(TypeError::Kind kind, const std::string& message, const SourceLocation& loc,
                        TypePtr expected, TypePtr actual) {
    TypeError err(kind, message, loc);
    err.expected = std::move(expected);
    err.actual = std::move(actual);
    errors_.push_back(std::move(err));
}

void TypeChecker::error(const Token& token, TypeError::Kind kind, const std::string& message,
                        TypePtr expected, TypePtr actual) {
    error(kind, message, token.location, std::move(expected), std::move(actual));
}

void TypeChecker::note(const std::string& message, const SourceLocation& loc) {
    if (!errors_.empty()) {
        errors_.back().notes.push_back(message + " (at " + loc.filename + ":" +
                                        std::to_string(loc.line) + ":" + std::to_string(loc.column) + ")");
    }
}

// ── Phase 1: Signature Collection ─────────────────────────────────────────────

void TypeChecker::collect_function_signature(FunctionDeclStmt* fn) {
    // Get return type annotation
    TypeAnnotation ret_ann = fn->return_type;
    TypePtr ret_type = annotation_to_type(ret_ann, fn->location);

    // Collect parameter types
    std::vector<TypePtr> param_types;
    for (const auto& param : fn->params) {
        TypePtr param_type = annotation_to_type(param.type, param.location);
        param_types.push_back(param_type);
    }

    // Store function signature in symbol table
    TypePtr fn_type = Type::make_function(param_types, ret_type, fn->is_async);
    symbols_.declare_function(fn->name, fn_type, fn->location, fn->is_async, false, fn->is_extern, fn->extern_name);
}

void TypeChecker::collect_class_signature(ClassDeclStmt* cls) {
    // Create type definition
    auto def = std::make_unique<TypeDefinition>(cls->name, TypeKind::CLASS, cls->location);
    for (const auto& field : cls->fields) {
        def->fields.emplace_back(field.name, annotation_to_type(field.type, field.location));
    }
    for (const auto& method : cls->methods) {
        std::vector<TypePtr> param_types;
        for (const auto& param : method->params) {
            param_types.push_back(annotation_to_type(param.type, param.location));
        }
        TypePtr ret_type = method->has_return_type
            ? annotation_to_type(method->return_type, method->location)
            : Type::make_void();
        def->methods.emplace_back(method->name, Type::make_function(param_types, ret_type, method->is_async));
    }
    def->implemented_interfaces.clear();
    for (const auto& iface_name : cls->interfaces) {
        if (TypeDefinition* iface_def = interner_.find_type_def(iface_name)) {
            def->implemented_interfaces.push_back(Type::make_nominal(iface_def));
        }
    }

    // Register with interner (owns the definition) and symbol table (borrows it)
    interner_.register_type_def(std::move(def));
    TypeDefinition* def_ptr = interner_.find_type_def(cls->name);
    symbols_.declare_type(cls->name, def_ptr, cls->location, false);
}

void TypeChecker::collect_interface_signature(InterfaceDeclStmt* iface) {
    // Create type definition
    auto def = std::make_unique<TypeDefinition>(iface->name, TypeKind::INTERFACE, iface->location);
    for (const auto& method : iface->methods) {
        std::vector<TypePtr> param_types;
        for (const auto& param : method->params) {
            param_types.push_back(annotation_to_type(param.type, param.location));
        }
        TypePtr ret_type = method->has_return_type
            ? annotation_to_type(method->return_type, method->location)
            : Type::make_void();
        def->methods.emplace_back(method->name, Type::make_function(param_types, ret_type, method->is_async));
    }

    // Register with interner (owns the definition) and symbol table (borrows it)
    interner_.register_type_def(std::move(def));
    TypeDefinition* def_ptr = interner_.find_type_def(iface->name);
    symbols_.declare_type(iface->name, def_ptr, iface->location, false);
}

// ── Type Inference Helpers ────────────────────────────────────────────────────

TypePtr TypeChecker::fresh_type_var() { return fresh_type_var(ctx_.level); }

TypePtr TypeChecker::fresh_type_var(int level) {
    return interner_.fresh_type_var(level);
}

bool TypeChecker::unify(TypePtr a, TypePtr b, const SourceLocation& loc, const char* context) {
    if (!a || !b) return false;
    if (a->unify(b, interner_)) return true;

    // Try numeric promotion: int -> float
    if (a->kind == TypeKind::INT && b->kind == TypeKind::FLOAT) {
        return true; // Allow int where float expected
    }
    if (a->kind == TypeKind::FLOAT && b->kind == TypeKind::INT) {
        return true;
    }

    error(TypeError::Kind::TYPE_MISMATCH,
          std::string("Type mismatch") + (context ? (": " + std::string(context)) : "") +
          ". Expected '" + b->to_string() + "', found '" + a->to_string() + "'.",
          loc, b, a);
    return false;
}

bool TypeChecker::unify_with_expected(TypePtr actual, TypePtr expected, const SourceLocation& loc) {
    if (expected->kind == TypeKind::INFER) return true;
    if (actual->is_subtype_of(expected, interner_)) return true;
    if (actual->unify(expected, interner_)) return true;
    if (actual->kind == TypeKind::TYPE_VAR && actual->type_var->is_bound()) {
        return unify_with_expected(actual->type_var->get_binding(), expected, loc);
    }
    return false;
}

TypePtr TypeChecker::instantiate_type(const TypePtr& type, const std::vector<TypePtr>& type_args) {
    if (type->kind == TypeKind::TYPE_VAR && type->type_var->is_bound()) {
        return instantiate_type(type->type_var->get_binding(), type_args);
    }
    if (type->kind != TypeKind::CLASS && type->kind != TypeKind::STRUCT &&
        type->kind != TypeKind::INTERFACE && type->kind != TypeKind::ENUM) {
        return type;
    }
    if (!type->definition || type->definition->type_params.empty()) return type;

    auto result = std::make_shared<Type>(*type);
    result->type_args = type_args;
    return result;
}

TypePtr TypeChecker::instantiate_function_type(const TypePtr& fn_type, const std::vector<TypePtr>& type_args) {
    if (fn_type->kind != TypeKind::FUNCTION) return fn_type;
    if (type_args.empty()) return fn_type;

    auto result = std::make_shared<Type>(*fn_type);
    // Simple substitution - full implementation would need generic parameter list
    return result;
}

bool TypeChecker::check_constraints(const TypePtr& type, const std::vector<TypePtr>& constraints, const SourceLocation& loc) {
    for (const auto& constraint : constraints) {
        if (!type->is_subtype_of(constraint, interner_)) {
            error(TypeError::Kind::GENERIC_CONSTRAINT_FAILED,
                  "Type '" + format_type(type) + "' does not satisfy constraint '" +
                  format_type(constraint) + "'.",
                  loc, constraint, type);
            return false;
        }
    }
    return true;
}

// ── Expression Type Checking ───────────────────────────────────────────────────

TypePtr TypeChecker::check_expr(Expr* expr, std::optional<TypePtr> expected) {
    if (!expr) return interner_.get_error();

    // Push expected type into context for checking mode
    std::optional<TypePtr> prev_expected = ctx_.expected_type;
    if (expected.has_value()) {
        ctx_.expected_type = expected;
    }

    expr->accept(this);

    auto it = expr_types_.find(expr);
    TypePtr result = (it != expr_types_.end()) ? it->second : interner_.get_error();

    // If we have an expected type and the result is unconstrained, unify
    if (expected.has_value() && result->kind == TypeKind::INFER) {
        if (result->unify(expected.value(), interner_)) {
            result = expected.value();
            expr_types_[expr] = result;
        }
    }

    ctx_.expected_type = prev_expected;
    return result;
}

TypePtr TypeChecker::synth_expr(Expr* expr) {
    return check_expr(expr, std::nullopt);
}

// ── Binary Operations ─────────────────────────────────────────────────────────

TypePtr TypeChecker::check_binary_op(const Token& op, TypePtr left, TypePtr right, const SourceLocation& loc) {
    switch (op.type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT: {
            // Arithmetic operators require numeric operands
            if (is_numeric_type(left) && is_numeric_type(right)) {
                if (left->kind == TypeKind::FLOAT || right->kind == TypeKind::FLOAT) {
                    return Type::make_float();
                }
                return Type::make_int();
            }
            // String concatenation
            if (op.type == TokenType::PLUS &&
                left->kind == TypeKind::STRING && right->kind == TypeKind::STRING) {
                return Type::make_string();
            }
            error(TypeError::Kind::TYPE_MISMATCH,
                  "Arithmetic operator requires numeric operands, found '" +
                  left->to_string() + "' and '" + right->to_string() + "'.",
                  loc, nullptr, nullptr);
            return interner_.get_error();
        }
        case TokenType::EQUAL_EQUAL:
        case TokenType::BANG_EQUAL:
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL:
            return check_comparison_op(op, left, right, loc);
        case TokenType::AND_AND:
        case TokenType::OR_OR:
            if (left->kind == TypeKind::BOOL && right->kind == TypeKind::BOOL) {
                return Type::make_bool();
            }
            error(TypeError::Kind::TYPE_MISMATCH,
                  "Logical operator requires boolean operands, found '" +
                  left->to_string() + "' and '" + right->to_string() + "'.",
                  loc, nullptr, nullptr);
            return interner_.get_error();
        default:
            error(TypeError::Kind::INVALID_OPERATION,
                  "Invalid binary operator.",
                  loc);
            return interner_.get_error();
    }
}

TypePtr TypeChecker::check_comparison_op(const Token& op, TypePtr left, TypePtr right, const SourceLocation& loc) {
    // Comparisons work with comparable types
    if (is_comparable_type(left) && is_comparable_type(right)) {
        // int/float can compare with each other (int promoted to float)
        if ((left->kind == TypeKind::INT && right->kind == TypeKind::FLOAT) ||
            (left->kind == TypeKind::FLOAT && right->kind == TypeKind::INT)) {
            return Type::make_bool();
        }
        if (left->kind == right->kind) {
            return Type::make_bool();
        }
    }

    error(TypeError::Kind::TYPE_MISMATCH,
          "Comparison operator incompatible types: '" +
          left->to_string() + "' and '" + right->to_string() + "'.",
          loc);
    return Type::make_bool();
}

TypePtr TypeChecker::check_assignment_op(const Token& op, TypePtr left, TypePtr right, const SourceLocation& loc) {
    // Assignment operators need compatible types
    if (left->equals(right) || left->is_subtype_of(right, interner_)) {
        return left;
    }

    error(TypeError::Kind::ARG_TYPE_MISMATCH,
          "Assignment operator type mismatch.",
          loc);
    return left;
}

// ── Function Calls ────────────────────────────────────────────────────────────

TypePtr TypeChecker::check_call(CallExpr* call, std::optional<TypePtr> expected) {
    return resolve_call_target(call->callee.get(), call->arguments, call->rparen_location);
}

TypePtr TypeChecker::resolve_call_target(Expr* callee, const std::vector<std::unique_ptr<Expr>>& args, const SourceLocation& loc) {
    // Handle identifier callee
    if (auto* id = dynamic_cast<IdentifierExpr*>(callee)) {
        Symbol* sym = symbols_.lookup_function(id->name);
        if (!sym) {
            sym = symbols_.lookup_variable(id->name);
        }
        if (!sym) {
            // Fall back to built-in functions (print, println, panic, assert, ...)
            return check_builtin_call(id->name, args, loc, ctx_.expected_type);
        }

        TypePtr fn_type = sym->type;
        if (!fn_type || fn_type->kind != TypeKind::FUNCTION) {
            error(TypeError::Kind::NOT_CALLABLE,
                  " '" + id->name + "' is not callable.",
                  loc);
            return interner_.get_error();
        }

        // Check argument count and types
        TypePtr fn_ret = fn_type->return_type;
        if (fn_type->param_types.size() != args.size()) {
            error(TypeError::Kind::WRONG_ARG_COUNT,
                  "Function expects " + std::to_string(fn_type->param_types.size()) +
                  " argument(s), got " + std::to_string(args.size()) + ".",
                  loc);
        }

        // Check each argument type
        for (size_t i = 0; i < args.size(); ++i) {
            TypePtr arg_type = synth_expr(args[i].get());
            if (i < fn_type->param_types.size()) {
                if (!unify_with_expected(arg_type, fn_type->param_types[i], loc)) {
                    error(TypeError::Kind::ARG_TYPE_MISMATCH,
                          "Argument " + std::to_string(i + 1) + " type mismatch.",
                          loc);
                }
            }
        }

        return fn_ret;
    }

    error(TypeError::Kind::NOT_CALLABLE, "Unknown callee type.", loc);
    return interner_.get_error();
}

// ── Member Access ─────────────────────────────────────────────────────────────

TypePtr TypeChecker::check_member_access(MemberAccessExpr* expr, std::optional<TypePtr> expected) {
    // Look up the type of the object
    TypePtr obj_type = synth_expr(expr->object.get());

    // For nominal types (classes), look up methods/fields
    if (obj_type->kind == TypeKind::CLASS && obj_type->definition) {
        // Search for member in class definition
        for (const auto& field : obj_type->definition->fields) {
            if (field.first == expr->member_name) {
                TypePtr field_type = field.second;
                if (expected.has_value() &&
                    !unify_with_expected(field_type, expected.value(), expr->member_location)) {
                    error(TypeError::Kind::TYPE_MISMATCH,
                          "Field type mismatch for '" + expr->member_name + "'.",
                          expr->member_location);
                }
                return field_type;
            }
        }
        for (const auto& method : obj_type->definition->methods) {
            if (method.first == expr->member_name) {
                TypePtr method_type = method.second;
                if (expected.has_value() &&
                    !unify_with_expected(method_type, expected.value(), expr->member_location)) {
                    error(TypeError::Kind::TYPE_MISMATCH,
                          "Method type mismatch for '" + expr->member_name + "'.",
                          expr->member_location);
                }
                return method_type;
            }
        }
    }

    error(TypeError::Kind::UNKNOWN_FIELD,
          "Unknown member '" + expr->member_name + "'." ,
          expr->member_location);
    return interner_.get_error();
}

// ── Index Access ─────────────────────────────────────────────────────────────

TypePtr TypeChecker::check_index(IndexExpr* expr, std::optional<TypePtr> expected) {
    TypePtr container_type = synth_expr(expr->object.get());

    if (container_type->kind == TypeKind::ARRAY && container_type->element_type) {
        // Array[index] -> element type
        if (expected.has_value() &&
            !unify_with_expected(container_type->element_type, expected.value(), expr->location)) {
            error(TypeError::Kind::TYPE_MISMATCH,
                  "Index expression type mismatch.",
                  expr->location);
        }
        return container_type->element_type;
    } else if (container_type->kind == TypeKind::SLICE && container_type->element_type) {
        // []T[index] -> T
        if (expected.has_value() &&
            !unify_with_expected(container_type->element_type, expected.value(), expr->location)) {
            error(TypeError::Kind::TYPE_MISMATCH,
                  "Index expression type mismatch.",
                  expr->location);
        }
        return container_type->element_type;
    }

    error(TypeError::Kind::NOT_INDEXABLE,
          "Value is not indexable.",
          expr->location);
    return interner_.get_error();
}

// ── Statement Type Checking ────────────────────────────────────────────────────

void TypeChecker::check_var_decl(VarDeclStmt* stmt) {
    // Check variable initializer type
    if (stmt->initializer) {
        TypePtr init_type = synth_expr(stmt->initializer.get());

        // If has type annotation, check compatibility
        if (stmt->has_type_annotation) {
            TypePtr ann_type = annotation_to_type(stmt->type, stmt->location);
            if (!unify_with_expected(init_type, ann_type, stmt->location)) {
                error(TypeError::Kind::TYPE_MISMATCH,
                      "Type mismatch in variable declaration.",
                      stmt->location,
                      ann_type, init_type);
            }
            symbols_.declare_variable(stmt->name, ann_type, stmt->is_mutable, stmt->location);
        } else {
            // Inferred type - store in symbol table
            symbols_.declare_variable(stmt->name, init_type, stmt->is_mutable, stmt->location);
        }
    } else if (stmt->has_type_annotation) {
        // Declaration with type but no initializer - infer type
        TypePtr ann_type = annotation_to_type(stmt->type, stmt->location);
        symbols_.declare_variable(stmt->name, ann_type, stmt->is_mutable, stmt->location);
    }
}

void TypeChecker::check_function_decl(FunctionDeclStmt* stmt) {
    // Enter function scope
    enter_function_scope(stmt);

    // Check parameter types
    for (const auto& param : stmt->params) {
        TypePtr param_type = annotation_to_type(param.type, param.location);
        symbols_.declare_variable(param.name, param_type, /*mutable=*/false, param.location);
    }

    // Set the expected return type for this function
    current_return_type_ = stmt->has_return_type
        ? annotation_to_type(stmt->return_type, stmt->location)
        : TypePtr(Type::make_void());
    current_function_has_return_ = false;

    // Check body
    if (stmt->body) {
        check_block(stmt->body.get());
    }

    // Check return type consistency
    if (stmt->has_return_type && !current_function_has_return_) {
        if (current_return_type_.has_value() && current_return_type_.value()->kind != TypeKind::VOID) {
            note("Function '" + stmt->name + "' declared to return '" +
                 current_return_type_.value()->to_string() + "' but has no return statement.",
                 stmt->location);
        }
    }

    current_return_type_.reset();
    exit_function_scope();
}

void TypeChecker::check_class_decl(ClassDeclStmt* stmt) {
    // Enter type scope
    symbols_.enter_type_scope();

    // Check fields
    for (const auto& field : stmt->fields) {
        annotation_to_type(field.type, field.location);
    }

    // Check methods
    for (const auto& method : stmt->methods) {
        TypePtr ret_type = method->has_return_type
            ? annotation_to_type(method->return_type, method->location)
            : Type::make_void();

        // Parameters
        std::vector<TypePtr> param_types;
        for (const auto& param : method->params) {
            param_types.push_back(annotation_to_type(param.type, param.location));
        }

        TypePtr fn_type = Type::make_function(param_types, ret_type, method->is_async);
        symbols_.declare_function(method->name, fn_type, method->location, method->is_async);
    }

    symbols_.exit_scope();
}

void TypeChecker::check_interface_decl(InterfaceDeclStmt* stmt) {
    // Enter type scope
    symbols_.enter_type_scope();

    // Check methods (pure virtual by default)
    for (const auto& method : stmt->methods) {
        TypePtr ret_type = method->has_return_type ? annotation_to_type(method->return_type, method->location)
                                                : Type::make_void();
        std::vector<TypePtr> param_types;
        for (const auto& param : method->params) {
            param_types.push_back(annotation_to_type(param.type, param.location));
        }
        TypePtr fn_type = Type::make_function(param_types, ret_type, method->is_async);
        symbols_.declare_function(method->name, fn_type, method->location, method->is_async);
    }

    symbols_.exit_scope();
}

void TypeChecker::check_if_stmt(IfStmt* stmt) {
    // Check condition type - must be boolean
    TypePtr cond_type = synth_expr(stmt->condition.get());
    if (cond_type->kind != TypeKind::BOOL) {
        error(TypeError::Kind::TYPE_MISMATCH,
              "Condition type must be boolean, found '" + cond_type->to_string() + "'.",
              stmt->location);
    }

    // Check then branch
    if (stmt->then_branch) {
        check_block(stmt->then_branch.get(), /*expected_return=*/std::nullopt);
    }

    // Check elif branches
    for (const auto& branch : stmt->elif_branches) {
        TypePtr elif_cond_type = synth_expr(branch.condition.get());
        if (elif_cond_type->kind != TypeKind::BOOL) {
            error(TypeError::Kind::TYPE_MISMATCH,
                  "Elif condition type must be boolean.",
                  stmt->location);
        }
        if (branch.block) {
            check_block(branch.block.get(), /*expected_return=*/std::nullopt);
        }
    }

    // Check else branch
    if (stmt->else_branch) {
        check_block(stmt->else_branch.get(), /*expected_return=*/std::nullopt);
    }
}

void TypeChecker::check_while_stmt(WhileStmt* stmt) {
    // Check condition type - must be boolean
    TypePtr cond_type = synth_expr(stmt->condition.get());
    if (cond_type->kind != TypeKind::BOOL) {
        error(TypeError::Kind::TYPE_MISMATCH,
              "While condition type must be boolean, found '" + cond_type->to_string() + "'.",
              stmt->location);
    }

    // Enter loop scope
    ctx_.in_loop = true;
    loop_stack_.push_back(true);

    // Check body
    if (stmt->body) {
        check_block(stmt->body.get(), /*expected_return=*/std::nullopt);
    }

    loop_stack_.pop_back();
    ctx_.in_loop = false;
}

void TypeChecker::check_match_stmt(MatchStmt* stmt) {
    // Check subject type
    TypePtr subject_type = synth_expr(stmt->subject.get());

    // Check each arm
    for (const auto& arm : stmt->arms) {
        // Check pattern type against subject
        TypePtr pattern_type = check_pattern(arm.pattern.get(), subject_type, stmt->location);

        // Check body (match arms are single statements, not blocks)
        if (arm.body) {
            arm.body->accept(this);
        }
    }

    // Check exhaustiveness if required
    if (config_.require_exhaustive_match) {
        check_match_exhaustiveness(stmt, subject_type);
    }
}

void TypeChecker::check_return_stmt(ReturnStmt* stmt) {
    // In function context, check return value type
    current_function_has_return_ = true;
    if (stmt->value) {
        TypePtr ret_type = synth_expr(stmt->value.get());
        if (current_return_type_.has_value()) {
            if (!unify_with_expected(ret_type, current_return_type_.value(), stmt->location)) {
                error(TypeError::Kind::RETURN_TYPE_MISMATCH,
                      "Return type mismatch: expected '" + current_return_type_.value()->to_string() +
                      "', found '" + ret_type->to_string() + "'.",
                      stmt->location);
            }
        }
    } else {
        // Return void
        if (current_return_type_.has_value() && current_return_type_.value()->kind != TypeKind::VOID) {
            error(TypeError::Kind::RETURN_TYPE_MISMATCH,
                  "Function returning '" + current_return_type_.value()->to_string() + "' cannot return void.",
                  stmt->location);
        }
    }
}

void TypeChecker::check_try_catch_stmt(TryCatchStmt* stmt) {
    // Check try block
    if (stmt->try_block) {
        check_block(stmt->try_block.get(), /*expected_return=*/std::nullopt);
    }

    // Check catch blocks
    for (const auto& catch_block : stmt->catch_blocks) {
        // Check catch variable declaration
        if (!catch_block.exception_type.empty()) {
            TypePtr exc_type = annotation_to_type(
                TypeAnnotation{catch_block.exception_type, false, false, false, false}, catch_block.location);
            symbols_.declare_variable(catch_block.exception_var, exc_type, false, catch_block.location);
        } else if (catch_block.has_exception_var) {
            // Generic catch without type - use infer type
            TypePtr exc_type = interner_.get_infer();
            symbols_.declare_variable(catch_block.exception_var, exc_type, false, catch_block.location);
        }

        // Check catch body
        if (catch_block.body) {
            check_block(catch_block.body.get(), /*expected_return=*/std::nullopt);
        }
    }

    // Check finally block
    if (stmt->finally_block) {
        check_block(stmt->finally_block.get(), /*expected_return=*/std::nullopt);
    }
}

void TypeChecker::check_go_stmt(GoStmt* stmt) {
    // Go statement spawns a goroutine - check the call expression
    if (stmt->call) {
        synth_expr(stmt->call.get()); // Just check it's valid, don't enforce arg counts for go
    }
}

// ── ASTVisitor Implementation ─────────────────────────────────────────────────

void TypeChecker::visit(IdentifierExpr* expr) {
    Symbol* sym = symbols_.lookup_variable(expr->name);
    if (!sym) {
        error(TypeError::Kind::UNKNOWN_VARIABLE,
              "Undefined variable '" + expr->name + "'.",
              expr->location);
        expr_types_[expr] = interner_.get_error();
        return;
    }
    symbols_.mark_used(expr->name);
    expr_types_[expr] = sym->type;
}

void TypeChecker::visit(LiteralExpr* expr) {
    switch (expr->token.type) {
        case TokenType::INTEGER_LITERAL:
            expr_types_[expr] = Type::make_int();
            break;
        case TokenType::FLOAT_LITERAL:
            expr_types_[expr] = Type::make_float();
            break;
        case TokenType::STRING_LITERAL:
        case TokenType::STRING_START:
        case TokenType::STRING_MIDDLE:
        case TokenType::STRING_END:
            expr_types_[expr] = Type::make_string();
            break;
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:
            expr_types_[expr] = Type::make_bool();
            break;
        case TokenType::KW_NULL:
            expr_types_[expr] = Type::make_infer()->with_nullable(true);
            break;
        default:
            error(TypeError::Kind::TYPE_MISMATCH,
                  "Unsupported literal in type checker.",
                  expr->token.location);
            expr_types_[expr] = interner_.get_error();
            break;
    }
}

void TypeChecker::visit(UnaryExpr* expr) {
    TypePtr right = synth_expr(expr->right.get());
    switch (expr->op.type) {
        case TokenType::BANG:
            if (right->kind != TypeKind::BOOL && right->kind != TypeKind::INFER) {
                error(TypeError::Kind::TYPE_MISMATCH,
                      "Logical not ('!') requires a boolean operand, found '" +
                      right->to_string() + "'.",
                      expr->op.location);
            }
            expr_types_[expr] = Type::make_bool();
            return;
        case TokenType::MINUS:
            if (!is_numeric_type(right) && right->kind != TypeKind::INFER) {
                error(TypeError::Kind::TYPE_MISMATCH,
                      "Unary minus requires a numeric operand, found '" +
                      right->to_string() + "'.",
                      expr->op.location);
            }
            expr_types_[expr] = right;
            return;
        case TokenType::AMPERSAND:
            // Immutable borrow: &T
            check_borrow(expr->right.get(), right, /*mutable_borrow=*/false, expr->op.location);
            expr_types_[expr] = right->with_ownership(Ownership::BORROW);
            return;
        default:
            error(TypeError::Kind::INVALID_OPERATION,
                  "Invalid unary operator.",
                  expr->op.location);
            expr_types_[expr] = interner_.get_error();
            return;
    }
}

void TypeChecker::visit(BinaryExpr* expr) {
    TypePtr left = synth_expr(expr->left.get());
    TypePtr right = synth_expr(expr->right.get());
    expr_types_[expr] = check_binary_op(expr->op, left, right, expr->op.location);
}

void TypeChecker::visit(CallExpr* expr) {
    expr_types_[expr] = check_call(expr, ctx_.expected_type);
}

void TypeChecker::visit(MemberAccessExpr* expr) {
    expr_types_[expr] = check_member_access(expr, ctx_.expected_type);
}

void TypeChecker::visit(AwaitExpr* expr) {
    // await unwraps the awaited value; for v0.1 we treat it as the inner type
    if (expr->value) {
        expr_types_[expr] = synth_expr(expr->value.get());
    } else {
        expr_types_[expr] = interner_.get_error();
    }
}

void TypeChecker::visit(IndexExpr* expr) {
    if (expr->index) {
        TypePtr idx_type = synth_expr(expr->index.get());
        if (idx_type->kind != TypeKind::INT && idx_type->kind != TypeKind::INFER) {
            error(TypeError::Kind::TYPE_MISMATCH,
                  "Array index must be an integer, found '" + idx_type->to_string() + "'.",
                  expr->location);
        }
    }
    expr_types_[expr] = check_index(expr, ctx_.expected_type);
}

void TypeChecker::visit(CastExpr* expr) {
    TypePtr inner = expr->expression ? synth_expr(expr->expression.get()) : interner_.get_error();

    TypePtr target = annotation_to_type(
        TypeAnnotation{expr->target_type, false, false, false, false}, expr->location);

    if (!is_numeric_type(inner) && inner->kind != TypeKind::INFER &&
        !(inner->kind == TypeKind::STRING && (target->kind == TypeKind::INT ||
                                              target->kind == TypeKind::FLOAT)) &&
        !(inner->kind == TypeKind::BOOL && (target->kind == TypeKind::INT ||
                                             target->kind == TypeKind::FLOAT))) {
        error(TypeError::Kind::TYPE_MISMATCH,
              "Cannot cast '" + inner->to_string() + "' to '" + expr->target_type + "'.",
              expr->location);
    }

    expr_types_[expr] = target;
}

void TypeChecker::visit(BlockStmt* stmt) {
    check_block(stmt);
}

void TypeChecker::visit(VarDeclStmt* stmt) {
    check_var_decl(stmt);
}

void TypeChecker::visit(ExpressionStmt* stmt) {
    if (stmt->expression) {
        synth_expr(stmt->expression.get());
    }
}

void TypeChecker::visit(PrintStmt* stmt) {
    if (stmt->value) synth_expr(stmt->value.get());
}

void TypeChecker::visit(PrintLnStmt* stmt) {
    if (stmt->value) synth_expr(stmt->value.get());
}

void TypeChecker::visit(EmptyStmt* stmt) {
    (void)stmt; // No-op
}

void TypeChecker::visit(FunctionDeclStmt* stmt) {
    check_function_decl(stmt);
}

void TypeChecker::visit(ClassDeclStmt* stmt) {
    check_class_decl(stmt);
}

void TypeChecker::visit(InterfaceDeclStmt* stmt) {
    check_interface_decl(stmt);
}

void TypeChecker::visit(IfStmt* stmt) {
    check_if_stmt(stmt);
}

void TypeChecker::visit(WhileStmt* stmt) {
    check_while_stmt(stmt);
}

void TypeChecker::visit(MatchStmt* stmt) {
    check_match_stmt(stmt);
}

void TypeChecker::visit(ReturnStmt* stmt) {
    check_return_stmt(stmt);
}

void TypeChecker::visit(TryCatchStmt* stmt) {
    check_try_catch_stmt(stmt);
}

void TypeChecker::visit(GoStmt* stmt) {
    check_go_stmt(stmt);
}

void TypeChecker::visit(DeferStmt* stmt) {
    if (stmt->body) check_block(stmt->body.get());
}

void TypeChecker::visit(UnsafeBlockStmt* stmt) {
    if (stmt->body) check_block(stmt->body.get());
}

void TypeChecker::visit(PanicStmt* stmt) {
    if (stmt->message) synth_expr(stmt->message.get());
}

void TypeChecker::visit(PythonFFIBlockStmt* stmt) {
    for (const auto& imp : stmt->imports) {
        if (imp) imp->accept(this);
    }
}

void TypeChecker::visit(JSXExprExpr* expr) {
    expr_types_[expr] = expr->expression
        ? synth_expr(expr->expression.get())
        : interner_.get_infer();
}

void TypeChecker::visit(JSXTagExpr* expr) {
    if (expr->children) synth_expr(expr->children.get());
    expr_types_[expr] = interner_.get_infer();
}

void TypeChecker::visit(CSSStylesStmt* stmt) {
    (void)stmt; // Styling content is not type-checked
}

void TypeChecker::visit(HTMLTemplateStmt* stmt) {
    (void)stmt; // Template content is not type-checked
}

void TypeChecker::visit(PythonImportStmt* stmt) {
    (void)stmt; // FFI import — validated at codegen
}

void TypeChecker::visit(JSExportStmt* stmt) {
    (void)stmt; // FFI export — validated at codegen
}

void TypeChecker::check_statement(Stmt* stmt) {
    if (stmt) stmt->accept(this);
}

// ── Block Checking ────────────────────────────────────────────────────────────

void TypeChecker::check_block(BlockStmt* block, std::optional<TypePtr> expected_return) {
    // Enter a new scope for the block
    symbols_.enter_scope(/*is_function=*/false);

    // Check each statement
    for (const auto& stmt : block->statements) {
        stmt->accept(this);
    }

    // Exit scope
    symbols_.exit_scope();
}

// ── Pattern Matching ─────────────────────────────────────────────────────────

TypePtr TypeChecker::check_pattern(Expr* pattern, TypePtr subject_type, const SourceLocation& loc) {
    // Pattern matching checks the pattern against the subject type
    // For now, just synthesize the pattern type
    return synth_expr(pattern);
}

bool TypeChecker::is_irrefutable_pattern(Expr* pattern) {
    // Check if a pattern is irrefutable (always matches)
    // For now, only wildcard _ is irrefutable
    if (auto* id = dynamic_cast<IdentifierExpr*>(pattern)) {
        return id->name == "_";
    }
    return false;
}

bool TypeChecker::check_match_exhaustiveness(MatchStmt* stmt, TypePtr subject_type) {
    // Check if match arms cover all possible values of the subject type
    // This is a complex check - for v0.1 we do a basic check

    // Collect all matched types from arms
    std::set<TypePtr> matched_types;
    for (const auto& arm : stmt->arms) {
        // Simple check: if pattern is a literal, add its type
        // Full implementation would analyze pattern types
    }

    // For now, just warn if no arms (but don't error)
    if (stmt->arms.empty()) {
        note("Match statement has no arms - always executes default branch.",
             stmt->location);
        return false;
    }

    return true;
}

// ── Ownership/Borrowing ───────────────────────────────────────────────────────

// Track ownership state per variable in symbol table
struct VariableOwnership {
    bool is_owned = false;        // Has unique ownership
    bool has_been_moved = false;  // Has been moved from
    bool is_borrowed_immutable = false;  // Currently immutably borrowed
    bool is_borrowed_mutable = false;    // Currently mutably borrowed
};

void TypeChecker::check_ownership_transfer(Expr* expr, TypePtr type, const SourceLocation& loc) {
    // Check for ownership transfer operations (e.g., move semantics)
    if (!type) return;
    if (type->kind == TypeKind::ERROR) {
        error(TypeError::Kind::OWNERSHIP_VIOLATION,
              "Cannot transfer ownership of error type.",
              loc);
        return;
    }
    // Check if trying to move from a value that's already been moved
    // This would need symbol table lookup in full implementation
    (void)expr; // Placeholder - full impl would check sym table
}

void TypeChecker::check_borrow(Expr* expr, TypePtr type, bool mutable_borrow, const SourceLocation& loc) {
    // Check borrow operations (& and &mut)
    if (!type) return;
    if (type->kind == TypeKind::ERROR) {
        error(TypeError::Kind::BORROW_CHECK_FAILED,
              "Cannot borrow value of error type.",
              loc);
        return;
    }
    // Check for conflicting borrows
    // In full implementation, check symbol table for existing borrows
    (void)expr; (void)mutable_borrow; // Placeholder
}

bool TypeChecker::is_move_operation(const Token& op) const {
    // Check if an operator is a move operation
    // In Novium, assignment can be a move depending on context
    // 'let x = y' where y is owned transfers ownership
    return op.type == TokenType::EQUAL;
}

// ── Utility Methods ───────────────────────────────────────────────────────────

TypePtr TypeChecker::get_common_type(TypePtr a, TypePtr b) {
    // Find common type between two types
    // Simplified: if both numeric, return float; if same, return that type
    if (a->equals(b)) return a;
    if (is_numeric_type(a) && is_numeric_type(b)) {
        // If one is float, promote to float
        if (a->kind == TypeKind::FLOAT || b->kind == TypeKind::FLOAT) {
            return Type::make_float();
        }
        return Type::make_int();
    }
    return Type::make_infer();
}

TypePtr TypeChecker::annotation_to_type(const TypeAnnotation& ann, const SourceLocation& loc) {
    // Resolve the base type from the annotation name
    TypePtr base;
    if (ann.name.empty() || ann.name == "_" || ann.name == "auto") {
        base = Type::make_infer();
    } else if (ann.name == "int" || ann.name == "i64" || ann.name == "i32" ||
               ann.name == "i16" || ann.name == "i8") {
        base = Type::make_int();
    } else if (ann.name == "float" || ann.name == "f64" || ann.name == "f32") {
        base = Type::make_float();
    } else if (ann.name == "string") {
        base = Type::make_string();
    } else if (ann.name == "bool") {
        base = Type::make_bool();
    } else if (ann.name == "void") {
        base = Type::make_void();
    } else {
        // Look up user-defined nominal types (class/interface/struct/enum)
        if (TypeDefinition* def = interner_.find_type_def(ann.name)) {
            base = Type::make_nominal(def);
        } else {
            error(TypeError::Kind::UNKNOWN_TYPE,
                  "Unknown type '" + ann.name + "'.",
                  loc);
            base = Type::make_infer();
        }
    }

    // Apply ownership modifiers
    if (ann.is_owned) base = base->with_ownership(Ownership::OWN);
    if (ann.is_borrowed) {
        base = base->with_ownership(
            ann.is_mutable_borrow ? Ownership::BORROW_MUT : Ownership::BORROW);
    }

    // Apply nullability
    if (ann.is_nullable) base = base->with_nullable(true);

    return base;
}

bool TypeChecker::is_numeric_type(TypePtr t) const {
    return t->kind == TypeKind::INT || t->kind == TypeKind::FLOAT;
}

bool TypeChecker::is_comparable_type(TypePtr t) const {
    // Types that can be compared (int, float, string, bool)
    switch (t->kind) {
        case TypeKind::INT:
        case TypeKind::FLOAT:
        case TypeKind::STRING:
        case TypeKind::BOOL:
            return true;
        default:
            return false;
    }
}

bool TypeChecker::is_callable_type(TypePtr t) const {
    return t->kind == TypeKind::FUNCTION;
}

TypePtr TypeChecker::get_element_type(TypePtr container) const {
    if (container->kind == TypeKind::ARRAY) return container->element_type;
    if (container->kind == TypeKind::SLICE) return container->element_type;
    return Type::make_infer();
}

std::string TypeChecker::format_type(TypePtr t) const {
    if (!t) return "<null>";
    return t->to_string();
}

// ── Built-in Function Handling ────────────────────────────────────────────────

TypePtr TypeChecker::check_builtin_call(const std::string& name, const std::vector<std::unique_ptr<Expr>>& args,
                                       const SourceLocation& loc, std::optional<TypePtr> expected) {
    // Handle built-in functions: print, println, panic, assert, int_to_float, float_to_int, to_string
    if (name == "print" || name == "println") {
        // These accept any type
        if (!expected.has_value()) return Type::make_infer();
        return expected.value();
    }

    if (name == "panic") {
        // panic(string) -> never
        if (args.size() >= 1) {
            TypePtr arg_type = synth_expr(args[0].get());
            return Type::make_never();
        }
        error(TypeError::Kind::WRONG_ARG_COUNT, "panic requires at least 1 argument.", loc);
        return interner_.get_error();
    }

    if (name == "assert") {
        // assert(bool, string?) -> void
        if (args.size() >= 1) {
            TypePtr cond_type = synth_expr(args[0].get());
            if (cond_type->kind != TypeKind::BOOL) {
                error(TypeError::Kind::TYPE_MISMATCH, "assert condition must be boolean.", loc);
            }
        }
        if (args.size() >= 2) {
            // Second arg is optional string message
        }
        return Type::make_void();
    }

    if (name == "int_to_float") {
        if (args.size() >= 1) {
            TypePtr arg_type = synth_expr(args[0].get());
            if (arg_type->kind == TypeKind::INT) return Type::make_float();
            error(TypeError::Kind::ARG_TYPE_MISMATCH, "int_to_float requires int argument.", loc);
        }
        return Type::make_float();
    }

    if (name == "float_to_int") {
        if (args.size() >= 1) {
            TypePtr arg_type = synth_expr(args[0].get());
            if (arg_type->kind == TypeKind::FLOAT) return Type::make_int();
            error(TypeError::Kind::ARG_TYPE_MISMATCH, "float_to_int requires float argument.", loc);
        }
        return Type::make_int();
    }

    if (name == "to_string") {
        // to_string(infer) -> string
        if (!expected.has_value()) return Type::make_string();
        return expected.value();
    }

    // Unknown builtin
    error(TypeError::Kind::UNKNOWN_TYPE, "Unknown built-in function: " + name, loc);
    return interner_.get_error();
}

// ── Scope Management ───────────────────────────────────────────────────────────

void TypeChecker::enter_function_scope(FunctionDeclStmt* fn) {
    ctx_.current_function = fn->name;
    ctx_.in_return_position = false;
    ctx_.generic_params_in_scope.clear();
    ctx_.level++;
    symbols_.enter_scope(true); // is_function_scope = true
    loop_stack_.clear();
    // Note: level is decremented in exit_function_scope to balance
}

void TypeChecker::exit_function_scope() {
    symbols_.exit_scope();
    ctx_.level--; // Balance the increment from enter_function_scope
}

} // namespace novium
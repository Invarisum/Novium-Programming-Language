// ============================================================================
// types.h — Type System for Novium
// ============================================================================
//
// The type system represents all types in the language with support for:
// - Primitives: int, float, string, bool, void
// - User-defined: classes, interfaces, structs, enums (ADTs)
// - Generics: type parameters with constraints
// - Ownership: own, & (borrow), &mut (mutable borrow)
// - Nullability: T?
// - Functions: (T, U) -> R
// - Collections: Array<T>, Slice<T>, Tuple<T...>
// - Type variables for inference
//
// Types are interned (unique pointers compared by address) for fast equality.
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <variant>
#include <cstdint>
#include <iostream>
#include "parser/ast.h"

namespace novium {

// Forward declarations
class Type;
using TypePtr = std::shared_ptr<const Type>;
class TypeInterner;
class TypeVariable;

// ── Ownership Kind ────────────────────────────────────────────────────────────
enum class Ownership {
    NONE,      // Plain value (copy/move semantics)
    OWN,       // Unique ownership (own T) - like Box<T> or unique_ptr
    BORROW,    // Immutable borrow (&T) - shared reference
    BORROW_MUT // Mutable borrow (&mut T) - exclusive reference
};

// ── Hardware Target ───────────────────────────────────────────────────────────
// Finite choice for code compilation/execution target hardware.
// Ensures deterministic code generation and interop.
enum class HardwareTarget {
    CPU,                           // Native x86_64 / ARM code generation
    GPU,                           // Parallel data-parallel execution
    CUDA,                          // NVIDIA CUDA kernels
    ROCm,                          // AMD ROCm kernels
    ONEAPI,                        // Intel oneAPI / DPC++ kernels
    NPU,                           // Neural network inference acceleration
    ACCELERATOR,                   // Custom hardware co-processor (via FFI)
    AUTO                           // Let runtime decide best target
};

// ── Type Kind ─────────────────────────────────────────────────────────────────
enum class TypeKind {
    // Unresolved / Inference
    INFER,           // Type to be inferred (_)
    TYPE_VAR,        // Unification variable for inference
    
    // Primitives
    VOID,
    BOOL,
    INT8,            // i8
    INT16,           // i16
    INT32,           // i32
    INT,             // i64
    UINT8,           // u8
    UINT16,          // u16
    UINT32,          // u32
    UINT,            // u64
    FLOAT16,         // f16 (half precision)
    FLOAT,           // f64
    STRING,          // UTF-8 owned string
    CHAR,            // Unicode scalar value
    
    // Never type (for diverging functions: panic, exit, infinite loop)
    NEVER,
    
    // User-defined nominal types
    CLASS,           // class Name<...>
    INTERFACE,       // interface Name<...>
    STRUCT,          // struct Name<...>
    ENUM,            // enum Name<...> (Algebraic Data Type)
    
    // Structural types
    FUNCTION,        // (T, U) -> R
    TUPLE,           // (T, U, ...)
    ARRAY,           // [T; N] - fixed size
    SLICE,           // []T - dynamic view
    
    // Pointer/Reference types (explicit, for C interop)
    RAW_PTR,         // *const T / *mut T (unsafe)
    
    // ML/DL Graph types
    TENSOR,          // Tensor<float> - for neural network computations
    MATRIX,          // Matrix<float> - 2D tensor operations
    WEIGHT,          // Learnable weight parameter
    BIAS,            // Learnable bias parameter
    OPTIMIZER_STATE, // Optimizer state (momentum, etc.)
    
    // Error type (for recovery)
    ERROR
};

// ── Type Parameter (for generics) ─────────────────────────────────────────────
struct TypeParameter {
    std::string name;
    std::vector<TypePtr> constraints; // e.g., T: Clone + Debug
    int index; // Position in parameter list
    
    TypeParameter(std::string n, std::vector<TypePtr> c, int i)
        : name(std::move(n)), constraints(std::move(c)), index(i) {}
};

// ── Enum Variant (for Algebraic Data Types) ───────────────────────────────────
struct EnumVariant {
    std::string name;
    std::vector<TypePtr> payload_types; // Empty for unit variants
    
    EnumVariant(std::string n, std::vector<TypePtr> p)
        : name(std::move(n)), payload_types(std::move(p)) {}
};

// ── Type Class (for trait/interface constraints) ──────────────────────────────
struct TypeClass {
    std::string name;
    std::vector<TypeParameter> params;
    std::vector<std::pair<std::string, TypePtr>> methods; // name -> function type
    
    TypeClass(std::string n) : name(std::move(n)) {}
};

// ── Type Definition (for nominal types) ───────────────────────────────────────
struct TypeDefinition {
    std::string name;
    TypeKind kind; // CLASS, INTERFACE, STRUCT, ENUM
    std::vector<TypeParameter> type_params;
    std::vector<std::pair<std::string, TypePtr>> fields; // For struct/class
    std::vector<EnumVariant> variants; // For enum
    std::vector<std::pair<std::string, TypePtr>> methods; // name -> function type
    std::vector<TypePtr> implemented_interfaces; // For class/struct
    std::optional<TypePtr> base_class; // For class (single inheritance)
    bool is_generic;
    SourceLocation location;
    
    TypeDefinition(std::string n, TypeKind k, SourceLocation loc)
        : name(std::move(n)), kind(k), is_generic(false), location(loc) {}
};

// ── Type Variable (for unification during inference) ──────────────────────────
class TypeVariable {
public:
    int id;
    int level; // For generalized vs. monomorphic variables
    std::optional<TypePtr> binding; // If unified with another type
    
    TypeVariable(int i, int l) : id(i), level(l) {}
    
    bool is_bound() const { return binding.has_value(); }
    TypePtr get_binding() const { return binding.value_or(nullptr); }
    void bind(TypePtr t) { binding = t; }
};

// ── Main Type Class ───────────────────────────────────────────────────────────
class Type : public std::enable_shared_from_this<Type> {
public:
    TypeKind kind;
    Ownership ownership = Ownership::NONE;
    bool nullable = false;
    HardwareTarget hardware_target = HardwareTarget::AUTO;
    
    // Kind-specific data
    std::vector<TypePtr> type_args; // For generic instantiations
    TypeDefinition* definition = nullptr; // For nominal types
    
    // For FUNCTION type
    std::vector<TypePtr> param_types;
    TypePtr return_type;
    bool is_async = false;
    
    // For TUPLE type
    std::vector<TypePtr> element_types;
    
    // For ARRAY type
    TypePtr element_type = nullptr;
    std::optional<int64_t> array_size; // null for slice
    
    // For TYPE_VAR
    TypeVariable* type_var = nullptr;
    
    // For RAW_PTR
    bool is_mutable_ptr = false;
    
    // LIFETIME TRACKING: generation counter for automatic memory management
    // Each type instance gets a unique generation ID; when generation decrements to 0,
    // the memory can be safely reclaimed (ARCs, refcounting, or scoped cleanup)
    int64_t generation_id = 0;
    
    // ML/DL Graph specific fields
    // Shape/-dimensions of tensor/matrix types
    std::vector<int64_t> shape; // e.g., [batch, height, width, channels]
    TypePtr dtype; // Element type (e.g., FLOAT16, FLOAT32)
    size_t element_count; // Total number of elements = product of shape
    
    // Operation kind for graph nodes
    enum class OpKind { NONE, ADD, MUL, MATMUL, CONV, POOL, ACTIVATION, RELU, SIGMOID, TANH };
    OpKind op = OpKind::NONE;
    
    // For graph node profiling
    std::string op_name; // Human-readable operation name
    int64_t flops; // Estimated floating-point operations
    size_t memory_bytes; // Memory footprint in bytes
    
    // Graph node input/output indices
    int input_idx = -1;
    int output_idx = -1;
    
    // Constructors
    Type(TypeKind k) : kind(k), generation_id(1), element_count(0) {
        shape = {};
        dtype = nullptr;
        op = OpKind::NONE;
        flops = 0;
        memory_bytes = 0;
    }
    
    static TypePtr make_void(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::VOID); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_bool(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::BOOL); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_int(HardwareTarget hw = HardwareTarget::AUTO, int size = 64) { 
        auto k = size <= 32 ? TypeKind::INT32 : TypeKind::INT; 
        auto t = std::make_shared<Type>(k); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_float(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::FLOAT); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_string(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::STRING); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_char(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::CHAR); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_never(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::NEVER); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_error(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::ERROR); 
        t->hardware_target = hw; 
        return t; 
    }
    static TypePtr make_infer(HardwareTarget hw = HardwareTarget::AUTO) { 
        auto t = std::make_shared<Type>(TypeKind::INFER); 
        t->hardware_target = hw; 
        return t; 
    }
    
    // Tensor type: [batch, height, width, channels] with element type
    static TypePtr make_tensor(std::vector<int64_t> shape, TypePtr dtype, HardwareTarget hw = HardwareTarget::AUTO) {
        auto t = std::make_shared<Type>(TypeKind::TENSOR);
        t->hardware_target = hw;
        t->shape = std::move(shape);
        t->dtype = dtype;
        // Calculate element count
        t->element_count = 1;
        for (int64_t dim : t->shape) {
            t->element_count *= dim;
        }
        return t;
    }
    
    // Matrix type: 2D tensor [rows, cols]
    static TypePtr make_matrix(int64_t rows, int64_t cols, TypePtr dtype, HardwareTarget hw = HardwareTarget::AUTO) {
        auto t = std::make_shared<Type>(TypeKind::MATRIX);
        t->hardware_target = hw;
        t->shape = {rows, cols};
        t->dtype = dtype;
        t->element_count = rows * cols;
        return t;
    }
    
    static TypePtr make_type_var(TypeVariable* var) {
        auto t = std::make_shared<Type>(TypeKind::TYPE_VAR);
        t->type_var = var;
        return t;
    }
    
    static TypePtr make_function(std::vector<TypePtr> params, TypePtr ret, bool async = false) {
        auto t = std::make_shared<Type>(TypeKind::FUNCTION);
        t->param_types = std::move(params);
        t->return_type = std::move(ret);
        t->is_async = async;
        return t;
    }
    
    static TypePtr make_tuple(std::vector<TypePtr> elements) {
        auto t = std::make_shared<Type>(TypeKind::TUPLE);
        t->element_types = std::move(elements);
        return t;
    }
    
    static TypePtr make_array(TypePtr elem, std::optional<int64_t> size = std::nullopt) {
        auto t = std::make_shared<Type>(TypeKind::ARRAY);
        t->element_type = std::move(elem);
        t->array_size = size;
        return t;
    }
    
    static TypePtr make_slice(TypePtr elem) {
        auto t = std::make_shared<Type>(TypeKind::SLICE);
        t->element_type = std::move(elem);
        return t;
    }
    
    static TypePtr make_raw_ptr(TypePtr pointee, bool mut = false) {
        auto t = std::make_shared<Type>(TypeKind::RAW_PTR);
        t->type_args.push_back(std::move(pointee));
        t->is_mutable_ptr = mut;
        return t;
    }
    
    static TypePtr make_nominal(TypeDefinition* def, std::vector<TypePtr> args = {}) {
        auto t = std::make_shared<Type>(def->kind);
        t->definition = def;
        t->type_args = std::move(args);
        return t;
    }
    
    // Ownership modifiers
    TypePtr with_ownership(Ownership o) const {
        auto copy = std::make_shared<Type>(*this);
        copy->ownership = o;
        return copy;
    }
    
    TypePtr with_nullable(bool n = true) const {
        auto copy = std::make_shared<Type>(*this);
        copy->nullable = n;
        return copy;
    }
    
    // Check if this is a type variable (possibly bound)
    bool is_type_var() const {
        if (kind == TypeKind::TYPE_VAR) return true;
        if (kind == TypeKind::INFER) return true;
        return false;
    }
    
    // Get the concrete type (following bindings)
    TypePtr deref() const {
        if (kind == TypeKind::TYPE_VAR && type_var && type_var->is_bound()) {
            return type_var->get_binding()->deref();
        }
        return std::const_pointer_cast<Type>(shared_from_this());
    }
    
    // Pretty printing
    std::string to_string() const;
    
    // Equality (structural, with type variable handling)
    bool equals(const TypePtr& other) const;
    
    // Subtyping check
    bool is_subtype_of(const TypePtr& other, TypeInterner& interner) const;
    
    // Unification (mutates type variable bindings, hence const-callable)
    bool unify(TypePtr other, TypeInterner& interner) const;

private:
    std::string to_string_impl(std::unordered_set<const Type*>& visited) const;
};

// ── Type Interner (canonicalizes types for fast equality) ─────────────────────
class TypeInterner {
public:
    TypePtr intern(TypePtr t);
    
    // Get/create builtin types
    TypePtr get_void() { return void_type; }
    TypePtr get_bool() { return bool_type; }
    TypePtr get_int() { return int_type; }
    TypePtr get_float() { return float_type; }
    TypePtr get_string() { return string_type; }
    TypePtr get_char() { return char_type; }
    TypePtr get_never() { return never_type; }
    TypePtr get_error() { return error_type; }
    TypePtr get_infer() { return infer_type; }
    
    // Create fresh type variable
    TypePtr fresh_type_var(int level = 0);
    
    // Register a type definition
    void register_type_def(std::unique_ptr<TypeDefinition> def);
    TypeDefinition* find_type_def(const std::string& name) const;
    
    // Register a type class (trait)
    void register_type_class(std::unique_ptr<TypeClass> tc);
    TypeClass* find_type_class(const std::string& name) const;
    
    // Constraint satisfaction
    bool satisfies_constraints(const TypePtr& type, const std::vector<TypePtr>& constraints) const;

private:
    // Canonical type storage
    std::vector<TypePtr> canonical_types;
    
    // Builtin types
    TypePtr void_type = Type::make_void();
    TypePtr bool_type = Type::make_bool();
    TypePtr int_type = Type::make_int();
    TypePtr float_type = Type::make_float();
    TypePtr string_type = Type::make_string();
    TypePtr char_type = Type::make_char();
    TypePtr never_type = Type::make_never();
    TypePtr error_type = Type::make_error();
    TypePtr infer_type = Type::make_infer();
    
    // Type definitions
    std::unordered_map<std::string, std::unique_ptr<TypeDefinition>> type_defs;
    std::unordered_map<std::string, std::unique_ptr<TypeClass>> type_classes;
    
    // Type variable counter
    int type_var_counter = 0;
};

// ── Utility Functions ─────────────────────────────────────────────────────────

// Common type predicates
inline bool is_primitive(const TypePtr& t) {
    switch (t->kind) {
        case TypeKind::BOOL:
        case TypeKind::INT:
        case TypeKind::FLOAT:
        case TypeKind::STRING:
        case TypeKind::CHAR:
            return true;
        default:
            return false;
    }
}

inline bool is_numeric(const TypePtr& t) {
    return t->kind == TypeKind::INT || t->kind == TypeKind::FLOAT;
}

inline bool is_void(const TypePtr& t) {
    return t->kind == TypeKind::VOID;
}

inline bool is_never(const TypePtr& t) {
    return t->kind == TypeKind::NEVER;
}

inline bool is_error(const TypePtr& t) {
    return t->kind == TypeKind::ERROR;
}

// Get the dereferenced type (follow type variable bindings)
inline TypePtr deref_type(const TypePtr& t) {
    return t->deref();
}

// Check if two types are the same (after dereferencing)
inline bool same_type(const TypePtr& a, const TypePtr& b) {
    return a->equals(b);
}

// ── Diagnostic Types ──────────────────────────────────────────────────────────

struct TypeError {
    enum class Kind {
        TYPE_MISMATCH,
        UNKNOWN_TYPE,
        UNKNOWN_VARIABLE,
        UNKNOWN_FIELD,
        UNKNOWN_METHOD,
        INVALID_OPERATION,
        MISSING_RETURN,
        RETURN_TYPE_MISMATCH,
        WRONG_ARG_COUNT,
        ARG_TYPE_MISMATCH,
        NOT_CALLABLE,
        NOT_INDEXABLE,
        NOT_ITERABLE,
        PATTERN_TYPE_MISMATCH,
        NON_EXHAUSTIVE_MATCH,
        OWNERSHIP_VIOLATION,
        BORROW_CHECK_FAILED,
        MOVE_AFTER_BORROW,
        USE_AFTER_MOVE,
        RECURSIVE_TYPE,
        GENERIC_CONSTRAINT_FAILED,
        AMBIGUOUS_TYPE,
        CYCLIC_DEFINITION,
        UNUSED_TYPE_PARAM
    };
    
    Kind kind;
    std::string message;
    SourceLocation location;
    TypePtr expected;
    TypePtr actual;
    std::vector<std::string> notes; // Additional context
    
    TypeError(Kind k, std::string m, SourceLocation loc)
        : kind(k), message(std::move(m)), location(loc) {}
};

} // namespace novium
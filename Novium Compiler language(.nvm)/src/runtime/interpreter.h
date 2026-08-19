#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>

#include "parser/ast.h"
#include "sema/types.h"

namespace novium {

using ValueData = std::variant<std::monostate, long long, double, bool, std::string>;

/// Ownership modifiers for .nvm memory management
/// .nvm has the most advanced: full ownership tracking enables compile-time
/// memory lifetime analysis and zero-cost abstractions
enum class MemoryOwnership {
    NONE,      // No ownership (borrowed, temporary)
    OWN,       // Exclusive ownership (RAII, deallocate when dropped)
    BORROW,    // Shared read-only borrow (ref-counted, no dealloc)
    BORROW_MUT // Shared mutable borrow (exclusive during lifetime)
};

struct Value {
    ValueData data;
    int64_t generation_id;  // For lifetime tracking / auto-reclamation
    MemoryOwnership ownership; // .nvm: track who owns this value

    Value() : generation_id(1), ownership(MemoryOwnership::NONE) {}
    Value(ValueData d) : data(d), generation_id(1), ownership(MemoryOwnership::NONE) {}
    Value(ValueData d, int64_t gen) : data(d), generation_id(gen), ownership(MemoryOwnership::NONE) {}
    Value(ValueData d, int64_t gen, MemoryOwnership ow) : data(d), generation_id(gen), ownership(ow) {}

    std::string to_string() const;
    bool truthy() const;
};

// Runtime environment: name -> binding chain
struct Binding {
    Value value;
    bool mutable_binding;
};

class Environment {
public:
    std::unordered_map<std::string, Binding> values;
    Environment* parent = nullptr;
};

// Internal control-flow signal for `return` statements
struct ReturnSignal {
    Value value;
};

// Memory management units for runtime
struct MemoryBlock {
    void* ptr;
    size_t size;
    int64_t generation_id;
    MemoryOwnership ownership; // Track ownership for reclamation
    int ref_count;  // Reference counting for shared memory

    MemoryBlock()
        : ptr(nullptr), size(0), generation_id(0), ownership(MemoryOwnership::NONE), ref_count(0) {}
    MemoryBlock(void* p, size_t s, int64_t gen, MemoryOwnership ow = MemoryOwnership::NONE)
        : ptr(p), size(s), generation_id(gen), ownership(ow), ref_count(1) {}
};

class MemoryManager {
public:
    // .nvm: Advanced memory management with ownership tracking

    // Allocate memory with ownership specification
    static void* allocate(size_t size, int64_t generation_id = 1, MemoryOwnership ownership = MemoryOwnership::NONE);

    // Deallocate memory (ownership-aware)
    static void deallocate(void* ptr);

    // Reference counting (ownership-aware)
    static int64_t add_ref(void* ptr);
    static int64_t release(void* ptr);
    static bool is_valid(void* ptr);

    // Generation-based reclamation (enhanced with ownership)
    // .nvm: Sweeps generations, respects ownership rules
    static void sweep_generation(int64_t gen);

    // Memory pool allocation (for frequent small allocations)
    // .nvm: Pool-based allocation for performance
    static void* allocate_from_pool(size_t size, MemoryOwnership ownership);
    static void deallocate_to_pool(void* ptr);

    // Ownership transfer
    // .nvm: Transfer ownership from one pointer to another
    static bool transfer_ownership(void* from, void* to, MemoryOwnership new_ow);

    // Get ownership of a pointer
    static MemoryOwnership get_ownership(void* ptr);

    // Set ownership of a pointer
    static void set_ownership(void* ptr, MemoryOwnership ow);

    // Memory statistics (for .nvm profiling)
    struct MemoryStats {
        size_t total_allocated;
        size_t currently_alive;
        size_t peak_allocated;
        size_t num_blocks;
        size_t num_pool_allocations;
        size_t pool_efficiency; // 0.0 - 1.0
    };
    static MemoryStats get_memory_stats();

    // .nvm: Force immediate reclamation of unused memory
    static void force_reclaim();

private:
    // Lazily initialize the size-class pools
    static void init_pool_system();

    static std::unordered_map<void*, MemoryBlock> blocks_;
    static std::mutex blocks_mutex_;
    // .nvm: Memory pools for different size classes
    static std::unordered_map<size_t, std::vector<void*>> size_pools_;
    static size_t next_pool_size_;
    static int64_t generation_id_;
};

// Runtime value serialization for interop
struct NoviumValueSerializer {
    // Serialize Value to portable format
    static std::string serialize(const Value& val);

    // Deserialize from portable format
    static Value deserialize(const std::string& data);

    // Format types
    enum class Format { JSON, BINARY, MSGPACK };
};

// Data interchange formats for cross-language/interop
struct DataPacket {
    std::string type_tag;  // e.g., "novium/int", "novium/float", "novium/string"
    std::string payload;   // Serialized data
    int64_t generation_id; // For lifetime tracking - CONSISTENT ACROSS .nvm/.nvi/.nvw
    bool is_shared;        // Whether this is shared memory reference

    // Trio compatibility: explicit constructor for all three variants
    DataPacket() : type_tag("novium/int"), generation_id(1), is_shared(false) {}
    DataPacket(const std::string& tag, const std::string& pay, int64_t gen = 1, bool shared = false)
        : type_tag(tag), payload(pay), generation_id(gen), is_shared(shared) {}
};

// Cross-language type mapping for interop
struct TypeMapper {
    // Novium type -> serialization format (CONSISTENT ACROSS ALL THREE VARIANTS)
    static std::string to_serialization_format(TypeKind kind);

    // Serialization format -> Novium type (REVERSE, consistent across variants)
    static TypeKind from_serialization_format(const std::string& format);

    // Trio: Get format name for each variant
    static std::string format_for_variant(TypeKind kind, const std::string& variant); // "nvm", "nvi", or "nvw"
};

// Main Interpreter class
class Interpreter {
public:
    Interpreter() : environment_(&globals_) {}

    void run(const std::vector<std::unique_ptr<Stmt>>& program);

    // Memory management
    void* allocate_memory(size_t size) {
        // .nvi: Use MemoryManager with NONE ownership (automatic GC)
        return MemoryManager::allocate(size, 1, MemoryOwnership::NONE);
    }

    void deallocate_memory(void* ptr) {
        // .nvi: Return to MemoryManager for GC tracking
        MemoryManager::deallocate(ptr);
    }

    void add_reference(void* ptr) {
        // .nvi: Increment ref count for shared memory
        MemoryManager::add_ref(ptr);
    }

    void release_reference(void* ptr) {
        // .nvi: Decrement ref count, potentially free
        MemoryManager::release(ptr);
    }

    // Data serialization for interop
    std::string serialize_value(const Value& val);
    Value deserialize_value(const std::string& data);

    // Global environment access
    void set_globals(const std::unordered_map<std::string, Value>& globals);
    Value get_global(const std::string& name) const;
    void add_built_in(const std::string& name, FunctionDeclStmt* func);
    std::unordered_map<std::string, Value> get_globals() const;

    // Architecture support
    enum class Architecture { X86_64, AARCH64, RISCV64, ARM64 };
    static Architecture current_architecture();
    static std::string architecture_name();

    // ML framework interop support
    bool support_ml_framework(const std::string& framework) const;

    // Python FFI integration (consistent across .nvm/.nvi/.nvw)
    bool support_python(const std::string& module_name) const;
    Value call_python_function(const std::string& function_name,
                              const std::vector<Value>& args = {},
                              const std::string& module_name = "novium_bridge");
    Value import_python_module(const std::string& module_name);

    // React/JSX bridge (consistent across .nvi/.nvw)
    bool support_react() const;
    std::string compile_jsx(const std::string& jsx_source);
    Value create_react_element(const std::string& type,
                               const std::unordered_map<std::string, Value>& props);
    std::string render_react(const std::string& component_name,
                            const std::unordered_map<std::string, Value>& props);

    // === FULL-STACK TRIO COMPATIBILITY ===

    // Module import/export across all three variants
    bool import_module(const std::string& module_name, const std::string& from_variant = "nvm");

    // Cross-variant function calling
    Value call_nvm_function_from_nvi(const std::string& function_name,
                                     const std::vector<Value>& args);
    Value call_nvi_function_from_nvm(const std::string& function_name,
                                    const std::vector<Value>& args);
    Value call_nvw_function(const std::string& function_name,
                           const std::vector<Value>& args);

    // DataPacket serialization/deserialization (trio-compatible)
    DataPacket serialize_to_datapacket(const Value& val);
    Value deserialize_from_datapacket(const DataPacket& pkt);

    // Shared global environment across all three variants
    void sync_globals_across_trio(const std::unordered_map<std::string, Value>& globals,
                                  const std::string& from_variant);
    std::unordered_map<std::string, Value> get_trio_globals(const std::string& variant);

    // Trio version and compatibility info
    std::string trio_compatibility_info() const;

    // Additional runtime methods
    static void register_python_react_builtins(Interpreter& interp);

    // Async task management
    void create_async_task(const std::string& task_name, std::function<void(Interpreter*)> func);
    Value await_task(const std::string& task_name);
    std::string mark_async(Stmt* stmt);

    // Value helpers
    std::string type_name(const Value& value);
    double number(const Value& value, const std::string& operation);
    bool equal(const Value& left, const Value& right);

private:
    // Core execution engine
    Binding* resolve(const std::string& name);
    void execute(Stmt* stmt);
    void execute_block(BlockStmt* block, Environment& environment);
    Value evaluate(Expr* expr);
    Value call(FunctionDeclStmt* function, const std::vector<Value>& args);
    bool pattern_matches(Expr* pattern, Value subject);

    // Python/React bridge built-in callbacks
    Value call_builtin_python_import(const std::vector<Value>& args);
    Value call_builtin_python_call(const std::vector<Value>& args);
    Value call_builtin_jsx_compile(const std::vector<Value>& args);
    Value call_builtin_react_create(const std::vector<Value>& args);
    Value call_builtin_react_render(const std::vector<Value>& args);

    // Runtime state
    Environment globals_;
    Environment* environment_;
    std::unordered_map<std::string, FunctionDeclStmt*> functions_;

    // Memory management
    std::unordered_map<void*, int64_t> memory_generations_;
    std::mutex memory_mutex_;

    // Serialization cache
    std::unordered_map<std::string, std::string> serialization_cache_;
};

} // namespace novium
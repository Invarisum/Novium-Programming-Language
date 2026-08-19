// ============================================================================
// unified_connections.h — Unified Cross-Language Connections for Novium Ecosystem
// ============================================================================
//
// Establishes unified bidirectional connections between all three Novium language
// variants:
//   - .nvm: Backend Compiler (systems compilation, LLVM codegen)
//   - .nvi: Full-Stack Interpreter (runtime, goroutines, FFI)
//   - .nvw: Frontend Web Compiler (web components, SSR, JS/WASM)
//
// Ensures:
//   - .nvm <-> .nvi: Compiled .nvm modules can be loaded and executed by .nvi
//   - .nvi <-> .nvw: Interpreter can surface functions to web frontend
//   - .nvw <-> .nvm: Frontend can compile to backend target
//   - Consistent AST, values, types, and execution across all three
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <mutex>
#include <functional>

#include "sema/types.h"
#include "runtime/interpreter.h"
#include "serialization.h"
#include "codegen.h"

namespace unified {

// ============================================================================
// Unified Type System Across All Variants
// ============================================================================

// Unified type identifier used across .nvm, .nvi, .nvw
struct UnifiedType {
    // Type kind (consistent across all frontends)
    novium::TypeKind kind;

    // Type size in bits (architecture-dependent)
    uint8_t bit_width;

    // Whether the type is nullable
    bool nullable;

    // Hardware target association
    novium::HardwareTarget hardware_target;

    // Generation ID for lifetime tracking
    int64_t generation_id;

    // Shape info for tensor types
    std::vector<int64_t> shape;

    // Element count for tensor/matrix types
    size_t element_count;

    // Constructor
    UnifiedType()
        : kind(novium::TypeKind::INT),
          bit_width(64),
          nullable(false),
          hardware_target(novium::HardwareTarget::AUTO),
          generation_id(1),
          element_count(0) {}

    UnifiedType(novium::TypeKind k, uint8_t width, bool is_nullable = false,
                novium::HardwareTarget hw = novium::HardwareTarget::AUTO,
                int64_t gen = 1,
                std::vector<int64_t> sh = {},
                size_t elem_count = 0)
        : kind(k), bit_width(width), nullable(is_nullable),
          hardware_target(hw), generation_id(gen), shape(std::move(sh)),
          element_count(elem_count) {}
};

// ============================================================================
// Unified Value Across All Variants
// ============================================================================

// Unified value that can be transferred between .nvm, .nvi, .nvw
struct UnifiedValue {
    // The actual value data
    novium::ValueData data;

    // Type metadata for proper deserialization
    UnifiedType type;

    // Generation ID for lifetime tracking across language boundaries
    int64_t generation_id;

    // Source variant identifier (.nvm, .nvi, .nvw)
    std::string source_variant;

    // Constructor for basic values
    UnifiedValue()
        : generation_id(1), source_variant("nvm") {}

    UnifiedValue(novium::ValueData d, UnifiedType t, int64_t gen = 1,
                 std::string src = "nvm")
        : data(d), type(t), generation_id(gen), source_variant(std::move(src)) {}

    // Convert to native .nvm Value
    novium::Value to_nvm_value() const;

    // Convert to native .nvi Value
    novium::Value to_nvi_value() const;

    // Convert to native .nvw Value
    novium::Value to_nvw_value() const;
};

// ============================================================================
// Global Environment Sharing
// ============================================================================

// Shared global environment that can be accessed across all three variants
struct SharedGlobalEnvironment {
    // Map of variable name -> unified value
    std::unordered_map<std::string, UnifiedValue> variables;

    // Map of function name -> function pointer/handle
    std::unordered_map<std::string, std::function<UnifiedValue(std::vector<UnifiedValue>)>> functions;

    // Map of tensor name -> tensor metadata
    std::unordered_map<std::string, novium::TensorMeta> tensors;

    // Map of module name -> loaded module handle
    std::unordered_map<std::string, void*> modules;

    // Thread safety
    std::mutex mutex;

    // Singleton access
    static SharedGlobalEnvironment& instance();

    // Get value by name (with generation checking)
    bool get_value(const std::string& name, UnifiedValue& out, int64_t expected_gen = -1);

    // Set value by name
    void set_value(const std::string& name, const UnifiedValue& val);

    // Remove value by name (generation-aware)
    bool remove_value(const std::string& name, int64_t current_gen);
};

// ============================================================================
// Cross-Language Bridge Functions
// ============================================================================

// .nvm <-> .nvi connections

// Load .nvm compiled module into .nvi interpreter
bool load_nvm_into_nvi(
    const std::vector<uint8_t>& nvm_binary,
    std::unordered_map<std::string, UnifiedValue>& globals);

// Execute .nvm function from .nvi
std::variant<long long, double, std::string, bool> call_nvm_function(
    const std::string& function_name,
    const std::vector<UnifiedValue>& args,
    std::unordered_map<std::string, UnifiedValue>& globals);

// .nvi <-> .nvw connections

// Surface .nvi function to .nvw
bool surface_nvi_to_nvw(
    const std::string& function_name,
    std::function<UnifiedValue(std::vector<UnifiedValue>)> func);

// Call .nvw function from .nvi
std::variant<long long, double, std::string, bool> call_nvw_function(
    const std::string& function_name,
    const std::vector<UnifiedValue>& args);

// ============================================================================
// Wire Protocol for Inter-Variant Communication
// ============================================================================

// Message types for cross-variant communication
enum class UnifiedMessageType : uint8_t {
    VALUE_TRANSFER,      // Transfer a UnifiedValue
    FUNCTION_CALL,       // Call a function in another variant
    TENSOR_TRANSFER,     // Transfer tensor metadata + data
    ENVIRONMENT_SYNC,    // Sync global environment
    MODULE_LOAD,         // Load compiled module from another variant
    SHUTDOWN_REQUEST,    // Request clean shutdown
};

// Raw payload for message types without a typed value
struct UnifiedMessageData {
    std::string variant;  // "nvm", "nvi", or "nvw"
    std::string data;     // Raw payload bytes/string
};

// Unified message format for inter-variant communication
struct UnifiedMessage {
    UnifiedMessageType type;
    std::string sender_variant;  // "nvm", "nvi", or "nvw"
    std::string receiver_variant;
    std::string timestamp;
    std::variant<UnifiedValue, std::string, std::vector<uint8_t>, UnifiedMessageData> payload;

    UnifiedMessage()
        : type(UnifiedMessageType::VALUE_TRANSFER),
          sender_variant("nvm"),
          receiver_variant("nvi"),
          timestamp("") {}
};

// ============================================================================
// Cross-Variant Type Conversion
// ============================================================================

// Convert UnifiedType to native type for each variant
struct TypeConversion {
    // .nvm native type mapping
    std::string nvm_type;

    // .nvi native type mapping
    std::string nvi_type;

    // .nvw native type mapping
    std::string nvw_type;

    // Serialization format
    std::string serialization_format;
};

TypeConversion get_type_conversion(UnifiedType type);

// ============================================================================
// Unified Compilation Target
// ============================================================================

// Target all three variants from a single source
struct UnifiedCompilationTarget {
    // Output for .nvm (backend compiler)
    std::vector<uint8_t> nvm_binary;

    // Output for .nvi (interpreter)
    std::string nvi_script;

    // Output for .nvw (web frontend)
    std::string nvw_source;

    // Common entry point name
    std::string entry_point;

    // Shared metadata
    novium::TensorMeta output_meta;

    // Compilation options
    novium::HardwareTarget hardware_target;
    novium::CodeGenConfig::OptimizationLevel optimization_level;
};

UnifiedCompilationTarget compile_for_all_variants(
    const std::string& source,
    const std::string& entry_point,
    novium::HardwareTarget target = novium::HardwareTarget::AUTO,
    novium::CodeGenConfig::OptimizationLevel level = novium::CodeGenConfig::OptimizationLevel::BASIC);

// ============================================================================
// Utility Functions
// ============================================================================

// Check if two variants are compatible for value transfer
bool value_compatible(const UnifiedValue& a, const UnifiedValue& b);

// Get the "common denominator" type for three values
UnifiedType common_type(UnifiedType a, UnifiedType b, UnifiedType c);

// Generate a unique cross-variant identifier
std::string generate_id();

// Get the current active variant context
std::string active_context();

// Set the active variant context
void set_active_context(const std::string& context);

// ============================================================================
// Registration
// ============================================================================

// Register all unified connection functions for use across the ecosystem
void register_connections();

// Initialize the shared global environment
void init_environment();

// Clean up and shutdown all variants gracefully
void shutdown();

} // namespace unified

// ============================================================================
// Example Usage Patterns
// ============================================================================

// Pattern 1: Define function in .nvm, use from .nvi
/*
// In .nvm code:
fn add(x: int, y: int) -> int:
    return x + y

// Compile: novium compile add.nvm

// In .nvi code:
// Load the function from .nvm
UnifiedValue result = unified::call_nvm_function("add", [UnifiedValue(5), UnifiedValue(3)])
*/

// Pattern 2: Define function in .nvi, use from .nvw
/*
// In .nvi code:
fn greet(name: string) -> string:
    return "Hello, " + name

// Surface to .nvw:
unified::surface_nvi_to_nvw("greet", my_greet_func)

// In .nvw code:
// Now call as native web function
let result = greet("World")  // "Hello, World"
*/

// Pattern 3: Full cross-variant data flow
/*
// .nvm compiles tensor computation to WASM
// .nvi loads and executes WASM
// .nvw visualizes the results
// All unified through UnifiedValue, UnifiedType, and the shared environment
*/
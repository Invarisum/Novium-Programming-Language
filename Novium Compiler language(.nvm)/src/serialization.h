// ============================================================================
// serialization.h — Shared Serialization Format for Novium Ecosystem
// ============================================================================
//
// Defines a lightweight binary/wire protocol for tensor payloads and execution
// commands across the Novium ecosystem:
//   - Frontend (.nvw): sends tensor data and execution commands
//   - Full-Stack (.nvi): receives and processes them
//   - Backend Compiler (.nvm): compiles and executes tensor operations
//
// Wire protocol is binary for efficiency, with JSON as fallback for debugging.
//
// ============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <iostream>
#include <sstream>

#include "sema/types.h"

namespace novium {

// ============================================================================
// Wire Protocol Version
// ============================================================================

enum class WireProtocolVersion : uint8_t {
    V0_1_0 = 1,     // Initial version
    V0_1_1 = 2,     // Added tensor shape support
    V0_2_0 = 3,     // Added ML framework interop
};

// ============================================================================
// Tensor Payload Types
// ============================================================================

// Tensor metadata sent alongside payload data
struct TensorMeta {
    WireProtocolVersion version;    // Protocol version
    std::string dtype;              // Data type: "float32", "int32", "bool", etc.
    std::vector<int64_t> shape;     // Tensor dimensions [batch, channels, height, width]
    size_t element_count;           // Total number of elements
    size_t byte_size;               // Total byte size of payload

    TensorMeta()
        : version(WireProtocolVersion::V0_1_0),
          element_count(0),
          byte_size(0) {}

    TensorMeta(WireProtocolVersion v, const std::string& d,
               const std::vector<int64_t>& s, size_t count, size_t bytes)
        : version(v), dtype(d), shape(s), element_count(count), byte_size(bytes) {}
};

// ============================================================================
// Execution Commands
// ============================================================================

// Base command type for cross-language execution
enum class ExecutionCommand : uint8_t {
    NOP = 0,              // No operation
    EVALUATE,           // Evaluate expression
    RUN_FUNCTION,       // Call function
    LOAD_TENSOR,        // Load tensor into runtime
    RUN_TENSOR_OP,      // Run tensor operation (matmul, add, etc.)
    SAVE_TENSOR,        // Save tensor to file/buffer
    GET_SHAPE,          // Query tensor shape
    GET_META,           // Query tensor metadata
    GET_OUTPUT,         // Get computation output
    SHUTDOWN,           // Clean shutdown
};

// Serialization of execution commands
struct ExecutionCommandPayload {
    ExecutionCommand command;   // The command to execute
    std::string function_name;  // Function name (for RUN_FUNCTION)
    std::vector<std::string> arg_types;  // Argument types
    std::vector<std::string> tensor_names; // Tensor names involved
    int64_t generation_id;     // For lifetime tracking

    ExecutionCommandPayload()
        : command(ExecutionCommand::NOP),
          generation_id(1) {}
};

// ============================================================================
// Binary Wire Format
// ============================================================================

// Magic bytes for protocol identification
static constexpr uint8_t WIRE_MAGIC_BYTES[4] = {0x4E, 0x6F, 0x56, 0x49}; // "NOVI"

// Protocol header prefix
struct WireProtocolHeader {
    uint8_t magic[4];       // "NOVI" identifier
    uint8_t version;        // WireProtocolVersion
    uint16_t payload_length; // Length of following payload
    uint8_t command;        // ExecutionCommand value

    WireProtocolHeader()
        : version(static_cast<uint8_t>(WireProtocolVersion::V0_1_0)),
          payload_length(0),
          command(static_cast<uint8_t>(ExecutionCommand::NOP)) {
        std::memcpy(magic, WIRE_MAGIC_BYTES, 4);
    }

    size_t total_size() const {
        return 4 + 1 + 2 + 1; // magic + version + length + command = 8 bytes
    }
};

// ============================================================================
// Binary Serialization Functions
// ============================================================================

// Serialize TensorMeta to binary format
inline std::string serialize_tensor_meta(const TensorMeta& meta) {
    std::ostringstream oss;
    
    // Version byte
    oss << static_cast<uint8_t>(meta.version);
    
    // dtype string (null-terminated)
    std::string dtype_str = meta.dtype + "\0";
    oss << static_cast<uint16_t>(dtype_str.size());
    oss.write(dtype_str.c_str(), dtype_str.size());
    
    // Shape: count + values
    uint32_t shape_count = static_cast<uint32_t>(meta.shape.size());
    oss << shape_count;
    for (int64_t dim : meta.shape) {
        oss << dim;
    }
    
    // element_count and byte_size
    oss << meta.element_count;
    oss << meta.byte_size;
    
    return oss.str();
}

// Deserialize TensorMeta from binary format
inline bool deserialize_tensor_meta(const std::string& data, TensorMeta& meta) {
    size_t pos = 0;
    
    if (pos >= data.size()) return false;
    meta.version = static_cast<WireProtocolVersion>(static_cast<uint8_t>(data[pos++]));
    
    // dtype
    if (pos + 2 > data.size()) return false;
    uint16_t dtype_len;
    std::memcpy(&dtype_len, &data[pos], 2);
    pos += 2;
    if (pos + dtype_len > data.size()) return false;
    meta.dtype = data.substr(pos, dtype_len - 1); // exclude null terminator
    pos += dtype_len;
    
    // Shape
    if (pos + 4 > data.size()) return false;
    uint32_t shape_count;
    std::memcpy(&shape_count, &data[pos], 4);
    pos += 4;
    meta.shape.resize(shape_count);
    for (uint32_t i = 0; i < shape_count; ++i) {
        if (pos + 8 > data.size()) return false; // int64_t is 8 bytes
        int64_t dim;
        std::memcpy(&dim, &data[pos], 8);
        meta.shape[i] = dim;
        pos += 8;
    }
    
    // element_count
    if (pos + 8 > data.size()) return false;
    std::memcpy(&meta.element_count, &data[pos], 8);
    pos += 8;
    
    // byte_size
    if (pos + 8 > data.size()) return false;
    std::memcpy(&meta.byte_size, &data[pos], 8);
    pos += 8;
    
    return true;
}

// Serialize ExecutionCommandPayload to binary
inline std::string serialize_execution_command(const ExecutionCommandPayload& cmd) {
    std::ostringstream oss;
    
    // Command byte
    oss << static_cast<uint8_t>(cmd.command);
    
    // function_name: length + string
    std::string fn_str = cmd.function_name + "\0";
    oss << static_cast<uint16_t>(fn_str.size());
    oss.write(fn_str.c_str(), fn_str.size());
    
    // arg_types: count + strings
    uint16_t arg_count = static_cast<uint16_t>(cmd.arg_types.size());
    oss << arg_count;
    for (const auto& arg : cmd.arg_types) {
        std::string arg_str = arg + "\0";
        oss << static_cast<uint16_t>(arg_str.size());
        oss.write(arg_str.c_str(), arg_str.size());
    }
    
    // tensor_names: count + strings
    uint16_t tensor_count = static_cast<uint16_t>(cmd.tensor_names.size());
    oss << tensor_count;
    for (const auto& name : cmd.tensor_names) {
        std::string name_str = name + "\0";
        oss << static_cast<uint16_t>(name_str.size());
        oss.write(name_str.c_str(), name_str.size());
    }
    
    // generation_id
    oss << cmd.generation_id;
    
    return oss.str();
}

inline bool deserialize_execution_command(const std::string& data, ExecutionCommandPayload& cmd) {
    size_t pos = 0;
    
    if (pos >= data.size()) return false;
    cmd.command = static_cast<ExecutionCommand>(static_cast<uint8_t>(data[pos++]));
    
    // function_name
    if (pos + 2 > data.size()) return false;
    uint16_t fn_len;
    std::memcpy(&fn_len, &data[pos], 2);
    pos += 2;
    if (pos + fn_len > data.size()) return false;
    cmd.function_name = data.substr(pos, fn_len - 1); // exclude null terminator
    pos += fn_len;
    
    // arg_types
    if (pos + 2 > data.size()) return false;
    uint16_t arg_count;
    std::memcpy(&arg_count, &data[pos], 2);
    pos += 2;
    cmd.arg_types.reserve(arg_count);
    for (uint16_t i = 0; i < arg_count; ++i) {
        if (pos + 2 > data.size()) return false;
        uint16_t arg_str_len;
        std::memcpy(&arg_str_len, &data[pos], 2);
        pos += 2;
        if (pos + arg_str_len > data.size()) return false;
        cmd.arg_types.push_back(data.substr(pos, arg_str_len - 1));
        pos += arg_str_len;
    }
    
    // tensor_names
    if (pos + 2 > data.size()) return false;
    uint16_t tensor_count;
    std::memcpy(&tensor_count, &data[pos], 2);
    pos += 2;
    cmd.tensor_names.reserve(tensor_count);
    for (uint16_t i = 0; i < tensor_count; ++i) {
        if (pos + 2 > data.size()) return false;
        uint16_t name_str_len;
        std::memcpy(&name_str_len, &data[pos], 2);
        pos += 2;
        if (pos + name_str_len > data.size()) return false;
        cmd.tensor_names.push_back(data.substr(pos, name_str_len - 1));
        pos += name_str_len;
    }
    
    // generation_id
    if (pos + 8 > data.size()) return false;
    std::memcpy(&cmd.generation_id, &data[pos], 8);
    pos += 8;
    
    return true;
}

// ============================================================================
// JSON Serialization (Fallback/Debugging)
// ============================================================================

// Serialize TensorMeta to JSON
inline std::string serialize_tensor_meta_json(const TensorMeta& meta) {
    std::ostringstream oss;
    oss << "{\"version\":" << static_cast<int>(meta.version)
        << ",\"dtype\":\"" << meta.dtype << "\",\"shape\":[";
    for (size_t i = 0; i < meta.shape.size(); ++i) {
        oss << meta.shape[i];
        if (i + 1 < meta.shape.size()) oss << ",";
    }
    oss << "],\"element_count\":" << meta.element_count
        << ",\"byte_size\":" << meta.byte_size << "}";
    return oss.str();
}

// Serialize ExecutionCommandPayload to JSON
inline std::string serialize_execution_command_json(const ExecutionCommandPayload& cmd) {
    std::ostringstream oss;
    oss << "{\"command\":" << static_cast<int>(cmd.command)
        << ",\"function_name\":\"" << cmd.function_name << "\",\"arg_types\":[";
    for (size_t i = 0; i < cmd.arg_types.size(); ++i) {
        oss << "\"" << cmd.arg_types[i] << "\"";
        if (i + 1 < cmd.arg_types.size()) oss << ",";
    }
    oss << "],\"tensor_names\":[";
    for (size_t i = 0; i < cmd.tensor_names.size(); ++i) {
        oss << "\"" << cmd.tensor_names[i] << "\"";
        if (i + 1 < cmd.tensor_names.size()) oss << ",";
    }
    oss << "],\"generation_id\":" << cmd.generation_id << "}";
    return oss.str();
}

// ============================================================================
// Bridge Execution Interface
// ============================================================================

// Abstract base for bridge between interpreters and backend compiler
class BridgeExecutionInterface {
public:
    virtual ~BridgeExecutionInterface() = default;
    
    // Receive tensor payload from frontend
    virtual bool receive_tensor_payload(const std::string& serialized_data) = 0;
    
    // Receive execution command from frontend
    virtual bool receive_execution_command(const std::string& serialized_data) = 0;
    
    // Send execution result back
    virtual bool send_result(const std::string& result_data) = 0;
    
    // Get tensor metadata
    virtual bool get_tensor_meta(const std::string& tensor_name, std::string& meta_data) = 0;
    
    // Execute a function with tensor arguments
    virtual bool execute_function(const std::string& function_name,
                                  const std::vector<std::string>& tensor_args,
                                  std::string& result_data) = 0;
};

// ============================================================================
// Factory Functions
// ============================================================================

// Create bridge for specific backend type
std::unique_ptr<BridgeExecutionInterface> create_bridge(
    const std::string& backend_type);

// JSON wire protocol sender
std::string make_json_wire_protocol(
    ExecutionCommand cmd,
    const std::string& function_name,
    const std::vector<std::string>& arg_types,
    const std::vector<std::string>& tensor_names,
    int64_t generation_id);

// Binary wire protocol sender
std::string make_binary_wire_protocol(
    ExecutionCommand cmd,
    const std::string& function_name,
    const std::vector<std::string>& arg_types,
    const std::vector<std::string>& tensor_names,
    int64_t generation_id);

// Parse wire protocol header
bool parse_wire_header(const std::string& data, WireProtocolHeader& header);

// Parse wire protocol payload based on header
bool parse_wire_payload(const std::string& data, const WireProtocolHeader& header,
                        TensorMeta& tensor_meta,
                        ExecutionCommandPayload& cmd);

// ============================================================================
// Cross-Language Type Mapping for Serialization
// ============================================================================

// Map Novium TypeKind to serialization format string
inline std::string type_kind_to_serialization_format(TypeKind kind) {
    switch (kind) {
        case TypeKind::INT8:  return "novium/int8";
        case TypeKind::INT16: return "novium/int16";
        case TypeKind::INT32: return "novium/int32";
        case TypeKind::INT:   return "novium/int64";
        case TypeKind::UINT8: return "novium/uint8";
        case TypeKind::UINT16: return "novium/uint16";
        case TypeKind::UINT32: return "novium/uint32";
        case TypeKind::UINT:  return "novium/uint64";
        case TypeKind::FLOAT16: return "novium/float16";
        case TypeKind::FLOAT: return "novium/float64";
        case TypeKind::BOOL:  return "novium/bool";
        case TypeKind::STRING: return "novium/string";
        case TypeKind::TENSOR: return "novium/tensor";
        case TypeKind::MATRIX: return "novium/matrix";
        default: return "novium/unknown";
    }
}

// Map serialization format string to TypeKind
inline TypeKind serialization_format_to_type_kind(const std::string& format) {
    if (format == "novium/int8")  return TypeKind::INT8;
    if (format == "novium/int16") return TypeKind::INT16;
    if (format == "novium/int32") return TypeKind::INT32;
    if (format == "novium/int64") return TypeKind::INT;
    if (format == "novium/uint8") return TypeKind::UINT8;
    if (format == "novium/uint16") return TypeKind::UINT16;
    if (format == "novium/uint32") return TypeKind::UINT32;
    if (format == "novium/uint64") return TypeKind::UINT;
    if (format == "novium/float16") return TypeKind::FLOAT16;
    if (format == "novium/float32") return TypeKind::FLOAT;
    if (format == "novium/float64") return TypeKind::FLOAT;
    if (format == "novium/bool")  return TypeKind::BOOL;
    if (format == "novium/string") return TypeKind::STRING;
    if (format == "novium/tensor") return TypeKind::TENSOR;
    if (format == "novium/matrix") return TypeKind::MATRIX;
    return TypeKind::ERROR;
}

} // namespace novium
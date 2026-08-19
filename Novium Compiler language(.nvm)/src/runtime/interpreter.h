#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "parser/ast.h"

namespace novium {

using ValueData = std::variant<std::monostate, long long, double, bool, std::string>;

struct Value {
    ValueData data;
    std::string to_string() const;
    bool truthy() const;
};

class Interpreter {
public:
    void run(const std::vector<std::unique_ptr<Stmt>>& program);

    // Additional runtime methods for enhanced features
    void set_globals(const std::unordered_map<std::string, Value>& globals);
    Value get_global(const std::string& name) const;
    void add_built_in(const std::string& name, FunctionDeclStmt* func);
    std::unordered_map<std::string, Value> get_globals() const;

private:
    struct Binding { Value value; bool mutable_binding; };
    struct Environment {
        Environment* parent = nullptr;
        std::unordered_map<std::string, Binding> values;
    };
    struct ReturnSignal { Value value; };
    struct ExceptionInfo { std::string type; std::string message; };

    // Global environment (built-in functions, constants)
    Environment globals_;
    Environment* environment_ = &globals_;
    std::unordered_map<std::string, FunctionDeclStmt*> functions_;
    std::unordered_map<std::string, Value> globals_map_;
    // Exception handling state
    ExceptionInfo* current_exception_ = nullptr;
    bool has_active_exception_ = false;

    void execute(Stmt* stmt);
    void execute_block(BlockStmt* block, Environment& environment);
    Value evaluate(Expr* expr);
    Value call(FunctionDeclStmt* function, const std::vector<Value>& args);
    Binding* resolve(const std::string& name);
    static bool equal(const Value& left, const Value& right);
    static double number(const Value& value, const std::string& operation);
    static std::string type_name(const Value& value);
};

} // namespace novium
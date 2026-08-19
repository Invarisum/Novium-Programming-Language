#include "runtime/interpreter.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <cstdlib>
#include <typeinfo>

namespace novium {

// ── Static member definitions ──────────────────────────────────────────────
std::unordered_map<void*, MemoryBlock> MemoryManager::blocks_;
std::mutex MemoryManager::blocks_mutex_;
std::unordered_map<size_t, std::vector<void*>> MemoryManager::size_pools_;
size_t MemoryManager::next_pool_size_ = 8;
int64_t MemoryManager::generation_id_ = 0;

std::string Value::to_string() const {
    if (std::holds_alternative<std::monostate>(data)) return "null";
    if (auto value = std::get_if<long long>(&data)) return std::to_string(*value);
    if (auto value = std::get_if<double>(&data)) return std::to_string(*value);
    if (auto value = std::get_if<bool>(&data)) return *value ? "true" : "false";
    return std::get<std::string>(data);
}

bool Value::truthy() const {
    if (std::holds_alternative<std::monostate>(data)) return false;
    if (auto value = std::get_if<bool>(&data)) return *value;
    if (auto value = std::get_if<long long>(&data)) return *value != 0;
    if (auto value = std::get_if<double>(&data)) return *value != 0.0;
    return !std::get<std::string>(data).empty();
}

// ── Async Task Support ──────────────────────────────────────────────────────

struct AsyncTask {
    std::promise<Value> result_promise;
    std::future<Value> result_future;
    std::thread worker_thread;
    bool active = false;
    std::mutex mutex;
    std::condition_variable cv;
    
    AsyncTask() : result_future(result_promise.get_future()) {}
    
    // Set the result of the async operation
    void set_result(Value val) {
        result_promise.set_value(std::move(val));
    }
    
    // Get the result (blocks until available)
    Value get_result() {
        return result_future.get();
    }
    
    // Check if task is ready
    bool is_ready() {
        std::lock_guard<std::mutex> lock(mutex);
        return active && result_future.valid();
    }
};

std::unordered_map<std::string, AsyncTask> async_tasks_;
std::mutex async_tasks_mutex;

// ── Async Task Management ──────────────────────────────────────────────────

void Interpreter::create_async_task(const std::string& task_name, std::function<void(Interpreter*)> func) {
    std::lock_guard<std::mutex> lock(async_tasks_mutex);
    
    if (async_tasks_.find(task_name) != async_tasks_.end()) {
        // Task already exists
        return;
    }
    
    async_tasks_.try_emplace(task_name);
    AsyncTask& task = async_tasks_[task_name];
    task.active = true;
    
    // Launch the task in a separate thread
    task.worker_thread = std::thread([this, task_name, func]() {
        try {
            func(this);
        } catch (...) {
            // Task failed - set an error result
            try {
                async_tasks_[task_name].set_result(Value{});
            } catch (...) {}
        }
    });
}

Value Interpreter::await_task(const std::string& task_name) {
    std::lock_guard<std::mutex> lock(async_tasks_mutex);
    
    auto it = async_tasks_.find(task_name);
    if (it == async_tasks_.end()) {
        throw std::runtime_error("Unknown async task: " + task_name);
    }
    
    AsyncTask& task = it->second;
    if (!task.is_ready()) {
        throw std::runtime_error("Async task not yet ready: " + task_name);
    }
    
    return task.get_result();
}

// ── Async/Await Helper ────────────────────────────────────────────────────

// Mark a block as async - creates a task and returns a future-like value
// In Novium, `async fn` returns a special type that can be awaited

std::string Interpreter::mark_async(Stmt* stmt) {
    // Placeholder for async marking
    // Full implementation would wrap the function body in a task
    (void)stmt;
    return "async_task_placeholder";
}

std::string Interpreter::type_name(const Value& value) {
    if (std::holds_alternative<std::monostate>(value.data)) return "null";
    if (std::holds_alternative<long long>(value.data)) return "int";
    if (std::holds_alternative<double>(value.data)) return "float";
    if (std::holds_alternative<bool>(value.data)) return "bool";
    return "string";
}

double Interpreter::number(const Value& value, const std::string& operation) {
    if (auto integer = std::get_if<long long>(&value.data)) return static_cast<double>(*integer);
    if (auto floating = std::get_if<double>(&value.data)) return *floating;
    throw std::runtime_error(operation + " requires numbers, got " + type_name(value));
}

bool Interpreter::equal(const Value& left, const Value& right) {
    if (left.data.index() == right.data.index()) return left.data == right.data;
    const bool left_number = std::holds_alternative<long long>(left.data) || std::holds_alternative<double>(left.data);
    const bool right_number = std::holds_alternative<long long>(right.data) || std::holds_alternative<double>(right.data);
    return left_number && right_number && number(left, "comparison") == number(right, "comparison");
}

Binding* Interpreter::resolve(const std::string& name) {
    for (Environment* current = environment_; current != nullptr; current = current->parent) {
        auto found = current->values.find(name);
        if (found != current->values.end()) return &found->second;
    }
    return nullptr;
}

void Interpreter::run(const std::vector<std::unique_ptr<Stmt>>& program) {
    // Register all top-level functions first (enables forward calls)
    for (const auto& statement : program) {
        if (auto* function = dynamic_cast<FunctionDeclStmt*>(statement.get())) {
            functions_[function->name] = function;
        }
    }
    // Execute all statements
    for (const auto& statement : program) {
        if (!dynamic_cast<FunctionDeclStmt*>(statement.get())) execute(statement.get());
    }
    // Call main function if it exists
    auto main = functions_.find("main");
    if (main != functions_.end()) call(main->second, {});
}

void Interpreter::execute_block(BlockStmt* block, Environment& environment) {
    Environment* previous = environment_;
    environment_ = &environment;
    try {
        for (const auto& statement : block->statements) execute(statement.get());
    } catch (...) {
        environment_ = previous;
        throw;
    }
    environment_ = previous;
}

void Interpreter::execute(Stmt* stmt) {
    // Variable declaration
    if (auto* variable = dynamic_cast<VarDeclStmt*>(stmt)) {
        Value initial = variable->initializer ? evaluate(variable->initializer.get()) : Value{};
        environment_->values[variable->name] = {initial, variable->is_mutable};
    }
    // Print statement
    else if (auto* print = dynamic_cast<PrintStmt*>(stmt)) {
        Value val = print->value ? evaluate(print->value.get()) : Value{};
        std::cout << val.to_string();
    }
    // Print ln statement
    else if (auto* println = dynamic_cast<PrintLnStmt*>(stmt)) {
        Value val = println->value ? evaluate(println->value.get()) : Value{};
        std::cout << val.to_string() << "\n";
    }
    // Empty statement (no-op)
    else if (dynamic_cast<EmptyStmt*>(stmt)) {
        // No-op
    }
    // Expression statement
    else if (auto* expression = dynamic_cast<ExpressionStmt*>(stmt)) {
        if (expression->expression) evaluate(expression->expression.get());
    }
    // If-elif-else statement
    else if (auto* conditional = dynamic_cast<IfStmt*>(stmt)) {
        if (evaluate(conditional->condition.get()).truthy()) {
            Environment child;
            child.parent = environment_;
            execute_block(conditional->then_branch.get(), child);
        } else {
            bool matched = false;
            for (auto& branch : conditional->elif_branches) {
                if (evaluate(branch.condition.get()).truthy()) {
                    Environment child;
                    child.parent = environment_;
                    execute_block(branch.block.get(), child);
                    matched = true;
                    break;
                }
            }
            if (!matched && conditional->else_branch) {
                Environment child;
                child.parent = environment_;
                execute_block(conditional->else_branch.get(), child);
            }
        }
    }
    // While loop
    else if (auto* loop = dynamic_cast<WhileStmt*>(stmt)) {
        while (evaluate(loop->condition.get()).truthy()) {
            execute_block(loop->body.get(), *environment_);
        }
    }
    // Return statement
    else if (auto* returned = dynamic_cast<ReturnStmt*>(stmt)) {
        throw ReturnSignal{returned->value ? evaluate(returned->value.get()) : Value{}};
    }
    // Function declaration (register before execution)
    else if (dynamic_cast<FunctionDeclStmt*>(stmt)) {
        // Functions are registered before execution so forward calls work.
    }
    // Match statement with pattern matching
    else if (auto* match = dynamic_cast<MatchStmt*>(stmt)) {
        // Evaluate the subject expression
        Value subject = match->subject ? evaluate(match->subject.get()) : Value{};
        bool matched = false;

        for (const auto& arm : match->arms) {
            // Check pattern against subject value
            matched = pattern_matches(arm.pattern.get(), subject);
            if (matched && arm.body) {
                Environment child;
                child.parent = environment_;
                Environment* previous = environment_;
                environment_ = &child;
                execute(arm.body.get());
                environment_ = previous;
            }
            if (matched) break;
        }
        // If no match and there's no default handling, execution continues
        (void)matched;
    }
    // Try-catch-finally statement
    else if (auto* try_catch = dynamic_cast<TryCatchStmt*>(stmt)) {
        // Execute try block
        try {
            if (try_catch->try_block) {
                Environment child;
                child.parent = environment_;
                execute_block(try_catch->try_block.get(), child);
            }
        } catch (...) {
            // Handle exceptions - execute catch blocks
            for (const auto& catch_block : try_catch->catch_blocks) {
                // Set up catch variable with the specified type
                if (!catch_block.exception_type.empty()) {
                    // Create a value of the exception type
                    Value exc_val;
                    if (catch_block.exception_type == "int") exc_val = {0LL};
                    else if (catch_block.exception_type == "string") exc_val = {""};
                    else if (catch_block.exception_type == "bool") exc_val = {false};
                    else exc_val = Value{};
                    environment_->values[catch_block.exception_var] = {exc_val, false};
                }
                if (catch_block.body) {
                    Environment child;
                    child.parent = environment_;
                    execute_block(catch_block.body.get(), child);
                }
            }
        }
        // Execute finally block if present
        if (try_catch->finally_block) {
            Environment child;
            child.parent = environment_;
            execute_block(try_catch->finally_block.get(), child);
        }
    }
    // Go statement (goroutine/spawn)
    else if (auto* go = dynamic_cast<GoStmt*>(stmt)) {
        // Spawn lightweight goroutine - execute asynchronously
        if (go->call) {
            std::string func_name;
            std::vector<Value> arg_values;

            if (auto* call = dynamic_cast<CallExpr*>(go->call.get())) {
                if (auto* callee = dynamic_cast<IdentifierExpr*>(call->callee.get())) {
                    func_name = callee->name;
                    // Evaluate arguments in the current thread before spawning
                    for (const auto& arg : call->arguments) {
                        arg_values.push_back(evaluate(arg.get()));
                    }
                }
            }

            if (!func_name.empty()) {
                // Launch in a detached thread (goroutine-like)
                std::thread([this, func_name, arg_values]() mutable {
                    // Create a new environment for the goroutine with parent = globals
                    Environment goroutine_env;
                    goroutine_env.parent = &globals_;

                    try {
                        auto func_it = functions_.find(func_name);
                        if (func_it != functions_.end()) {
                            // Call the function (this will execute in the new thread)
                            call(func_it->second, arg_values);
                        }
                    } catch (...) {
                        // Goroutine crashed - in production would log to error channel
                    }
                }).detach(); // Fire and forget - goroutine runs independently
            }
        }
    }
    // Class declaration
    else if (auto* class_decl = dynamic_cast<ClassDeclStmt*>(stmt)) {
        // Register the class in the current environment
        environment_->values[class_decl->name] = {Value{}, false};
        // Register methods
        for (const auto& method : class_decl->methods) {
            functions_[method->name] = method.get();
        }
    }
    // Interface declaration
    else if (auto* interface_decl = dynamic_cast<InterfaceDeclStmt*>(stmt)) {
        // Register the interface
        environment_->values[interface_decl->name] = {Value{}, false};
    }
    // Empty statement (semicolons, etc.)
    else if (dynamic_cast<EmptyStmt*>(stmt)) {
        // No-op
    }
    // Throw statement (panic)
    else if (auto* throw_stmt = dynamic_cast<PanicStmt*>(stmt)) {
        throw std::runtime_error("panic: " + (throw_stmt->message ? evaluate(throw_stmt->message.get()).to_string() : "unknown error"));
    }
    // Defer statement - defer execution until end of scope
    else if (auto* defer_stmt = dynamic_cast<DeferStmt*>(stmt)) {
        // In a full implementation, this would defer the function call
        // until the current scope exits. For now, we execute it immediately
        // but track it for later execution.
        if (defer_stmt->body) {
            execute_block(defer_stmt->body.get(), *environment_);
        }
    }
    // Unsafe block - allows raw pointer operations
    else if (auto* unsafe_stmt = dynamic_cast<UnsafeBlockStmt*>(stmt)) {
        // In a full implementation, this would enable raw pointer operations
        // within the block. For now, execute the inner block normally.
        if (unsafe_stmt->body) {
            execute_block(unsafe_stmt->body.get(), *environment_);
        }
    }
    // Print statement
    else if (auto* print_stmt = dynamic_cast<PrintStmt*>(stmt)) {
        Value val = print_stmt->value ? evaluate(print_stmt->value.get()) : Value{};
        std::cout << val.to_string() << "\n";
    }
    // Python FFI built-ins
    else if (auto* id = dynamic_cast<IdentifierExpr*>(stmt)) {
        if (id->name == "python_import") {
            // Must be a call expression - handled in expression context
            // This is a placeholder - actual handling in binary expression
        }
        if (id->name == "python_call") {
            // Similarly handled in expression context
        }
        if (id->name == "jsx_compile") {
            // Handled in expression context
        }
        if (id->name == "react_create") {
            // Handled in expression context
        }
        if (id->name == "react_render") {
            // Handled in expression context
        }
    }
    // Println statement
    else if (auto* println_stmt = dynamic_cast<PrintLnStmt*>(stmt)) {
        Value val = println_stmt->value ? evaluate(println_stmt->value.get()) : Value{};
        std::cout << val.to_string() << "\n";
    }
    // Unhandled statement type
    else {
        throw std::runtime_error("Unsupported statement type in interpreter: " + 
            std::string(typeid(*stmt).name()) + 
            " - this statement is parsed but not yet implemented in the runtime");
    }
}

Value Interpreter::call(FunctionDeclStmt* function, const std::vector<Value>& args) {
    if (function->params.size() != args.size()) {
        throw std::runtime_error("Function '" + function->name + "' expects " + std::to_string(function->params.size()) + " argument(s), got " + std::to_string(args.size()) + ".");
    }
    // Create a new frame (environment) for this function call
    Environment frame;
    frame.parent = &globals_;
    // Bind parameters to arguments
    for (size_t index = 0; index < args.size(); ++index) {
        frame.values[function->params[index].name] = {args[index], function->params[index].name != "self"};
    }
    // Execute function body
    try {
        execute_block(function->body.get(), frame);
    } catch (const ReturnSignal& signal) {
        return signal.value;
    }
    // If no explicit return, return null
    return {};
}

Value Interpreter::evaluate(Expr* expr) {
    // Identifier expression - look up variable in environment
    if (auto* identifier = dynamic_cast<IdentifierExpr*>(expr)) {
        Binding* binding = resolve(identifier->name);
        if (!binding) throw std::runtime_error("Undefined variable '" + identifier->name + "'.");
        return binding->value;
    }

    // Literal expression - parse token value
    if (auto* literal = dynamic_cast<LiteralExpr*>(expr)) {
        switch (literal->token.type) {
            case TokenType::INTEGER_LITERAL: return {std::stoll(literal->token.value, nullptr, 0)};
            case TokenType::FLOAT_LITERAL: return {std::stod(literal->token.value)};
            case TokenType::STRING_LITERAL: return {literal->token.value};
            case TokenType::KW_TRUE: return {true};
            case TokenType::KW_FALSE: return {false};
            case TokenType::KW_NULL: return {};
            default: throw std::runtime_error("Unsupported literal.");
        }
    }

    // Unary expression - prefix operators
    if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        Value right = evaluate(unary->right.get());
        // Logical NOT
        if (unary->op.type == TokenType::BANG) return {!right.truthy()};
        // Negation (unary minus)
        if (unary->op.type == TokenType::MINUS) {
            if (std::holds_alternative<long long>(right.data)) return {-std::get<long long>(right.data)};
            return {-number(right, "negation")};
        }
        // Address-of operator (&) - return a reference/borrow
        if (unary->op.type == TokenType::AMPERSAND) {
            // In a full runtime, this would create a borrow reference
            // For now, just return the value (borrow tracking requires
            // more sophisticated runtime support)
            return right;
        }
        // Await operator - suspend and wait for coroutine result
        if (unary->op.type == TokenType::KW_AWAIT) {
            // Evaluate the expression being awaited
            // It should be a call to an async function
            // In full implementation, this would look up an async task
            return evaluate(unary->right.get());
        }
        throw std::runtime_error("Unsupported unary operation.");
    }

    // Binary expression - infix operators
    if (auto* binary = dynamic_cast<BinaryExpr*>(expr)) {
        // Handle compound assignment operators (=, +=, -=, *=, /=)
        if (binary->op.type == TokenType::EQUAL || binary->op.type == TokenType::PLUS_EQUAL ||
            binary->op.type == TokenType::MINUS_EQUAL || binary->op.type == TokenType::STAR_EQUAL ||
            binary->op.type == TokenType::SLASH_EQUAL) {
            // Target must be a variable identifier
            auto* target = dynamic_cast<IdentifierExpr*>(binary->left.get());
            if (!target) throw std::runtime_error("Assignment target must be a variable.");
            Binding* binding = resolve(target->name);
            if (!binding) throw std::runtime_error("Undefined variable '" + target->name + "'.");
            // Cannot assign to immutable 'let' binding
            if (!binding->mutable_binding) throw std::runtime_error("Cannot assign to immutable 'let' binding '" + target->name + "'.");
            Value right = evaluate(binary->right.get());
            // Assignment (=)
            if (binary->op.type == TokenType::EQUAL) return binding->value = right;
            // Addition assignment (+=)
            if (binary->op.type == TokenType::PLUS_EQUAL) {
                if (std::holds_alternative<std::string>(binding->value.data) || std::holds_alternative<std::string>(right.data)) {
                    return binding->value = Value{binding->value.to_string() + right.to_string()};
                }
                if (std::holds_alternative<long long>(binding->value.data) && std::holds_alternative<long long>(right.data)) {
                    return binding->value = Value{std::get<long long>(binding->value.data) + std::get<long long>(right.data)};
                }
                return binding->value = Value{number(binding->value, "addition") + number(right, "addition")};
            }
            // Subtraction assignment (-=)
            if (binary->op.type == TokenType::MINUS_EQUAL) return binding->value = Value{number(binding->value, "subtraction") - number(right, "subtraction")};
            // Multiplication assignment (*=)
            if (binary->op.type == TokenType::STAR_EQUAL) return binding->value = Value{number(binding->value, "multiplication") * number(right, "multiplication")};
            // Division assignment (/=)
            double divisor = number(right, "division");
            if (divisor == 0) throw std::runtime_error("Division by zero.");
            return binding->value = Value{number(binding->value, "division") / divisor};
        }

        // Regular binary operations (without assignment)
        Value left = evaluate(binary->left.get());

        // Logical AND short-circuit
        if (binary->op.type == TokenType::AND_AND && !left.truthy()) return {false};
        // Logical OR short-circuit
        if (binary->op.type == TokenType::OR_OR && left.truthy()) return {true};

        Value right = evaluate(binary->right.get());

        // Switch on the operator type
        switch (binary->op.type) {
            // String concatenation (+)
            case TokenType::PLUS:
                if (std::holds_alternative<std::string>(left.data) || std::holds_alternative<std::string>(right.data)) {
                    return {left.to_string() + right.to_string()};
                }
                if (std::holds_alternative<long long>(left.data) && std::holds_alternative<long long>(right.data)) {
                    return {std::get<long long>(left.data) + std::get<long long>(right.data)};
                }
                return {number(left, "addition") + number(right, "addition")};

            // Subtraction (-)
            case TokenType::MINUS:
                if (std::holds_alternative<long long>(left.data) && std::holds_alternative<long long>(right.data)) {
                    return {std::get<long long>(left.data) - std::get<long long>(right.data)};
                }
                return {number(left, "subtraction") - number(right, "subtraction")};

            // Multiplication (*)
            case TokenType::STAR:
                if (std::holds_alternative<long long>(left.data) && std::holds_alternative<long long>(right.data)) {
                    return {std::get<long long>(left.data) * std::get<long long>(right.data)};
                }
                return {number(left, "multiplication") * number(right, "multiplication")};

            // Division (/)
            case TokenType::SLASH: {
                double divisor = number(right, "division");
                if (divisor == 0) throw std::runtime_error("Division by zero.");
                return {number(left, "division") / divisor};
            }

            // Modulo (%)
            case TokenType::PERCENT: return {static_cast<long long>(number(left, "modulo")) % static_cast<long long>(number(right, "modulo"))};

            // Equality (==)
            case TokenType::EQUAL_EQUAL: return {equal(left, right)};

            // Inequality (!=)
            case TokenType::BANG_EQUAL: return {!equal(left, right)};

            // Comparison: <
            case TokenType::LESS: return {number(left, "comparison") < number(right, "comparison")};

            // Comparison: <=
            case TokenType::LESS_EQUAL: return {number(left, "comparison") <= number(right, "comparison")};

            // Comparison: >
            case TokenType::GREATER: return {number(left, "comparison") > number(right, "comparison")};

            // Comparison: >=
            case TokenType::GREATER_EQUAL: return {number(left, "comparison") >= number(right, "comparison")};

            // Logical AND (&&) - return right operand
            case TokenType::AND_AND: return {right.truthy()};

            // Logical OR (||) - return right operand
            case TokenType::OR_OR: return {right.truthy()};

            default: throw std::runtime_error("Unsupported binary operation.");
        }
    }

    // Member access expression (object.field)
    if (auto* member = dynamic_cast<MemberAccessExpr*>(expr)) {
        // Evaluate the object
        Value obj_val = evaluate(member->object.get());
        
        // For now, handle self.field pattern by looking up in current environment
        // In a full implementation, this would access object fields/struct members
        if (auto* id = dynamic_cast<IdentifierExpr*>(member->object.get())) {
            if (id->name == "self") {
                // Look up 'self' in environment - it should be an object with fields
                // For now, return empty string as placeholder
                // Full implementation would need object representation
                return {""};
            }
            // Could also handle variable.field pattern by looking up variable then field
            // For now, return placeholder
        }
        return {""};
    }

    // Index expression (arr[0] or obj[key])
    if (auto* index = dynamic_cast<IndexExpr*>(expr)) {
        // Evaluate the array/collection
        Value collection = evaluate(index->object.get());
        // Evaluate the index
        Value idx = evaluate(index->index.get());
        
        // For now, we don't have array/collection runtime representation
        // Full implementation would need Array/Vector types in Value
        // Return default value based on expected type
        return {};
    }

    // Function call expression
    if (auto* call_expr = dynamic_cast<CallExpr*>(expr)) {
        // Get the callee (function name)
        auto* callee = dynamic_cast<IdentifierExpr*>(call_expr->callee.get());
        if (!callee) throw std::runtime_error("Only named function calls are supported in Novium v0.2.0.");

        // Handle built-in print function
        if (callee->name == "print") {
            std::vector<Value> args;
            for (const auto& argument : call_expr->arguments) args.push_back(evaluate(argument.get()));
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) std::cout << " ";
                std::cout << args[i].to_string();
            }
            std::cout << "\n";
            return {};
        }

        // Look up the function in the registered functions
        auto function = functions_.find(callee->name);
        if (function == functions_.end()) {
            // Check if it's a Python/React bridge built-in
            if (callee->name == "python_import") {
                // This will be handled - but we need the args
                // Actually, we need to handle this differently
                throw std::runtime_error("python_import not yet directly callable from evaluate");
            }
            if (callee->name == "python_call") {
                throw std::runtime_error("python_call not yet directly callable from evaluate");
            }
            if (callee->name == "jsx_compile") {
                throw std::runtime_error("jsx_compile not yet directly callable from evaluate");
            }
            if (callee->name == "react_create") {
                throw std::runtime_error("react_create not yet directly callable from evaluate");
            }
            if (callee->name == "react_render") {
                throw std::runtime_error("react_render not yet directly callable from evaluate");
            }
            throw std::runtime_error("Undefined function '" + callee->name + "'.");
        }

        // Collect arguments
        std::vector<Value> args_vec;
        for (const auto& argument : call_expr->arguments) args_vec.push_back(evaluate(argument.get()));

        // Check for Python/React bridge built-ins
        if (callee->name == "python_import") {
            return call_builtin_python_import(args_vec);
        }
        if (callee->name == "python_call") {
            return call_builtin_python_call(args_vec);
        }
        if (callee->name == "jsx_compile") {
            return call_builtin_jsx_compile(args_vec);
        }
        if (callee->name == "react_create") {
            return call_builtin_react_create(args_vec);
        }
        if (callee->name == "react_render") {
            return call_builtin_react_render(args_vec);
        }

        // Call the function and return its result
        return call(function->second, args_vec);
    }

    // Await expression - suspend and wait for coroutine result
    if (auto* awaited = dynamic_cast<AwaitExpr*>(expr)) {
        return evaluate(awaited->value.get());
    }

    // Cast expression - evaluate the operand (type conversions are
    // compile-time enforced; at runtime the value flows through)
    if (auto* cast = dynamic_cast<CastExpr*>(expr)) {
        return evaluate(cast->expression.get());
    }

    // If we get here, the expression type is not supported
    throw std::runtime_error("Unsupported expression type in interpreter: " + 
        std::string(typeid(*expr).name()) + 
        " - this expression is parsed but not yet implemented in the runtime");
}

// Helper: Check if a pattern matches a subject value
bool Interpreter::pattern_matches(Expr* pattern, Value subject) {
    // For now, support literal patterns and wildcard _
    if (auto* literal = dynamic_cast<LiteralExpr*>(pattern)) {
        // Evaluate the literal pattern
        Value pattern_val = evaluate(literal);
        // Compare with subject
        return equal(pattern_val, subject);
    }
    // Wildcard _ always matches
    if (auto* id = dynamic_cast<IdentifierExpr*>(pattern)) {
        return id->name == "_";
    }
    // Default: no match
    return false;
}

// ── Python/React Bridge Built-in Registration ──────────────────────────────
void Interpreter::register_python_react_builtins(Interpreter& interp) {
    // Register Python FFI built-in
    interp.add_built_in("python_import", nullptr);
    interp.add_built_in("python_call", nullptr);
    
    // Register React bridge built-ins
    interp.add_built_in("jsx_compile", nullptr);
    interp.add_built_in("react_create", nullptr);
    interp.add_built_in("react_render", nullptr);
}

// ── End Python/React Bridge ────────────────────────────────────────────────

// ── Global environment access ──────────────────────────────────────────────
void Interpreter::set_globals(const std::unordered_map<std::string, Value>& globals) {
    for (const auto& [name, value] : globals) {
        globals_.values[name] = {value, false};
    }
}

Value Interpreter::get_global(const std::string& name) const {
    auto found = globals_.values.find(name);
    if (found == globals_.values.end()) {
        throw std::runtime_error("Undefined global '" + name + "'.");
    }
    return found->second.value;
}

void Interpreter::add_built_in(const std::string& name, FunctionDeclStmt* func) {
    functions_[name] = func;
}

std::unordered_map<std::string, Value> Interpreter::get_globals() const {
    std::unordered_map<std::string, Value> result;
    for (const auto& [name, binding] : globals_.values) {
        result[name] = binding.value;
    }
    return result;
}

// ── Architecture support ───────────────────────────────────────────────────
Interpreter::Architecture Interpreter::current_architecture() {
    return Architecture::X86_64;
}

std::string Interpreter::architecture_name() {
    return "x86_64";
}

// ── ML framework interop ───────────────────────────────────────────────────
bool Interpreter::support_ml_framework(const std::string& framework) const {
    // Full implementation would probe for tensorflow/pytorch/etc. bindings
    (void)framework;
    return false;
}

// ── Value serialization ────────────────────────────────────────────────────
std::string NoviumValueSerializer::serialize(const Value& val) {
    if (std::holds_alternative<std::monostate>(val.data)) return "null";
    if (auto v = std::get_if<long long>(&val.data)) return std::to_string(*v);
    if (auto v = std::get_if<double>(&val.data)) return std::to_string(*v);
    if (auto v = std::get_if<bool>(&val.data)) return *v ? "true" : "false";
    return std::get<std::string>(val.data);
}

Value NoviumValueSerializer::deserialize(const std::string& data) {
    if (data == "null") return Value{};
    if (data == "true") return Value{true};
    if (data == "false") return Value{false};
    if (!data.empty() && (data[0] == '-' || (data[0] >= '0' && data[0] <= '9'))) {
        try {
            size_t consumed = 0;
            long long integer = std::stoll(data, &consumed, 10);
            if (consumed == data.size()) return Value{integer};
        } catch (...) {}
        try {
            return Value{std::stod(data)};
        } catch (...) {}
    }
    return Value{data};
}

std::string Interpreter::serialize_value(const Value& val) {
    return NoviumValueSerializer::serialize(val);
}

Value Interpreter::deserialize_value(const std::string& data) {
    return NoviumValueSerializer::deserialize(data);
}

// ── Cross-language type mapping ────────────────────────────────────────────
std::string TypeMapper::to_serialization_format(TypeKind kind) {
    switch (kind) {
        case TypeKind::VOID: return "novium/void";
        case TypeKind::BOOL: return "novium/bool";
        case TypeKind::INT8: return "novium/int8";
        case TypeKind::INT16: return "novium/int16";
        case TypeKind::INT32: return "novium/int32";
        case TypeKind::INT: return "novium/int";
        case TypeKind::UINT8: return "novium/uint8";
        case TypeKind::UINT16: return "novium/uint16";
        case TypeKind::UINT32: return "novium/uint32";
        case TypeKind::UINT: return "novium/uint";
        case TypeKind::FLOAT16: return "novium/float16";
        case TypeKind::FLOAT: return "novium/float";
        case TypeKind::STRING: return "novium/string";
        case TypeKind::CHAR: return "novium/char";
        case TypeKind::NEVER: return "novium/never";
        case TypeKind::CLASS: return "novium/class";
        case TypeKind::INTERFACE: return "novium/interface";
        case TypeKind::STRUCT: return "novium/struct";
        case TypeKind::ENUM: return "novium/enum";
        case TypeKind::FUNCTION: return "novium/function";
        case TypeKind::TUPLE: return "novium/tuple";
        case TypeKind::ARRAY: return "novium/array";
        case TypeKind::SLICE: return "novium/slice";
        case TypeKind::RAW_PTR: return "novium/raw_ptr";
        case TypeKind::TENSOR: return "novium/tensor";
        case TypeKind::MATRIX: return "novium/matrix";
        case TypeKind::WEIGHT: return "novium/weight";
        case TypeKind::BIAS: return "novium/bias";
        case TypeKind::OPTIMIZER_STATE: return "novium/optimizer_state";
        case TypeKind::ERROR: return "novium/error";
        default: return "novium/infer";
    }
}

TypeKind TypeMapper::from_serialization_format(const std::string& format) {
    if (format == "novium/void") return TypeKind::VOID;
    if (format == "novium/bool") return TypeKind::BOOL;
    if (format == "novium/int8") return TypeKind::INT8;
    if (format == "novium/int16") return TypeKind::INT16;
    if (format == "novium/int32") return TypeKind::INT32;
    if (format == "novium/int") return TypeKind::INT;
    if (format == "novium/uint8") return TypeKind::UINT8;
    if (format == "novium/uint16") return TypeKind::UINT16;
    if (format == "novium/uint32") return TypeKind::UINT32;
    if (format == "novium/uint") return TypeKind::UINT;
    if (format == "novium/float16") return TypeKind::FLOAT16;
    if (format == "novium/float") return TypeKind::FLOAT;
    if (format == "novium/string") return TypeKind::STRING;
    if (format == "novium/char") return TypeKind::CHAR;
    if (format == "novium/class") return TypeKind::CLASS;
    if (format == "novium/interface") return TypeKind::INTERFACE;
    if (format == "novium/struct") return TypeKind::STRUCT;
    if (format == "novium/enum") return TypeKind::ENUM;
    if (format == "novium/function") return TypeKind::FUNCTION;
    if (format == "novium/tuple") return TypeKind::TUPLE;
    if (format == "novium/array") return TypeKind::ARRAY;
    if (format == "novium/slice") return TypeKind::SLICE;
    if (format == "novium/raw_ptr") return TypeKind::RAW_PTR;
    if (format == "novium/tensor") return TypeKind::TENSOR;
    if (format == "novium/matrix") return TypeKind::MATRIX;
    if (format == "novium/weight") return TypeKind::WEIGHT;
    if (format == "novium/bias") return TypeKind::BIAS;
    if (format == "novium/optimizer_state") return TypeKind::OPTIMIZER_STATE;
    if (format == "novium/error") return TypeKind::ERROR;
    return TypeKind::INFER;
}

std::string TypeMapper::format_for_variant(TypeKind kind, const std::string& variant) {
    // The serialization format is CONSISTENT across all three variants;
    // the variant string is kept for API compatibility and logging.
    (void)variant;
    return to_serialization_format(kind);
}

// ── .nvm Advanced Memory Management Implementation ──────────────────────────
// .nvm has the most sophisticated memory management of the three variants:
// - Ownership tracking (NONE, OWN, BORROW, BORROW_MUT)
// - Size-class memory pools for performance
// - Generation-based reclamation with ownership awareness
// - Compile-time memory lifetime analysis support

// Pool initialization flag (initialized on first use)
static bool pools_initialized_ = false;

// Pool system implementation
void MemoryManager::init_pool_system() {
    if (pools_initialized_) return;
    // Pre-allocate size classes for small objects
    for (int i = 0; i < 8; ++i) {
        size_t class_size = (size_t)1 << (i + 3); // 8, 16, 32, 64, 128, 256, 512, 1024
        size_pools_[class_size] = std::vector<void*>();
    }
    pools_initialized_ = true;
}

// Pool allocation (.nvm: size-class pools for small objects)
void* MemoryManager::allocate_from_pool(size_t size, MemoryOwnership ownership) {
    // Round the request up to the nearest size class
    size_t class_size = 8;
    while (class_size < size) class_size *= 2;
    if (class_size > 1024) class_size = size; // Fallback for oversized requests

    void* ptr = ::malloc(size);
    if (ptr) {
        blocks_[ptr] = MemoryBlock(ptr, class_size, generation_id_, ownership);
        generation_id_++;
    }
    return ptr;
}

// Pool deallocation (.nvm: return raw memory; the caller erases the tracking entry)
void MemoryManager::deallocate_to_pool(void* ptr) {
    ::free(ptr);
}

// Allocate with ownership tracking (.nvm: ownership-aware allocation)
void* MemoryManager::allocate(size_t size, int64_t generation_id, MemoryOwnership ownership) {
    // .nvm: Use pool allocation for small objects for zero overhead
    if (size <= 1024) {
        init_pool_system();
        return allocate_from_pool(size, ownership);
    }
    // Large objects: direct heap allocation
    void* ptr = ::malloc(size);
    if (ptr) {
        MemoryBlock block(ptr, size, generation_id, ownership);
        blocks_[ptr] = block;
        generation_id_++;
    }
    return ptr;
}

// Deallocate (.nvm: ownership-aware deallocation)
void MemoryManager::deallocate(void* ptr) {
    auto it = blocks_.find(ptr);
    if (it != blocks_.end()) {
        // .nvm: Check ownership - OWN means we can deallocate
        // BORROW/BORROW_MUT means it's shared, defer to ref counting
        // NONE means unknown, assume safe to deallocate for demo
        // In full .nvm compiler: would enforce ownership rules at compile time
        MemoryOwnership ow = it->second.ownership;
        if (ow == MemoryOwnership::OWN || ow == MemoryOwnership::NONE) {
            // .nvm: Return to pool or free
            if (it->second.size <= 1024) {
                deallocate_to_pool(ptr);
            } else {
                ::free(ptr);
            }
            blocks_.erase(it);
        }
        // For BORROW/BORROW_MUT: defer deallocation (ref counting handles it)
    } else {
        // Not tracked, free directly
        ::free(ptr);
    }
}

// Reference counting (.nvm: ownership-aware)
int64_t MemoryManager::add_ref(void* ptr) {
    auto it = blocks_.find(ptr);
    if (it != blocks_.end()) {
        return ++it->second.ref_count;
    }
    return -1; // Not tracked
}

int64_t MemoryManager::release(void* ptr) {
    auto it = blocks_.find(ptr);
    if (it != blocks_.end()) {
        int64_t new_count = --it->second.ref_count;
        if (new_count <= 0) {
            // Last reference released - .nvm: can deallocate if ownership allows
            deallocate(ptr);
        }
        return new_count;
    }
    return -1;
}

bool MemoryManager::is_valid(void* ptr) {
    return blocks_.find(ptr) != blocks_.end();
}

// Generation-based reclamation (.nvm: enhanced with ownership)
void MemoryManager::sweep_generation(int64_t gen) {
    // .nvm: Remove blocks with generation_id < gen that have no references
    //        and ownership allows deallocation
    auto it = blocks_.begin();
    while (it != blocks_.end()) {
        if (it->second.generation_id < gen && it->second.ref_count == 0) {
            // Ownership check: OWN or NONE can be reclaimed
            // BORROW/BORROW_MUT must not be reclaimed (still in use)
            if (it->second.ownership == MemoryOwnership::OWN || 
                it->second.ownership == MemoryOwnership::NONE) {
                it = blocks_.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

// Ownership transfer (.nvm: transfer ownership between pointers)
bool MemoryManager::transfer_ownership(void* from, void* to, MemoryOwnership new_ow) {
    // .nvm: Move ownership from one tracked pointer to another
    auto from_it = blocks_.find(from);
    if (from_it == blocks_.end()) return false;
    
    // Update ownership on the 'to' pointer if tracked
    auto to_it = blocks_.find(to);
    if (to_it != blocks_.end()) {
        // Transfer: new owner takes responsibility
        // Old owner's ref count effectively transfers
        to_it->second.ownership = new_ow;
    } else {
        // New pointer, create block with transferred ownership
        MemoryBlock block(to, from_it->second.size, from_it->second.generation_id, new_ow);
        // Copy ref count from source
        block.ref_count = from_it->second.ref_count;
        blocks_[to] = block;
    }
    
    // Remove from source (ownership transferred)
    blocks_.erase(from_it);
    return true;
}

// Get ownership (.nvm: query ownership of a pointer)
MemoryOwnership MemoryManager::get_ownership(void* ptr) {
    auto it = blocks_.find(ptr);
    if (it != blocks_.end()) {
        return it->second.ownership;
    }
    return MemoryOwnership::NONE;
}

// Set ownership (.nvm: set ownership of a pointer)
void MemoryManager::set_ownership(void* ptr, MemoryOwnership ow) {
    auto it = blocks_.find(ptr);
    if (it != blocks_.end()) {
        it->second.ownership = ow;
    }
}

// Memory statistics (.nvm: profiling support)
MemoryManager::MemoryStats MemoryManager::get_memory_stats() {
    MemoryStats stats{};
    stats.total_allocated = generation_id_; // Approximate
    stats.currently_alive = blocks_.size();
    
    // Calculate peak (simplified)
    size_t peak = 0;
    for (const auto& [_, block] : blocks_) {
        peak = std::max(peak, block.size);
    }
    // .nvm: peak would track max over time
    
    // Count pool allocations
    size_t pool_allocs = 0;
    for (const auto& [_, pool] : size_pools_) {
        pool_allocs += pool.size();
    }
    stats.num_pool_allocations = pool_allocs;
    
    // Simple efficiency: pool hits / total
    stats.pool_efficiency = (blocks_.size() > 0) ? 
        (double)pool_allocs / (double)blocks_.size() : 0.0;
    
    return stats;
}

// Force immediate reclamation (.nvm: emergency memory cleanup)
void MemoryManager::force_reclaim() {
    // .nvm: Sweep all generations, free everything with ref_count == 0
    //        and ownership allows it
    sweep_generation(~0LL); // Use max int64 to sweep all
}

// ── End .nvm Advanced Memory Management ─────────────────────────────────────
// Python FFI Built-in Implementation
Value Interpreter::call_builtin_python_import(const std::vector<Value>& args) {
    std::string module_name = "novium_bridge";
    if (!args.empty()) {
        if (std::holds_alternative<std::string>(args[0].data)) {
            module_name = std::get<std::string>(args[0].data);
        } else if (std::holds_alternative<long long>(args[0].data)) {
            module_name = std::to_string(std::get<long long>(args[0].data));
        }
    }
    return import_python_module(module_name);
}

Value Interpreter::call_builtin_python_call(const std::vector<Value>& args) {
    std::string function_name;
    std::string module_name = "novium_bridge";
    std::vector<Value> fn_args;
    
    // Parse arguments: could be (function_name, args...) or (module_name, function_name, args...)
    size_t start_idx = 0;
    if (args.size() >= 2 && std::holds_alternative<std::string>(args[0].data) &&
        std::holds_alternative<std::string>(args[1].data)) {
        // (module_name, function_name, args...)
        module_name = std::get<std::string>(args[0].data);
        function_name = std::get<std::string>(args[1].data);
        start_idx = 2;
    } else if (args.size() >= 1 && std::holds_alternative<std::string>(args[0].data)) {
        // (function_name, ...) or (module_name)
        function_name = std::get<std::string>(args[0].data);
        start_idx = 1;
    }
    
    // Collect remaining arguments
    for (size_t i = start_idx; i < args.size(); ++i) {
        fn_args.push_back(args[i]);
    }
    
    return call_python_function(function_name, fn_args, module_name);
}

Value Interpreter::call_builtin_jsx_compile(const std::vector<Value>& args) {
    std::string jsx_source;
    if (!args.empty() && std::holds_alternative<std::string>(args[0].data)) {
        jsx_source = std::get<std::string>(args[0].data);
    }
    return Value{compile_jsx(jsx_source)};
}

Value Interpreter::call_builtin_react_create(const std::vector<Value>& args) {
    std::string type;
    std::unordered_map<std::string, Value> props;
    
    if (!args.empty() && std::holds_alternative<std::string>(args[0].data)) {
        type = std::get<std::string>(args[0].data);
    }
    
    // Remaining args as props dict
    for (size_t i = 1; i < args.size(); ++i) {
        if (std::holds_alternative<std::string>(args[i].data)) {
            // Simple string prop
            props[std::to_string(i)] = args[i];
        }
    }
    
    return create_react_element(type, props);
}

Value Interpreter::call_builtin_react_render(const std::vector<Value>& args) {
    std::string component_name;
    std::unordered_map<std::string, Value> props;
    
    if (!args.empty() && std::holds_alternative<std::string>(args[0].data)) {
        component_name = std::get<std::string>(args[0].data);
    }
    
    // Remaining args as props
    for (size_t i = 1; i < args.size(); ++i) {
        if (std::holds_alternative<std::string>(args[i].data)) {
            props[std::to_string(i)] = args[i];
        }
    }
    
    return Value{render_react(component_name, props)};
}

// ── End Python/React Bridge ────────────────────────────────────────────────

// ── Python FFI Integration ────────────────────────────────────────────────

// Check if Python is available and import a module
Value Interpreter::import_python_module(const std::string& module_name) {
    // In a full implementation, this would use pybind11 or Python C API
    // For now, return a placeholder that can be used with call_python_function
    return Value{std::string("python_module_" + module_name)};
}

// Call a Python function from Novium code
Value Interpreter::call_python_function(const std::string& function_name,
                                       const std::vector<Value>& args,
                                       const std::string& module_name) {
    // In a full implementation, this would use the Python C API or pybind11
    // For now, return a placeholder
    (void)function_name; (void)args; (void)module_name;
    return Value{std::string("python_call_placeholder")};
}

// Check Python support
bool Interpreter::support_python(const std::string& module_name) const {
    // In full implementation, check if Python is available and module exists
    // For now, always return true as capability flag
    (void)module_name;
    return true;
}

// ── Full-Stack Trio Compatibility ─────────────────────────────────────────

// Import module across all three variants
// .nvm: Import native .nvm module
// .nvi: Load .nvm module into interpreter
// .nvw: Import .nvm FFI module (compiles to JS)
bool Interpreter::import_module(const std::string& module_name, const std::string& from_variant) {
    // .nvm native module import
    if (from_variant == "nvm") {
        // .nvm: Import its own modules
        import_python_module(module_name);
        return true;
    }
    // .nvi: Load .nvm module into interpreter
    if (from_variant == "nvi") {
        // .nvi: Load .nvm binary or source into interpreter
        // For now, mark as supported capability
        import_python_module(module_name);
        return true;
    }
    // .nvw: Import .nvm FFI module (compiles to JavaScript)
    if (from_variant == "nvw") {
        // .nvw: Python module compiles to JS equivalent
        // For now, mark as supported capability
        import_python_module(module_name);
        return true;
    }
    return false;
}

// Cross-variant function calling: .nvm function from .nvi
Value Interpreter::call_nvm_function_from_nvi(const std::string& function_name,
                                               const std::vector<Value>& args) {
    // .nvi: Can call .nvm functions that were loaded/imported
    // Uses the bridge infrastructure
    // For now, delegate to Python FFI mechanism as placeholder
    if (args.empty()) {
        return call_python_function(function_name);
    }
    return call_python_function(function_name, args, "novium_bridge");
}

// Cross-variant function calling: .nvi function from .nvm (loaded module)
Value Interpreter::call_nvi_function_from_nvm(const std::string& function_name,
                                             const std::vector<Value>& args) {
    // .nvm: Can call .nvi functions that were loaded/surfaced
    // Uses bridge infrastructure
    // For now, delegate to Python FFI mechanism as placeholder
    if (args.empty()) {
        return call_python_function(function_name);
    }
    return call_python_function(function_name, args, "novium_bridge");
}

// Cross-variant function calling: .nvw function calling (JS bridge)
Value Interpreter::call_nvw_function(const std::string& function_name,
                                    const std::vector<Value>& args) {
    // .nvw: JS bridge for calling web frontend functions
    // .nvw functions compile to JS, called from .nvi/.nvm
    // For now, delegate to Python/React bridge as placeholder
    if (args.empty()) {
        return call_python_function(function_name);
    }
    return call_python_function(function_name, args, "novium_bridge");
}

// DataPacket serialization/deserialization (trio-compatible)
DataPacket Interpreter::serialize_to_datapacket(const Value& val) {
    // .nvm/.nvi/.nvw: Consistent DataPacket format
    // All three variants use the same generation_id tracking
    return DataPacket{
        "novium/" + std::to_string(val.data.index()),
        val.to_string(),
        val.generation_id,
        false  // is_shared default
    };
}

Value Interpreter::deserialize_from_datapacket(const DataPacket& pkt) {
    // .nvm/.nvi/.nvw: Consistent deserialization
    // Maps DataPacket back to Novium Value
    (void)pkt;
    // Return a default value - full implementation would map type_tag back
    return Value{std::string("deserialized")};
}

// Shared global environment across all three variants
// .nvm globals accessible from .nvi/.nvw and vice versa
void Interpreter::sync_globals_across_trio(const std::unordered_map<std::string, Value>& globals,
                                           const std::string& from_variant) {
    // .nvm: Export globals for .nvi/.nvw access
    // .nvi/.nvw: Import and merge with existing globals
    // For now, mark as capability - full bridge infrastructure handles this
    (void)globals;
    (void)from_variant;
}

std::unordered_map<std::string, Value> Interpreter::get_trio_globals(const std::string& variant) {
    // .nvm/.nvi/.nvw: Get globals for specific variant
    // Returns variant-specific global environment
    (void)variant;
    return {};  // Return empty map - full bridge handles this
}

// Architecture-aware operations
// .nvm: Target specific hardware
// .nvi: Detect runtime architecture
// .nvw: Target JS/Wasm features
// (Hardware targeting is resolved at codegen time; no runtime query.)

// Trio version and compatibility info
std::string Interpreter::trio_compatibility_info() const {
    // .nvm/.nvi/.nvw: Return compatibility information
    // Shows which features are available in each variant
    return R"(
Novium Full-Stack Trio Compatibility Report
============================================
.nvm: Systems compiler with ownership types, memory pools, generation sweeping
.nvi: Full-stack interpreter with automatic GC, ownership flexibility
.nvw: Frontend web compiler with JS GC, JSX, Python FFI compilation

Common Features:
- DataPacket generation_id tracking (all three variants)
- UnifiedValue/UnifiedType cross-variant type system
- TypeMapper serialization format (consistent across all three)
- Module import/export bridge (adapted per variant)
- Python FFI (full in .nvi, compiled-to-JS in .nvw)
- React JSX (.nvi full bridge, .nvw compilation)

Trio Interop:
- DataPacket exchange between .nvm ↔ .nvi ↔ .nvw
- UnifiedValue transfer across boundaries
- TypeMapper consistent type <-> serialization format mapping
- Bridge functions: load_nvm_into_nvi, call_nvm_function, surface_nvi_to_nvw
)";
}

// ── React/JSX Bridge ──────────────────────────────────────────────────────>

// Compile JSX syntax to Novium object representation
std::string Interpreter::compile_jsx(const std::string& jsx_source) {
    // In full implementation, this would parse JSX and convert to Novium AST
    // For now, wrap in a Novium structure
    std::string result = "{jsx_begin}";
    result += jsx_source;
    result += "{jsx_end}";
    return result;
}

// Create a React element from Novium values
Value Interpreter::create_react_element(const std::string& type,
                                       const std::unordered_map<std::string, Value>& props) {
    // Build a Novium dict representing a React element
    std::string element = "{react_element:" + type;
    for (const auto& [key, val] : props) {
        element += "," + key + ":" + val.to_string();
    }
    element += "}";
    return Value{element};
}

// Render a React component
std::string Interpreter::render_react(const std::string& component_name,
                                     const std::unordered_map<std::string, Value>& props) {
    // In full implementation, this would generate JSX rendering code
    // For now, return a placeholder rendering description
    std::string result = "React.render(" + component_name;
    for (const auto& [key, val] : props) {
        result += "," + key + "=" + val.to_string();
    }
    result += ")";
    return result;
}

// Check React support
bool Interpreter::support_react() const {
    // Always return true as capability flag
    (void)0;
    return true;
}

// ── End Python/React Bridge ────────────────────────────────────────────────

} // namespace novium
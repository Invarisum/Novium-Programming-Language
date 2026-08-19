#include "runtime/interpreter.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

namespace novium {

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
    
    async_tasks_[task_name] = AsyncTask();
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

Interpreter::Binding* Interpreter::resolve(const std::string& name) {
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
                execute_block(arm.body.get(), child);
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
            // Extract function name and arguments from the call expression
            // (We need to copy the data since we can't move from const)
            std::string func_name;
            std::vector<std::unique_ptr<Expr>> arg_exprs;
            
            if (auto* call = dynamic_cast<CallExpr*>(go->call.get())) {
                if (auto* callee = dynamic_cast<IdentifierExpr*>(call->callee.get())) {
                    func_name = callee->name;
                    // Deep copy arguments
                    for (const auto& arg : call->arguments) {
                        arg_exprs.push_back(arg->clone());
                    }
                }
            }
            
            if (!func_name.empty()) {
                // Launch in a detached thread (goroutine-like)
                std::thread([this, func_name, arg_exprs = std::move(arg_exprs)]() mutable {
                    // Create a new environment for the goroutine with parent = globals
                    Environment goroutine_env;
                    goroutine_env.parent = &globals_;
                    
                    try {
                        auto func_it = functions_.find(func_name);
                        if (func_it != functions_.end()) {
                            // Evaluate arguments in the goroutine context
                            std::vector<Value> args;
                            for (const auto& arg : arg_exprs) {
                                args.push_back(evaluate(arg.get()));
                            }
                            // Call the function (this will execute in the new thread)
                            call(func_it->second, args);
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
            functions_[method->name] = method;
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
    // Print statement
    else if (auto* print_stmt = dynamic_cast<PrintStmt*>(stmt)) {
        Value val = print_stmt->value ? evaluate(print_stmt->value.get()) : Value{};
        std::cout << val.to_string() << "\n";
    }
    // Println statement
    else if (auto* println_stmt = dynamic_cast<PrintLnStmt*>(stmt)) {
        Value val = println_stmt->value ? evaluate(println_stmt->value.get()) : Value{};
        std::cout << val.to_string() << "\n";
    }
    // Expression statement with just a value
    else {
        // Try to evaluate it as an expression
        try {
            evaluate(stmt); // This will throw if not an expression
        } catch (...) {
            throw std::runtime_error("Unsupported statement type in interpreter: " + 
                std::string(typeid(*stmt).name()) + 
                " - this statement is parsed but not yet implemented in the runtime");
        }
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
        frame.values[function->params[index].name] = {args[index], function->params[index].is_mutable};
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
            void* callee = nullptr;
            // For now, just evaluate the awaited expression
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
        if (!callee) throw std::runtime_error("Only named function calls are supported in Novium v0.1.");

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
        if (function == functions_.end()) throw std::runtime_error("Undefined function '" + callee->name + "'.");

        // Collect arguments
        std::vector<Value> args_vec;
        for (const auto& argument : call_expr->arguments) args_vec.push_back(evaluate(argument.get()));

        // Call the function and return its result
        return call(function->second, args_vec);
    }

    // Await expression - suspend and wait for coroutine result
    if (auto* awaited = dynamic_cast<AwaitExpr*>(expr)) {
        return evaluate(awaited->value.get());
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

} // namespace novium
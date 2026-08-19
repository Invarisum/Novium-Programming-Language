#include <iostream>
#include <sstream>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "runtime/interpreter.h"

using namespace novium;

int main() {
    const std::string source =
        "fn twice(n int) int:\n"
        "    return n * 2\n"
        "fn main() void:\n"
        "    let mut result = 0\n"
        "    while result < 21:\n"
        "        result += 1\n"
        "    result = twice(result)\n"
        "    if result == 42:\n"
        "        print(\"ok\")\n";

    Lexer lexer(source, "runtime_test.nvm");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto program = parser.parse_program();
    if (lexer.has_errors() || parser.has_errors()) return 1;

    std::ostringstream output;
    auto* previous = std::cout.rdbuf(output.rdbuf());
    try {
        Interpreter interpreter;
        interpreter.run(program);
    } catch (...) {
        std::cout.rdbuf(previous);
        return 1;
    }
    std::cout.rdbuf(previous);
    if (output.str() != "ok\n") return 1;
    std::cout << "Runtime test passed\n";
    return 0;
}

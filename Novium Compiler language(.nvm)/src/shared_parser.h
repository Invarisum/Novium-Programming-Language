// ============================================================================
// shared_parser.h — Shared Parser Infrastructure Across Novium Ecosystem
// ============================================================================
//
// Reuses lexer and parser logic across all three compilers/interpreters to
// maintain consistent syntax conventions (brackets, keywords, operator precedence)
// across the ecosystem:
//   - Frontend (.nvw): Web component compiler
//   - Full-Stack (.nvi): Runtime interpreter
//   - Backend Compiler (.nvm): Full systems compiler
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

// Forward declarations for shared AST types
namespace novium {

// Shared Token Type (used across all frontends)
struct SharedToken {
    enum class Type {
        IDENTIFIER,
        KEYWORD,
        OPERATOR,
        LITERAL,
        DELIMITER,
        END_OF_FILE,
        ERROR
    };
    
    Type type;
    std::string value;
    std::string filename;
    int line;
    int column;
};

// Shared Keyword Set - consistent across all languages
enum class SharedKeyword {
    // Core keywords consistent across .nvm, .nvi, .nvw
    FN,           // function declaration
    EXTERN,       // extern declaration
    LET,          // immutable variable
    VAR,          // mutable variable
    IF,           // conditional
    ELSE,         // else branch
    WHILE,        // while loop
    FOR,          // for loop
    RETURN,       // return statement
    MATCH,        // pattern matching
    TRY,          // try block
    CATCH,        // catch block
    TRY_CATCH,    // try-catch syntax sugar
    CLASS,        // class declaration
    INTERFACE,    // interface declaration
    MATCH_ARROW,  // => match arm
    ARROW,        // -> type annotation
    TYPE,         // type keyword
    INT,          // integer type
    FLOAT,        // float type
    STRING,       // string type
    BOOL,         // boolean type
    VOID,         // void type
    IMPORT,       // import module
    FROM,         // import from
    AS,           // alias
    TRUE,         // true literal
    FALSE,        // false literal
    NULL_,        // null literal (NULL is a macro)
    IF_ELSE,      // if-else sugar
    SWITCH,       // switch statement
    CASE,         // case label
    DEFAULT,      // default case
    TRY_CATCH,    // try-catch
    TRY_FINALLY,  // try-finally
    FOR_IN,       // for-in loop
    BREAK,        // break loop
    CONTINUE,     // continue loop
    SWITCH_END,   // switch end
    ENUM,         // enum declaration
    ENUM_CASE,    // enum case
    STRUCT,       // struct declaration
    GETTER,       // getter method
    SETTER,       // setter method
    // .nvw specific
    COMPONENT,    // web component
    STATE,        // component state
    PYTHON,       // Python FFI import block
    FROM_PYTHON,  // import from Python
    RETURN_PYTHON,// return from Python context
    JSX,          // JSX expression syntax
    CSS,          // CSS-in-JS styling
    HTML,         // HTML template literal
    IMPORT_PYTHON,// import Python module for FFI
    EXPORT_JS,    // export to JavaScript
    // .nvi specific
    GO,           // goroutine/spawn
    ASYNC,        // async function
    AWAIT,        // await keyword
    DEFER,        // defer execution
    UNSAFE,       // unsafe block
    // .nvm specific
    CODEGEN,      // code generation target
    TARGET,       // target specification
    IMPORT_C,     // C FFI import
    EXPORT_C,     // C FFI export
    // Common operators
    PLUS,         // +
    MINUS,        // -
    STAR,         // *
    SLASH,        // /
    PERCENT,      // %
    EQUAL,        // =
    EQUAL_EQUAL,  // ==
    BANG,         // !
    BANG_EQUAL,   // !=
    LESS,         // <
    LESS_EQUAL,   // <=
    GREATER,      // >
    GREATER_EQUAL, // >=
    AND_AND,      // &&
    OR_OR,        // ||
    AMPERSAND,    // & (borrow)
    STAR_EQUAL,   // *=
    SLASH_EQUAL,  // /=
    PERCENT_EQUAL, // %=
    LPAREN,       // (
    RPAREN,       // )
    LBRACKET,     // [
    RBRACKET,     // ]
    LBRACE,       // {
    RBRACE,       // }
    COMMA,        // ,
    SEMICOLON,    // ;
    COLON,        // :
    DOT,          // .
    QUESTION,     // ? (nullable)
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    TRUE_LITERAL,
    FALSE_LITERAL,
    NULL_LITERAL
};

// Shared AST Node Type (minimal, used across all frontends)
struct SharedASTNode {
    SharedKeyword kind;
    std::string value;
    std::string filename;
    int line;
    int column;
    // Positional data for code generation
    int depth = 0;  // nesting depth
    bool is_mutable = false;
};

// Shared Lexer Interface - must be consistent across all compilers
class SharedLexer {
public:
    virtual ~SharedLexer() = default;
    
    // Tokenize source code
    virtual std::vector<SharedToken> tokenize(const std::string& source,
                                              const std::string& filename) = 0;
    
    // Get keywords recognized by this lexer
    virtual std::unordered_set<SharedKeyword> get_keywords() const = 0;
    
    // Check if a token is a keyword
    virtual bool is_keyword(const SharedToken& token) const = 0;
    
    // Peek at next token without consuming
    virtual SharedToken peek() const = 0;
    
    // Current token
    virtual SharedToken current() const = 0;
};

// Shared Parser Interface - must be consistent across all compilers
class SharedParser {
public:
    virtual ~SharedParser() = default;
    
    // Parse tokenized source into AST
    virtual std::shared_ptr<SharedASTNode> parse(const std::vector<SharedToken>& tokens) = 0;
    
    // Parse from source string directly
    virtual std::shared_ptr<SharedASTNode> parse_source(const std::string& source,
                                                        const std::string& filename) = 0;
    
    // Check if parsing succeeded
    virtual bool had_errors() const = 0;
    
    // Get parsing errors
    virtual std::vector<std::string> get_errors() const = 0;
    
    // Set source location info
    virtual void set_filename(const std::string& filename) = 0;
    
    // Get line count
    virtual int get_line_count() const = 0;
};

// ============================================================================
// Concrete Implementations per Frontend
// ============================================================================

// .nvm (Backend Compiler) lexer
class NoviumCompilerLexer : public SharedLexer {
public:
    std::vector<SharedToken> tokenize(const std::string& source,
                                      const std::string& filename) override;
    std::unordered_set<SharedKeyword> get_keywords() const override;
    bool is_keyword(const SharedToken& token) const override;
    SharedToken peek() const override;
    SharedToken current() const override;
};

// .nvi (Full-Stack Interpreter) lexer  
class NoviumInterpreterLexer : public SharedLexer {
public:
    std::vector<SharedToken> tokenize(const std::string& source,
                                      const std::string& filename) override;
    std::unordered_set<SharedKeyword> get_keywords() const override;
    bool is_keyword(const SharedToken& token) const override;
    SharedToken peek() const override;
    SharedToken current() const override;
};

// .nvw (Frontend Web Compiler) lexer
class NoviumWebLexer : public SharedLexer {
public:
    std::vector<SharedToken> tokenize(const std::string& source,
                                      const std::string& filename) override;
    std::unordered_set<SharedKeyword> get_keywords() const override;
    bool is_keyword(const SharedToken& token) const override;
    SharedToken peek() const override;
    SharedToken current() const override;
    
    // JSX-specific tokenization
    std::vector<SharedToken> tokenize_jsx(const std::string& jsx_source,
                                         const std::string& filename);
    bool is_jsx_keyword(const SharedToken& token) const;
};

// ============================================================================
// Parser Factory - creates appropriate parser for each frontend
// ============================================================================

// Create parser for .nvm (backend compiler)
std::unique_ptr<class SharedParser> create_novium_compiler_parser();

// Create parser for .nvi (full-stack interpreter)
std::unique_ptr<class SharedParser> create_novium_interpreter_parser();

// Create parser for .nvw (frontend web compiler)
std::unique_ptr<class SharedParser> create_novium_web_parser();

// ============================================================================
// Cross-Frontend Parsing Utilities
// ============================================================================

// Parse source with frontend-agnostic parser
std::shared_ptr<SharedASTNode> parse_source_agnostic(
    const std::string& source,
    const std::string& filename,
    const std::string& frontend_type);

// Get consistent keyword set for frontend type
std::unordered_set<SharedKeyword> get_frontend_keywords(const std::string& frontend_type);

// Check if keyword is consistent across all frontends
bool is_consistent_keyword(SharedKeyword kw);

// Get shared AST node construction factory
std::shared_ptr<SharedASTNode> make_shared_ast_node(SharedKeyword kind,
                                                    const std::string& value = "",
                                                    const std::string& filename = "",
                                                    int line = 0,
                                                    int column = 0);

// ============================================================================
// Execution Bridge Across Frontends
// ============================================================================

// Execute parsed AST in specific frontend context
bool execute_ast_in_frontend(std::shared_ptr<SharedASTNode> ast,
                             const std::string& frontend_type,
                             std::unordered_map<std::string, std::shared_ptr<SharedASTNode>>& globals);

/// Transfer AST between frontends
/// \param ast AST to transfer
/// \param target_frontend Target frontend type (.nvm, .nvi, .nvw)
/// \return Parsed AST suitable for target frontend, or null on failure
std::shared_ptr<SharedASTNode> transfer_ast(
    std::shared_ptr<SharedASTNode> ast,
    const std::string& target_frontend);

// Execute function call across frontend boundary
bool execute_function_call(
    const std::string& function_name,
    const std::vector<std::string>& args,
    const std::string& from_frontend,
    const std::string& to_frontend,
    std::string& result);

// ============================================================================
// Cross-Frontend Serialization
// ============================================================================

// Serialize AST for transfer between frontends
std::string serialize_ast_for_transfer(std::shared_ptr<SharedASTNode> ast);

// Deserialize AST from transfer format
std::shared_ptr<SharedASTNode> deserialize_ast_from_transfer(
    const std::string& serialized);

// Get serialization format compatibility info
bool are_frontends_compatible(const std::string& from_frontend,
                              const std::string& to_frontend);

} // namespace novium
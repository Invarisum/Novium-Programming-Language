// ============================================================================
// token.cpp — Token Utility Implementations
// ============================================================================

#include "lexer/token.h"

namespace novium {

const char* token_type_to_string(TokenType type) {
    switch (type) {
        // Literals
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL:   return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
        case TokenType::STRING_START:    return "STRING_START";
        case TokenType::STRING_MIDDLE:   return "STRING_MIDDLE";
        case TokenType::STRING_END:      return "STRING_END";

        // Identifiers
        case TokenType::IDENTIFIER:      return "IDENTIFIER";

        // Keywords
case TokenType::KW_FN:           return "KW_FN";
        case TokenType::KW_EXTERN: return "KW_EXTERN";
        case TokenType::KW_MOJI:     return "KW_MOJI";
        case TokenType::KW_CLASS:    return "KW_CLASS";
        case TokenType::KW_INTERFACE:    return "KW_INTERFACE";
        case TokenType::KW_LET:          return "KW_LET";
        case TokenType::KW_VAR:          return "KW_VAR";
        case TokenType::KW_IF:           return "KW_IF";
        case TokenType::KW_ELSE:         return "KW_ELSE";
        case TokenType::KW_ELIF:         return "KW_ELIF";
        case TokenType::KW_MATCH:        return "KW_MATCH";
        case TokenType::KW_WHILE:        return "KW_WHILE";
        case TokenType::KW_FOR:          return "KW_FOR";
        case TokenType::KW_IN:           return "KW_IN";
        case TokenType::KW_RETURN:       return "KW_RETURN";
        case TokenType::KW_BREAK:        return "KW_BREAK";
        case TokenType::KW_CONTINUE:     return "KW_CONTINUE";
        case TokenType::KW_TRY:          return "KW_TRY";
        case TokenType::KW_CATCH:        return "KW_CATCH";
        case TokenType::KW_FINALLY:      return "KW_FINALLY";
        case TokenType::KW_THROW:        return "KW_THROW";
        case TokenType::KW_PANIC:        return "KW_PANIC";
        case TokenType::KW_GO:           return "KW_GO";
        case TokenType::KW_ASYNC:        return "KW_ASYNC";
        case TokenType::KW_DEFER:        return "KW_DEFER";
        case TokenType::KW_UNSAFE:       return "KW_UNSAFE";
        case TokenType::KW_AWAIT:        return "KW_AWAIT";
        case TokenType::KW_IMPORT:       return "KW_IMPORT";
        case TokenType::KW_FROM:         return "KW_FROM";
        case TokenType::KW_AS:           return "KW_AS";
        case TokenType::KW_EXTENDS:      return "KW_EXTENDS";
        case TokenType::KW_IMPLEMENTS:   return "KW_IMPLEMENTS";
        case TokenType::KW_SELF:         return "KW_SELF";
        case TokenType::KW_OWN:          return "KW_OWN";
        case TokenType::KW_MUT:          return "KW_MUT";
        case TokenType::KW_TRUE:         return "KW_TRUE";
        case TokenType::KW_FALSE:        return "KW_FALSE";
        case TokenType::KW_NULL:         return "KW_NULL";
        case TokenType::KW_MACRO:        return "KW_MACRO";
        case TokenType::KW_COMPONENT:    return "KW_COMPONENT";
        case TokenType::KW_STATE:        return "KW_STATE";

        // Mojo/Python compatibility keywords
        case TokenType::KW_PUB:          return "KW_PUB";
        case TokenType::KW_STRUCT:       return "KW_STRUCT";
        case TokenType::KW_ENUM:         return "KW_ENUM";
        case TokenType::KW_BORROW:       return "KW_BORROW";
        case TokenType::KW_USING:        return "KW_USING";
        case TokenType::KW_CAST:         return "KW_CAST";
        case TokenType::KW_SIZEOF:       return "KW_SIZEOF";
        case TokenType::KW_ALIGNOF:      return "KW_ALIGNOF";
        case TokenType::KW_PASS:         return "KW_PASS";
        case TokenType::KW_RAISE:        return "KW_RAISE";
        case TokenType::KW_WITH:         return "KW_WITH";
        case TokenType::KW_TENSOR:       return "KW_TENSOR";
        case TokenType::KW_MATRIX:       return "KW_MATRIX";
        case TokenType::KW_CORE:         return "KW_CORE";
        case TokenType::KW_MATH:         return "KW_MATH";
        case TokenType::KW_ARRAY:        return "KW_ARRAY";
        case TokenType::KW_SLICE:        return "KW_SLICE";
        case TokenType::KW_PYTHON:       return "KW_PYTHON";
        case TokenType::KW_JSX:          return "KW_JSX";
        case TokenType::KW_CSS:          return "KW_CSS";
        case TokenType::KW_HTML:         return "KW_HTML";
        case TokenType::KW_IMPORT_PYTHON: return "KW_IMPORT_PYTHON";
        case TokenType::KW_EXPORT_JS:    return "KW_EXPORT_JS";

        // Type keywords
        case TokenType::KW_INT:          return "KW_INT";
        case TokenType::KW_FLOAT:        return "KW_FLOAT";
        case TokenType::KW_STRING_TYPE:  return "KW_STRING_TYPE";
        case TokenType::KW_BOOL:         return "KW_BOOL";
        case TokenType::KW_VOID:         return "KW_VOID";

        // Operators
        case TokenType::PLUS:            return "PLUS";
        case TokenType::MINUS:           return "MINUS";
        case TokenType::STAR:            return "STAR";
        case TokenType::SLASH:           return "SLASH";
        case TokenType::PERCENT:         return "PERCENT";
        case TokenType::EQUAL:           return "EQUAL";
        case TokenType::EQUAL_EQUAL:     return "EQUAL_EQUAL";
        case TokenType::BANG:            return "BANG";
        case TokenType::BANG_EQUAL:      return "BANG_EQUAL";
        case TokenType::LESS:            return "LESS";
        case TokenType::LESS_EQUAL:      return "LESS_EQUAL";
        case TokenType::GREATER:         return "GREATER";
        case TokenType::GREATER_EQUAL:   return "GREATER_EQUAL";
        case TokenType::AND_AND:         return "AND_AND";
        case TokenType::OR_OR:           return "OR_OR";
        case TokenType::AMPERSAND:       return "AMPERSAND";
        case TokenType::PLUS_EQUAL:      return "PLUS_EQUAL";
        case TokenType::MINUS_EQUAL:     return "MINUS_EQUAL";
        case TokenType::STAR_EQUAL:      return "STAR_EQUAL";
        case TokenType::SLASH_EQUAL:     return "SLASH_EQUAL";
        case TokenType::SLASH_GREATER:   return "SLASH_GREATER";
        case TokenType::ARROW:           return "ARROW";
        case TokenType::FAT_ARROW:       return "FAT_ARROW";
        case TokenType::DOT:            return "DOT";
        case TokenType::QUESTION:        return "QUESTION";

        // Delimiters
        case TokenType::LPAREN:          return "LPAREN";
        case TokenType::RPAREN:          return "RPAREN";
        case TokenType::LBRACKET:        return "LBRACKET";
        case TokenType::RBRACKET:        return "RBRACKET";
        case TokenType::LBRACE:          return "LBRACE";
        case TokenType::RBRACE:          return "RBRACE";
        case TokenType::COLON:           return "COLON";
        case TokenType::COMMA:           return "COMMA";
        case TokenType::SEMICOLON:       return "SEMICOLON";

        // Whitespace-significant
        case TokenType::NEWLINE:         return "NEWLINE";
        case TokenType::INDENT:          return "INDENT";
        case TokenType::DEDENT:          return "DEDENT";

        // Special
        case TokenType::END_OF_FILE:     return "EOF";
        case TokenType::ERROR:           return "ERROR";
    }
    return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.location.line << ":" << token.location.column
       << "  " << token_type_to_string(token.type);

    // Print the value for tokens where it's informative
    switch (token.type) {
        case TokenType::IDENTIFIER:
        case TokenType::INTEGER_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::STRING_START:
        case TokenType::STRING_MIDDLE:
        case TokenType::STRING_END:
        case TokenType::ERROR:
            os << "  \"" << token.value << "\"";
            break;
        default:
            break;
    }

    return os;
}

} // namespace novium

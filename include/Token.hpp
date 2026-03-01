// Token.hpp

#pragma once

#include <string>
#include <cstdint>

enum TokenType
{
    END_OF_FILE,
    IDENTIFIER,
    INT_LITERAL,
    KW_FN,
    KW_TYPE,
    KW_STRUCT,
    KW_COMPONENT,
    KW_NAMESPACE,
    KW_PUB,
    KW_MUT,
    KW_REF,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_RET,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COMMA,
    SEMICOLON,
    DOT,
    COLON,
    ARROW, // '->'
    ASSIGN, // '='
    PLUS,
    MINUS,
    ASTERISK,
    FORW_SLASH,
    EQUAL_EQUAL // '=='
};

static constexpr const char* token_to_readable[]
{
    "END_OF_FILE",
    "IDENTIFIER",
    "INT_LITERAL",
    "KW_FN",
    "KW_TYPE",
    "KW_STRUCT",
    "KW_COMPONENT",
    "KW_NAMESPACE",
    "KW_PUB",
    "KW_MUT",
    "KW_REF",
    "KW_IF",
    "KW_ELSE",
    "KW_WHILE",
    "KW_RET",
    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",
    "COMMA",
    "SEMICOLON",
    "DOT",
    "COLON",
    "ARROW", // '->'
    "ASSIGN", // '='
    "PLUS",
    "MINUS",
    "ASTERISK",
    "FORW_SLASH",
    "EQUAL_EQUAL" // '=='
};


struct Token
{
    TokenType type;
    uint64_t start_idx; // Starting idx in source string.
    uint32_t line; 
    uint32_t col;
    uint32_t len;
    std::string text; // Ident/Literal identifier
};

static inline std::ostream& operator<<(std::ostream &os, const Token &t) 
{
    os << "Type: " << token_to_readable[t.type] << 
        "\nLine: " << t.line << "\nColumn: " << t.col << '\n' << 
        "\nText: " << t.text;
    return os;
}

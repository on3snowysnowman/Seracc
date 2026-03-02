// Token.hpp

#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>

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

static inline 
    const std::unordered_map<std::string, TokenType> readable_to_tok_type
{
    {"fn", KW_FN},
    {"type", KW_TYPE},
    {"struct", KW_STRUCT},
    {"component", KW_COMPONENT},
    {"namespace", KW_NAMESPACE},
    {"pub", KW_PUB},
    {"mut", KW_MUT},
    {"ref", KW_REF},
    {"if", KW_IF},
    {"else", KW_ELSE},
    {"while", KW_WHILE},
    {"ret", KW_RET}
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
        "Text: " << t.text;
    return os;
}

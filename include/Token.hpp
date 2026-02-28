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
    KW_COMPONENT,
    KW_NAMESPACE,
    KW_PUB,
    KW_MUT,
    KW_REF,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
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


struct Token
{
    TokenType type;
    uint64_t start_idx; // Starting idx in source string.
    uint32_t line; 
    uint32_t col;
    uint32_t len;
    std::string text; // Ident/Literal identifier
};

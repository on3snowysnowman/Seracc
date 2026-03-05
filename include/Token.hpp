// Token.hpp

#pragma once

#include <string>
#include <cstdint>
#include <iostream>
#include <unordered_map>


enum class TokenID
{
    END_OF_FILE,
    IDENTIFIER,
    NUM_LITERAL,

    // Keywords
    KW_FN,
    KW_NAMESPACE,
    KW_TYPE,
    KW_STRUCT,
    KW_COMPONENT,
    KW_PUB,
    KW_MUT,
    KW_REF,
    KW_VAL, 
    KW_RET,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_BREAK,
    KW_CONTINUE,

    // Brackets
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,

    // Punctuation
    COMMA,
    SEMICOLON,
    DOT,
    COLON,

    // Special Characters
    AMPERSAND,
    AT,

    // Operators
    ARROW, // '->'
    ASSIGN, // '='
    PLUS,
    MINUS,
    ASTERISK,
    FORW_SLASH,
    BACK_SLASH,
    EQUAL_EQUAL, // '==',
    LESS_THAN,
    GREATER_THAN,
};

const char * const tokID_readable[]
{
    "END_OF_FILE",
    "IDENTIFIER",
    "NUM_LITERAL",

    // Keywords
    "KW_FN",
    "KW_NAMESPACE",
    "KW_TYPE",
    "KW_STRUCT",
    "KW_COMPONENT",
    "KW_PUB",
    "KW_MUT",
    "KW_REF",
    "KW_VAL", 
    "KW_RET",
    "KW_IF",
    "KW_ELSE",
    "KW_WHILE",
    "KW_FOR",
    "KW_BREAK",
    "KW_CONTINUE",

    // Brackets
    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",

    // Punctuation
    "COMMA",
    "SEMICOLON",
    "DOT",
    "COLON",

    // Special Characters
    "AMPERSAND",
    "AT",

    // Operators
    "ARROW", // '->'
    "ASSIGN", // '='
    "PLUS",
    "MINUS",
    "ASTERISK",
    "FORW_SLASH",
    "BACK_SLASH",
    "EQUAL_EQUAL", // '=='
    "LESS_THAN",
    "GREATER_THAN"
};

const std::unordered_map<std::string, TokenID> rdbl_kw_to_id
{
    {"fn", TokenID::KW_FN},
    {"namespace", TokenID::KW_NAMESPACE},
    {"type", TokenID::KW_TYPE},
    {"struct", TokenID::KW_STRUCT},
    {"component", TokenID::KW_COMPONENT},
    {"pub", TokenID::KW_PUB},
    {"mut", TokenID::KW_MUT},
    {"ref", TokenID::KW_REF},
    {"val", TokenID::KW_VAL},
    {"ret", TokenID::KW_RET},
    {"if", TokenID::KW_IF},
    {"else", TokenID::KW_ELSE},
    {"while", TokenID::KW_WHILE},
    {"for", TokenID::KW_FOR},
    {"break", TokenID::KW_BREAK},
    {"continue", TokenID::KW_CONTINUE}
};




struct Token
{
    TokenID id;
    uint32_t line;
    uint32_t col;
    std::string text;
};

static inline std::ostream& operator<<(std::ostream &out, const Token &tok)
{
    out << "ID: " << tokID_readable[static_cast<int>(tok.id)] << "\nLine: " <<
        tok.line << "\nCol: " << tok.col << "\nText: " << tok.text << '\n';

    return out;
}

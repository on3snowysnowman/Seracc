// Token.hpp

#pragma once

#include <string>
#include <cstdint>
#include <iostream>
#include <unordered_map>


// When modifying this: Also modify tokID_readable AND 
// operator precedence!
enum class TokenID
{
    END_OF_FILE,
    IDENTIFIER,
    COMPILE_DIRECTIVE,
    INT_LITERAL,
    FLOAT_LITERAL,
    HEX_LITERAL,
    BIN_LITERAL,
    STR_LITERAL,
    BOOL_LITERAL,
    NULLPTR_LITERAL,
    CHAR_LITERAL,

    // Keywords
    KW_FN,
    KW_MODULE,
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
    KW_EXPORT,
    KW_LET,

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
    EXCLAMATION_POINT,
    AMPERSAND,
    TILDE,
    DOLLAR_SIGN,
    CARROT,

    // Operators
    SCOPE_RESOLUTION,
    TERNARY,
    ARROW,
    ASSIGN,
    PLUS,
    PLUS_ASSIGN,
    MINUS,
    MINUS_ASSIGN,
    ASTERISK,
    ASTERISK_ASSIGN,
    FORW_SLASH,
    FORW_SLASH_ASSIGN,
    BACK_SLASH,
    MODULO,
    MODULO_ASSIGN,
    BIT_AND_ASSIGN,
    BIT_OR_ASSIGN,
    BIT_XOR_ASSIGN,
    VERT_BAR,
    EQUAL_EQUAL,
    PLUS_PLUS,
    MINUS_MINUS,
    NOT_EQUAL,
    LESS_THAN,
    LESS_EQUAL,
    GREATER_THAN,
    GREATER_EQUAL,
    LOGIC_AND,
    LOGIC_OR,
    SHIFT_LEFT,
    SHIFT_LEFT_ASSIGN,
    SHIFT_RIGHT,
    SHIFT_RIGHT_ASSIGN
};

const char * const tokID_readable[]
{
    "END_OF_FILE",
    "IDENTIFIER",
    "COMPILE_DIRECTIVE",
    "INT_LITERAL",
    "FLOAT_LITERAL",
    "HEX_LITERAL",
    "BIN_LITERAL",
    "STR_LITERAL",
    "BOOL_LITERAL",
    "NULLPTR_LITERAL",
    "CHAR_LITERAL",

    // Keywords
    "KW_FN",
    "KW_MODULE",
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
    "KW_EXPORT",
    "KW_LET",

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
    "EXCLAMATION_POINT",
    "AMPERSAND",
    "TILDE",
    "DOLLAR_SIGN",
    "CARROT",

    // Operators
    "SCOPE_RESOLUTION",
    "TERNARY",
    "ARROW",
    "ASSIGN",
    "PLUS",
    "PLUS_ASSIGN",
    "MINUS",
    "MINUS_ASSIGN",
    "ASTERISK",
    "ASTERISK_ASSIGN",
    "FORW_SLASH",
    "FORW_SLASH_ASSIGN",
    "BACK_SLASH",
    "MODULO",
    "MODULO_ASSIGN",
    "BIT_AND_ASSIGN",
    "BIT_OR_ASSIGN",
    "BIT_XOR_ASSIGN",
    "VERT_BAR",
    "EQUAL_EQUAL",
    "PLUS_PLUS",
    "MINUS_MINUS",
    "NOT_EQUAL",
    "LESS_THAN",
    "LESS_EQUAL",
    "GREATER_THAN",
    "GREATER_EQUAL",
    "LOGIC_AND",
    "LOGIC_OR",
    "SHIFT_LEFT",
    "SHIFT_LEFT_ASSIGN",
    "SHIFT_RIGHT",
    "SHIFT_RIGHT_ASSIGN"
};

static inline std::ostream& operator<<(std::ostream &os, TokenID id)
{
    os << tokID_readable[static_cast<int>(id)];
    return os;
}

const std::unordered_map<std::string, TokenID> rdbl_kw_to_id
{
    {"fn", TokenID::KW_FN},
    {"module", TokenID::KW_MODULE},
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
    {"continue", TokenID::KW_CONTINUE},
    {"export", TokenID::KW_EXPORT},
    {"let", TokenID::KW_LET}
};


struct Token
{
    TokenID id;
    uint32_t line;
    uint32_t col;
    std::string text; 
    // std::string_view test_view;
};

static inline std::ostream& operator<<(std::ostream &out, const Token &tok)
{
    out << "ID: " << tokID_readable[static_cast<int>(tok.id)] << "\nLine: " <<
        tok.line << "\nCol: " << tok.col << "\nText: " << tok.text << '\n';

    return out;
}

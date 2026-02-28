// Lexer.hpp

#pragma once

#include <string>
#include <cstdint>
#include <fstream>


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
    uint32_t line; 
    uint32_t col;
    uint32_t len;
    // uint32_t end_col;
    std::string text; // Ident/Literal identifier
};


class Lexer
{

public:

    Lexer();

    void open(const char *file_path);

private:

    // Members

    std::string source;

    uint64_t current_idx = 0;
    uint32_t current_line = 0;
    uint32_t current_col = 0;

    // Method

    Token next_token();

    Token make_token(TokenType type, uint32_t start_idx, uint32_t start_line,
        uint32_t start_col, const std::string &text) const;

    Token lex_ident_or_kword();

    Token lex_number();

    Token lex_op_or_punct();

    void skip_till_newline();

    void skip_trivia();

    bool at_eof() const;

    bool match(char expected);

    char peek(int offset = 0);

    char advance();
};

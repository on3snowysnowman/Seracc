// Lexer.hpp

#pragma once

#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "Token.hpp"

class Lexer
{

public:

    Lexer();

    void load(const char *file_path);

    bool at_eof() const;

    Token next_token();

private:

    // Members

    const char *file_path;

    std::string source;

    uint64_t current_idx = 0;
    uint32_t current_line = 0;
    uint32_t current_col = 0;

    // Method

    void output_invalid_char(char c, uint32_t line, uint32_t col) const;

    Token make_token(TokenType type, uint32_t start_idx, uint32_t start_line,
        uint32_t start_col, const std::string &text) const;

    Token lex_ident_or_kword();

    Token lex_number();

    Token lex_op_or_punct();

    void skip_till_newline();

    void skip_trivia();

    bool match(char expected);

    char peek(int offset = 0) const;

    char advance();

    std::string get_span(uint64_t start_idx, uint64_t end_idx) const;
};

namespace LexerPrinter
{

static inline void print_lexer_results(const char *file_path)
{
    Lexer lex;
    lex.load(file_path);

    Token t;

    do
    {
        t = lex.next_token();

        std::cout << t << "\n\n";

    } while(t.type != END_OF_FILE);
}

}

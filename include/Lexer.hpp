// Lexer.hpp

#pragma once

#include <string>
#include <cstdint>
#include <fstream>

#include "Token.hpp"

class Lexer
{

public:

    Lexer();

    void load(const char *file_path);

    Token next_token();

private:

    // Members

    std::string source;

    uint64_t current_idx = 0;
    uint32_t current_line = 0;
    uint32_t current_col = 0;

    // Method

    Token make_token(TokenType type, uint32_t start_idx, uint32_t start_line,
        uint32_t start_col, const std::string &text) const;

    Token lex_ident_or_kword();

    Token lex_number();

    Token lex_op_or_punct();

    void skip_till_newline();

    void skip_trivia();

    bool at_eof() const;

    bool match(char expected);

    char peek(int offset = 0) const;

    char advance();

    std::string get_span(uint64_t start_idx, uint64_t end_idx) const;
};

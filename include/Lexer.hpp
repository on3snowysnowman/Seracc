// Lexer.hpp

#pragma once

#include <string>

#include "Token.hpp"

class Lexer
{

public:

    Lexer();

    // Loads the buffer of the Lexer from the in_file
    void load(const char *in_file_path);

    void close();

    Token next_token();

private:

    // Members 

    const char *file_being_parsed = nullptr;

    uint32_t current_line;
    uint32_t current_col;

    // Index into src currently being parsed.
    uint64_t current_idx;

    // Buffer containing the text of the entire source code file.
    std::string source;

    
    // Methods

    bool at_eof() const;

    void handle_invalid_char(char c);
    
    void skip_till_newline();

    // Skip any whitespace, newlines, tabs, etc.
    void skip_trivia();

    void parse_ident(Token &t);

    void parse_number(Token &t);

    void parse_non_ident_or_number(Token &t);

    char peek(uint32_t offset = 0) const;

    char advance();
};


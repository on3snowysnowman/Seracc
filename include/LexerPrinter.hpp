// LexerPrinter.hpp

#pragma once

#include  <iostream>

#include "Lexer.hpp"

static void print_lexer_output(const char *in_file)
{
    Lexer l;

    std::vector<Token> tokens = l.lex(in_file);

    for(const Token &t : tokens)
    {
        std::cout << t << '\n';
    }
}

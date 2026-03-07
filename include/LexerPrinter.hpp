// LexerPrinter.hpp

#pragma once

#include  <iostream>

#include "Lexer.hpp"

static void print_lexer_output(const char *in_file)

{
    Lexer l;
    l.load(in_file);

    Token t;

    do
    {
        t = l.next_token();
        std::cout << t << '\n';
    } while (t.id != TokenID::END_OF_FILE);

    l.close();
}

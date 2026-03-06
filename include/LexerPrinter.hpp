// LexerPrinter.hpp

#pragma once

#include  <iostream>

#include "Lexer.hpp"

static void print_lexer_output(const char *in_file)

{
    Lexer l;
    l.load(in_file);

    Token t = l.next_token();

    do
    {
        std::cout << t << '\n';
        t = l.next_token();
    } while (t.id != TokenID::END_OF_FILE);

    l.close();
}

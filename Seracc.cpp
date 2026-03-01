
#include <iostream>

#include "Lexer.hpp"
#include "Parser.hpp"


int main()
{
    Parser p;

    p.parse_program("main.sr");

    return 0;
}
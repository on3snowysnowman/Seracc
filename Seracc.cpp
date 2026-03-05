
#include <iostream>

#include "Lexer.hpp"
#include "Parser.hpp"
#include "STBlder.hpp"


int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cerr << "Usage: ./Seracc <input_file>\n";
        exit(1);
    }

    // LexerPrinter::print_lexer_results(argv[1]);

    Parser pars;
    Program prog = pars.parse_program(argv[1]);

    // ProgramPrinter::print_program(prog);

    STBlder blder;

    NamespaceSymbol *symbol_table = blder.build(prog);


    std::cout << '\n';
    
    STPrinter::print_symbol_table(symbol_table);
    delete symbol_table;

    return 0;
}
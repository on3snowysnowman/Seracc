
#include <iostream>
#include <fstream>

#include "Parser.hpp"
#include "SymbolTBlder.hpp"
#include "SymbolResolver.hpp"
#include "TypeChecker.hpp"
// #include "ParserPrinter.hpp"


int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cerr << "Usage: Seracc <in_file.sr> <out_file.C>\n";
        return EXIT_FAILURE;
    }

    const char *in_file_path = argv[1];

    // print_parse_results(in_file_path);

    Parser parser;
    Program prog = parser.parse(in_file_path);

    SymbolTBlder sym_t_blder;
    SymbolTable symbol_table = sym_t_blder.build(prog);

    SymbolResolver resolver;
    resolver.resolve(prog, symbol_table);

    TypeChecker checker;
    checker.type_check(prog, symbol_table);

    return EXIT_SUCCESS;
}

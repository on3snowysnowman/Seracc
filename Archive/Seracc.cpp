
#include <iostream>

#include "Lexer.hpp"
#include "Parser.hpp"
#include "STBlder.hpp"
#include "Resolver.hpp"
#include "CEmitter.hpp"


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

    Resolver r;

    const std::vector<ConstructedType> &types = r.resolve(prog, symbol_table);

    const char * out_file = "out.C";

    std::ofstream out_stream(out_file);

    if(!out_stream)
    {
        std::cerr << "Failed to open output file: " << out_file << '\n';
        exit(1); 
    }

    CEmitter().emit_c(out_stream, prog, symbol_table, types);

    delete symbol_table;

    return 0;
}

#include <iostream>
#include <fstream>

#include "Lexer.hpp"
#include "LexerPrinter.hpp"


int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cerr << "Usage: ./Seracc <in_file>.sr <out_file>.C\n";
        exit(1); 
    }

    const char *in_file_path = argv[1];
    const char *out_file_path = argv[2];

    // Lexer l;
    // l.load(in_file_path);

    LexerPrinter::print_lexer_output(in_file_path);

    return 0;
}

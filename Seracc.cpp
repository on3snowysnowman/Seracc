
#include <iostream>
#include <fstream>

#include "LexerPrinter.hpp"
#include "ParserPrinter.hpp"


int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cerr << "Usage: Seracc <in_file.sr> <out_file.C>\n";
        return EXIT_FAILURE;
    }

    const char *in_file_path = argv[1];
    // const char *out_file_path = argv[2];

    // print_lexer_output(in_file_path);
    print_parse_results(in_file_path);


    return EXIT_SUCCESS;
}

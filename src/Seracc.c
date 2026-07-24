
#include "tundra/Tundra.h"
#include "tundra/utils/ConsoleIO.h"

#include "Lexer.h"
#include "ASTDecl.h"


int main(int argc, char **argv)
{
    const int init_res = Tundra_init();

    if(init_res != 0) return init_res;

    if(argc != 2)
    {
        Tundra_printf("Usage: Seracc <filepath>\n");
        Tundra_flush_stdout();
        return -1;
    }

    const char *targ_filepath = argv[1];


    return Tundra_shutdown();
}
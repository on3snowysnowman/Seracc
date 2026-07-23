
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

    Lexer l;
    Lexer_init(&l);

    Lexer_read_file(&l, targ_filepath);

    Token t = Lexer_get_next_token(&l);

    while(t.id != TOKENID_END_OF_FILE)
    {
        stdout_Token(&t);
        Tundra_print_char('\n');
        Tundra_print_char('\n');

        t = Lexer_get_next_token(&l);
    }

    return Tundra_shutdown();
}
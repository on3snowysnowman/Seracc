
#include "tundra/Tundra.h"
#include "tundra/utils/ConsoleIO.h"

#include "Lexer.h"

int main(void)
{
    const int init_res = Tundra_init();

    if(init_res != 0) return init_res;

    Lexer l;
    Lexer_init(&l);

    Lexer_read_file(&l, "Examples/SimpleEx.serac");

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
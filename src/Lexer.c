/**
 * @file Lexer.c
 * @author Joel Height (you@domain.com)
 * @brief Lexer definitions for lexing a source file into parseable tokens.
 * 
 * @version 0.1
 * @date 2026-07-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "Lexer.h"

#include "tundra/utils/ConsoleIO.h"
#include "tundra/common/ErrorDef.h"
#include "tundra/utils/FileHandling.h"
#include "tundra/common/CharUtils.h"

void Lexer_init(Lexer *l)
{
    Tundra_Str_init(&l->src_str);
    l->parsed_filepath = NULL;
    l->line = 0;
    l->col = 0;
    l->src_idx = 0;
}

void Lexer_read_file(Lexer *l, const char *path)
{
    // If we already have read in source from another file.
    if(l->parsed_filepath != NULL)
    {
        Tundra_Str_clear(&l->src_str);
    }

    Tundra_File file;

    i64 result = Tundra_File_open(
        &file, 
        path, 
        TUNDRA_FILE_OPEN_MODE_READONLY, 
        TUNDRA_FILE_WRITE_BEHAVIOR_NONE,
        false,
        false);
    TUNDRA_RT_ASSERT(result >= 0, "Lexer failed to open file at path: \"%s\" "
        "with error: %s.\n", Tundra_err_to_rdbl(result));

    // -- Read in file's contents --
    u64 file_size = Tundra_File_get_size(&file);
    // +1 so we can add a null terminator. This is required as we're going to
    // give this buffer to the String and it expects buffer to be null 
    // terminated.
    const u64 buff_size = file_size + 1;
    char *buffer = (char*)Tundra_alloc_mem(buff_size); 
    Tundra_File_readin_bytes(&file, buffer, file_size, &result);
    TUNDRA_RT_ASSERT(result == (i64)file_size, "Failed to read in all bytes "
        "from file at path: \"%s\".", path)
    buffer[buff_size - 1] = '\0';

    result = Tundra_File_close(&file);
    TUNDRA_RT_ASSERT(result >= 0, "Lexer failed to close file opened from path:"
        " \"%s\" with error: %s.\n", Tundra_err_to_rdbl(buff_size));

    Tundra_Str_take_buffer(&l->src_str, buffer, buff_size);

    l->parsed_filepath = path;
    l->line = 1;
    l->col = 1;
    l->src_idx = 0;
}

/**
 * @brief Flushes stdout and exits program with -1 error code.
 */
static void flush_stdout_exit(void)
{
    Tundra_flush_stdout();
    Tundra_exit(-1);
}

/**
 * @brief Returns true if the Lexer has reached the end of the read in src 
 * content.
 * 
 * @param l Lexer. 
 * 
 * @return `bool` True if the end of source has been reached. 
 */
static bool at_source_end(Lexer *l)
{
    return l->src_idx >= Tundra_Str_size(&l->src_str);
}

/**
 * @brief Prints error location to stderr when an error occurs.
 * 
 * @param file_path File path where the error occurred.
 * @param line Line where the error occurred.
 * @param col Column where the error occurred.
 */
static void print_err_loc(Lexer *l)
{
    Tundra_printf("%s:%u:%u", l->parsed_filepath, l->line, l->col);
}

/**
 * @brief Called when a Lexer lexes an invalid character is lexed. Prints an 
 * error and exits.
 * 
 * @param l Lexer.
 * @param c Invalid char.
 */
static void handle_invalid_char(Lexer *l, char c)
{
    print_err_loc(l);
    Tundra_printf(" -> Invalid character: \'%c\'.\n", c);
    flush_stdout_exit();
}

/**
 * @brief Checks if a Lexer has reached the end of its source, if it has prints 
 * an error and exits.
 * 
 * @param l Lexer.
 */
static void check_unexpected_eos(Lexer *l)
{
    if(!at_source_end(l)) return;

    print_err_loc(l);
    Tundra_printf(" -> Reached end of file unexpectedly.\n");
    flush_stdout_exit();
}

/**
 * @brief Returns true if a peek with `offset` with a Lexer would be valid.
 * 
 * @param l Lexer.
 * @param offset Offset from the Lexer's current position.
 * 
 * @return `bool` True if the peek would be valid. 
 */
static bool is_peek_in_range(Lexer *l, u32 offset)
{
    return l->src_idx + offset < Tundra_Str_size(&l->src_str);
}

/**
 * @brief Peeks in the source string of a Lexer with an offset from the current
 * position to view a character.
 * 
 * @param l Lexer.
 * @param offset Offset from the Lexer's current position in source.
 * 
 * @return `char` Peeked character. 
 */
static char peek(Lexer *l, u32 offset)
{
    u64 targ_idx = l->src_idx + offset;

    if(!is_peek_in_range(l, offset))
    {
        Tundra_printf("Lexer attempted to peek past source's end in file: "
            "\"%s\".\n", l->parsed_filepath);
        flush_stdout_exit();
    }

    return *Tundra_Str_at_nocheck(&l->src_str, targ_idx);
}

/**
 * @brief Skips a Lexer to the next character returning the character it was 
 * on before it advanced.
 * 
 * @param l Lexer.
 * 
 * @return `char` Char the Lexer was on before it advanced. 
 */
static char advance(Lexer *l)
{
    if(peek(l, 0) == '\n')
    {
        ++l->line;
        l->col = 1;
    }
    else { ++l->col; }

    return *Tundra_Str_at(&l->src_str, l->src_idx++);
}

/**
 * @brief Skips a Lexer until either end of source or a newline is found.
 * 
 * @param l Lexer.
 */
static void skip_till_newline(Lexer *l)
{
    while(!at_source_end(l) && peek(l, 0) != '\n')
    {
        advance(l);
    }
}

/**
 * @brief Skips a Lexer past any trivia characters.
 * 
 * @param l Lexer.
 */
static void skip_trivia(Lexer *l)
{
    while(!at_source_end(l))
    {
        const char c = peek(l, 0);

        if(c == ' ' || 
            c == '\t' ||
            c == '\r' || 
            c == '\n')
        {
            advance(l);
            continue;
        }

        // Comment
        if(c == '/' && is_peek_in_range(l, 1) && peek(l, 1) == '/')
        {
            skip_till_newline(l);
            continue;
        }

        // Non trivia character.
        break;
    }
}

/**
 * @brief Lexes an identifier and fills out the passed Token's ID and text.
 * 
 * @param l Lexer.
 * @param t Token to fill. 
 */
static void lex_ident(Lexer *l, Token *t)
{
    // Starting index of the string view this Token will have into the source.
    u64 view_start = l->src_idx;

    char c = peek(l, 0);

    while(Tundra_is_alphnum(c) || c == '-')
    {
        advance(l);
        if(at_source_end(l)) break;

        c = peek(l, 0);
    }

    const u64 num_ch_parsed = l->src_idx - view_start;

    // Construct a view into the source string that views the ident we just
    // parsed.
    t->text = Tundra_Str_make_view(&l->src_str, view_start, 
        num_ch_parsed);

    // Check if the parsed identifier is a keyword.
    TokenID try_kw_to_tok = kw_to_token(t->text.view, t->text.num_char);

    // The ident is a keyword.
    if(try_kw_to_tok != TOKENID_ENUM_END)
    {
        t->id = try_kw_to_tok;

        // Since this is a keyword, we no longer need the identifier text.
        t->text.view = NULL;
        t->text.num_char = 0;
    }

    else if(Tundra_cmp_mem(t->text.view, "null_ptr", 8))
    {
        t->id = TOKENID_NULLPTR_LITERAL;

        // Since this is a null_ptr literal, we no longer need the identifier 
        // text.
        t->text.view = NULL;
        t->text.num_char = 0;
    }

    else if(Tundra_cmp_mem(t->text.view, "true", 4) || 
            Tundra_cmp_mem(t->text.view, "false", 5))
    {
        t->id = TOKENID_BOOL_LITERAL;
    }

    else
    {
        t->id = TOKENID_IDENTIFIER;
    }
}

/**
 * @brief Lexes an integer or float literal and fills out the passed Token's ID
 * and text.
 * 
 * @param l Lexer.
 * @param t Token to fill.
 * @param is_negative If the number to be parsed is negative.
 */
static void lex_int_flt_lit(Lexer *l, Token *t, bool is_negative)
{
    // Default to int literal.
    t->id = TOKENID_INT_LITERAL;

    // If `is_negative` is true, we are on a '-' character and there is 
    // guaranteed to be a digit ahead by one. If `is_negative` is false, we 
    // are on a valid digit.

    // Starting index of the string view this Token will have into the source.
    u64 view_start = l->src_idx;

    if(is_negative) { advance(l); }

    // Wether a dot has been found in the literal.
    bool dot_found = false;

    char c = peek(l, 0);

    while(true)
    {
        if(!Tundra_is_digit(c))
        {
            if(c == '.')
            {
                if(dot_found) { handle_invalid_char(l, c); } // exits
                
                dot_found = true;
                t->id = TOKENID_FLOAT_LITERAL;
            }

            // Found a non digit and it's not a dot, done.
            else { break; }
        }

        advance(l);
        if(at_source_end(l)) { break; }

        c = peek(l, 0);
    }

    const u64 num_ch_parsed = l->src_idx - view_start;

    t->text = Tundra_Str_make_view(&l->src_str, view_start, num_ch_parsed);

    if(dot_found && t->text.view[t->text.num_char - 1] == '.')
    {
        print_err_loc(l);
        Tundra_printf(" -> Trailing dot at end of float literal.\n");
        flush_stdout_exit();
    }
}

/**
 * @brief Lexes a string literal and fills out the passed Token's ID
 * and text.
 * 
 * @param l Lexer.
 * @param t Token to fill.
 */
static void lex_str_lit(Lexer *l, Token *t)
{
    t->id = TOKENID_STR_LITERAL;

    // Starting index of the string view this Token will have into the source.
    u64 view_start = l->src_idx;

    char c = peek(l, 0);

    bool prev_char_was_bslash = false;

    while(true)
    {
        if(c == '"' && !prev_char_was_bslash)
        {
            // We have found a double quote that is not escaped, we are done.
            break;
        }

        prev_char_was_bslash = c == '\\';
        advance(l);
        if(at_source_end(l))
        {
            print_err_loc(l);
            Tundra_printf(" -> Reached end of file while parsing string "
                "defined here: %s:%u:%u.\n",
                l->parsed_filepath,
                t->line,
                t->col);
            flush_stdout_exit();
        }

        c = peek(l, 0);
    }

    const u64 num_ch_parsed = l->src_idx - view_start;

    t->text = Tundra_Str_make_view(&l->src_str, view_start, num_ch_parsed);

    advance(l); // Consume '"'. We wait till here to do it so it is not 
        // included in the token's view.
}

/**
 * @brief Lexes a char literal and fills out the passed Token's ID
 * and text.
 * 
 * @param l Lexer.
 * @param t Token to fill.
 */
static void lex_char_lit(Lexer *l, Token *t)
{
    t->id = TOKENID_CHAR_LITERAL;

    char c = peek(l, 0);

    // Starting index of the string view this Token will have into the source.
    u64 view_start = l->src_idx;

    // Escaped character.
    if(c == '\\')
    {
        advance(l); // Consume '\'
        if(at_source_end(l))
        {
            print_err_loc(l);
            Tundra_print_cstr(
                " -> Reached end of file while parsing character.\n");
            flush_stdout_exit();
        }
        c = peek(l, 0);
    }

    advance(l); // Consume character.

    if(at_source_end(l))
    {
        print_err_loc(l);
        Tundra_print_cstr(
            " -> Reached end of file while parsing character.\n");
        flush_stdout_exit();
    }

    c = peek(l, 0);

    if(c != '\'')
    {
        Tundra_printf("%s:%u:%u -> Either too many characters in char literal "
            "or missing closing quote.\n",
            l->parsed_filepath,
            t->line,
            t->col);
        flush_stdout_exit();
    }

    const u64 num_ch_parsed = l->src_idx - view_start;

    t->text = Tundra_Str_make_view(&l->src_str, view_start, num_ch_parsed);

    advance(l); // Consume '''. We wait till here to do it so it is not 
        // included in the token's view.
}

/**
 * @brief Lexes a non identifier or number and fills the passed Token's ID and
 * text.
 * 
 * @param l Lexer.
 * @param t Token to fill.
 */
static void lex_non_ident_or_num(Lexer *l, Token *t)
{
    // These token types do not have any text to them, so NULL out the view.
    t->text.view = NULL;
    t->text.num_char = 0;

    char c = advance(l); 

    bool at_eos = at_source_end(l);

    switch(c)
    {
        case '-':

            t->id = TOKENID_MINUS;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "->"
            if(c == '>')
            {
                t->id = TOKENID_ARROW;
                advance(l);
                return;
            }

            // Check for "--"
            if(c == '-')
            {
                t->id = TOKENID_MINUS_MINUS;
                advance(l);
                return;
            }

            // Check for "-="
            if(c == '=')
            {
                t->id = TOKENID_MINUS_ASSIGN;
                advance(l);
                return;
            }

            break;

        case '+':

            t->id = TOKENID_PLUS;

            if(at_eos) { return; }

            c = peek(l, 0); 

            // Check for "++"
            if(c == '+')
            {
                t->id = TOKENID_PLUS_PLUS;
                advance(l);
                return;
            }

            // Check for "+="
            if(c == '=')
            {
                t->id = TOKENID_PLUS_ASSIGN;
                advance(l);
                return;
            }

            break;

        case '*':

            t->id = TOKENID_ASTERISK;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "*="
            if(c == '=')
            {
                t->id = TOKENID_ASTERISK_ASSIGN;
                advance(l);
                return;
            }

            break;

        case '/':

            t->id = TOKENID_FORW_SLASH;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "/="
            if(c == '=')
            {
                t->id = TOKENID_FORW_SLASH_ASSIGN;
                advance(l);
                return;
            }

            break;

        case '%':

            t->id = TOKENID_MODULO;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "%="
            if(c == '=')
            {
                t->id = TOKENID_MODULO_ASSIGN;
                advance(l);
                return;
            }

            break;

        case '=':

            t->id = TOKENID_COPY_ASSIGN;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "=="
            if(c == '=')
            {
                t->id = TOKENID_EQUAL_EQUAL;
                advance(l);
                return; 
            }

            break;

        case '!':

            t->id = TOKENID_EXCLAMATION_POINT;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "!="
            if(c == '=')
            {
                t->id = TOKENID_NOT_EQUAL;
                advance(l);
                return;
            }

            break;

        case '^':

            t->id = TOKENID_CARET;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "^="
            if(c == '=')
            {
                t->id = TOKENID_BIT_OR_ASSIGN;
                advance(l);
                return;
            }

            break;

        case '<':

            t->id = TOKENID_LANGLE_BRACKET;

            if(at_eos) { return; }

            c = peek(l, 0);
            // at_eos = at_source_end(l);

            // Check for "<="
            if(c == '=')
            {
                t->id = TOKENID_LESS_EQUAL;
                advance(l);
                return;
            }

            // Check for "<-"
            if(c == '-')
            {
                t->id = TOKENID_RELOC_ASSIGN;
                advance(l);
                return;
            }

            // Check for "<<"
            if(c == '<')
            {
                advance(l);
                at_eos = at_source_end(l);

                t->id = TOKENID_SHIFT_LEFT;

                if(at_eos) { return; }

                c = peek(l, 0);

                // Check for "<<="
                if(c == '=')
                {
                    t->id = TOKENID_SHIFT_LEFT_ASSIGN;
                    advance(l);
                    return;
                }
                
                break;
            }

            break;

        case '>':

            t->id = TOKENID_RANGLE_BRACKET;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for ">="
            if(c == '=')
            {
                t->id = TOKENID_GREATER_EQUAL;
                advance(l);
                return;
            }

            // Check for ">>"
            if(c == '>')
            {
                advance(l);
                at_eos = at_source_end(l);

                t->id = TOKENID_SHIFT_RIGHT;

                if(at_eos) { return; }

                c = peek(l, 0);

                // Check for ">>="
                if(c == '=')
                {
                    t->id = TOKENID_SHIFT_RIGHT_ASSIGN;
                    advance(l);
                    return;
                }
                
                break;
            }

            break;

        case '&':

            t->id = TOKENID_AMPERSAND;

            if(at_eos) { return; }
            
            c = peek(l, 0);

            // Check for "&&"
            if(c == '&')
            {
                t->id = TOKENID_LOGIC_AND;
                advance(l);
                return;
            }        

            // Check for "&="
            if(c == '=')
            {
                t->id = TOKENID_BIT_AND_ASSIGN;
                advance(l);
                return;
            }

            break;

        case '|':

            t->id = TOKENID_VERT_BAR;

            if(at_eos) { return; }
            
            c = peek(l, 0);

            // Check for "||"
            if(c == '|')
            {
                t->id = TOKENID_LOGIC_OR;
                advance(l);
                return;
            }        

            // Check for "|="
            if(c == '=')
            {
                t->id = TOKENID_BIT_OR_ASSIGN;
                advance(l);
                return;
            }

            break;

        case ':':

            t->id = TOKENID_COLON;

            if(at_eos) { return; }

            c = peek(l, 0);

            // Check for "::"
            if(c == ':')
            {
                t->id = TOKENID_SCOPE_RESOLUTION;
                advance(l);
                return;
            }

            break;

        case '@':

            t->id = TOKENID_COMPILE_DIRECTIVE;
            break;
        
        case '(':

            t->id = TOKENID_LPAREN;
            break;

        case ')':

            t->id = TOKENID_RPAREN;
            break;

        case '{':

            t->id = TOKENID_LBRACE;
            break;
    
        case '}':

            t->id = TOKENID_RBRACE;
            break;

        case '[':

            t->id = TOKENID_LBRACKET;
            break;

        case ']':

            t->id = TOKENID_RBRACKET;
            break;

        case ',':

            t->id = TOKENID_COMMA;
            break;

        case ';':

            t->id = TOKENID_SEMICOLON;
            break;

        case '.':

            t->id = TOKENID_DOT;
            break;

        case '?':

            t->id = TOKENID_TERNARY;
            break;

        case '\\':

            t->id = TOKENID_BACK_SLASH;
            break;

        case '~':

            t->id = TOKENID_TILDE;
            break;

        case '$':

            t->id = TOKENID_DOLLAR_SIGN;
            break;

        default:

            handle_invalid_char(l, c); // exits 
    }
}


Token Lexer_get_next_token(Lexer *l)
{
    skip_trivia(l);

    Token t;
    t.line = l->line;
    t.col = l->col;

    if(at_source_end(l))
    {
        t.id = TOKENID_END_OF_FILE;
        return t;
    }

    const char c = peek(l, 0);

    // Identifier or Keyword
    if(Tundra_is_letter(c) || c == '_')
    {
        lex_ident(l, &t);
        return t;
    }

    // Negative number
    if(c == '-' && is_peek_in_range(l, 1) && Tundra_is_digit(peek(l, 1)))
    {
        lex_int_flt_lit(l, &t, true);
        return t;
    }

    // Positive number.
    if(Tundra_is_digit(c))
    {
        lex_int_flt_lit(l, &t, false);
        return t;
    }

    // String literal
    if(c == '"')
    {
        advance(l); // Consume "
        if(at_source_end(l))
        {
            print_err_loc(l);
            Tundra_printf(" -> Reached end of file while parsing string.\n");
            flush_stdout_exit();
        }
        lex_str_lit(l, &t);
        return t;
    }

    // Char literal
    if(c == '\'')
    {
        advance(l); // Consume '''
        if(at_source_end(l))
        {
            print_err_loc(l);
            Tundra_print_cstr(
                " -> Reached end of file while parsing character.\n");
            flush_stdout_exit();
        }
        lex_char_lit(l, &t);
        return t;
    }

    lex_non_ident_or_num(l, &t);

    return t;
}

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
    Tundra_printf("%s:%ud:%ud", l->parsed_filepath, l->line, l->col);
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
    Tundra_flush_stdout();
    Tundra_exit(-1);
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
    Tundra_flush_stdout();
    Tundra_exit(-1);
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
        Tundra_flush_stdout();
        Tundra_exit(-1);
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
static void parse_ident(Lexer *l, Token *t)
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
    }

    else if(Tundra_cmp_mem(t->text.view, "null_ptr", 8))
    {
        t->id = TOKENID_NULLPTR_LITERAL;
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

Token Lexer_get_next_token(Lexer *l)
{
    Token t;
    t.line = l->line;
    t.col = l->col;

    if(at_source_end(l))
    {
        t.id = TOKENID_END_OF_FILE;
        return t;
    }

    const char c = peek(l);

    if(Tundra_is_letter(c) || c == '_')
    {

    }

    TUNDRA_FATAL("Not implemented.\n");
}
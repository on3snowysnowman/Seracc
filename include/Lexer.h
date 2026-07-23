/**
 * @file Lexer.h
 * @author Joel Height (you@domain.com)
 * @brief Lexer definitions for lexing a source file into parseable tokens.
 * 
 * @version 0.1
 * @date 2026-07-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef LEXER_H
#define LEXER_H

#include "tundra/common/TypeDef.h"
#include "tundra/containers/String.h"

#include "Token.h"

/**
 * @brief Lexer for lexing source files into parseable Tokens.
 * 
 */
typedef struct
{
    const char *parsed_filepath; // Path of the file being lexed. 

    // Source string, read contents from the file being lexed.
    // During the duration of compilation, all Tokens construct string views 
    // into this string, so it is crucial that this source not be modified after
    // it is initially read.
    Tundra_String src_str;

    u32 line; // Current line in the src file. 
    u32 col; // Current col in the src file.

    u64 src_idx; // Current idx into the src string.
} Lexer;

/**
 * @brief Initializes a Lexer. Must be called before any operations are used
 * with it.
 * 
 * @param l Lexer. 
 */
void Lexer_init(Lexer *l);

/**
 * @brief Reads the contents of the file at `path` into the Lexer so it can 
 * begin lexing Tokens.
 * 
 * @param l Lexer.
 * @param path Path to file.
 */
void Lexer_read_file(Lexer *l, const char *path);

/**
 * @brief Returns the next lexed Token in the currently lexed source.
 * 
 * @param l Lexer.
 * 
 * @return `Token` Next token. 
 */
Token Lexer_get_next_token(Lexer *l);


#endif
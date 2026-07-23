/**
 * @file Token.h
 * @author Joel Height (you@domain.com)
 * @brief Lexical token obtained from lexing a source file. This file provides
 * the definition for the token, its possible ideas, and methods for getting 
 * readable versions of the ids. 
 * 
 * @version 0.1
 * @date 2026-07-21
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#ifndef TOKEN_H
#define TOKEN_H

#include "tundra/common/Core.h"
#include "tundra/utils/MemUtils.h"
#include "tundra/containers/String.h"
#include "tundra/utils/ConsoleIO.h"


typedef enum TokenID
{
    TOKENID_END_OF_FILE,
    TOKENID_IDENTIFIER,
    TOKENID_COMPILE_DIRECTIVE,      // @
    
    // Literals
    TOKENID_INT_LITERAL,
    TOKENID_FLOAT_LITERAL,
    TOKENID_STR_LITERAL,
    TOKENID_BOOL_LITERAL,
    TOKENID_NULLPTR_LITERAL,
    TOKENID_CHAR_LITERAL,

    // Keywords
    TOKENID_KW_FN,
    TOKENID_KW_MODULE,
    TOKENID_KW_TYPE,
    TOKENID_KW_TYPENAME,
    TOKENID_KW_PUB,
    TOKENID_KW_MUT,
    TOKENID_KW_REF, 
    TOKENID_KW_RET,
    TOKENID_KW_IF,
    TOKENID_KW_ELSE,
    TOKENID_KW_WHILE,
    TOKENID_KW_FOR,
    TOKENID_KW_BREAK,
    TOKENID_KW_CONTINUE,
    TOKENID_KW_LET,

    // Brackets
    TOKENID_LPAREN,                 // (
    TOKENID_RPAREN,                 // )
    TOKENID_LBRACE,                 // {
    TOKENID_RBRACE,                 // }
    TOKENID_LBRACKET,               // [
    TOKENID_RBRACKET,               // ]
    TOKENID_LANGLE_BRACKET,         // <
    TOKENID_RANGLE_BRACKET,         // >

      // Punctuation
    TOKENID_COMMA,                  // ,
    TOKENID_SEMICOLON,              // ;
    TOKENID_DOT,                    // .
    TOKENID_COLON,                  // :

    // Special Characters
    TOKENID_EXCLAMATION_POINT,      // !
    TOKENID_AMPERSAND,              // &
    TOKENID_TILDE,                  // ~
    TOKENID_DOLLAR_SIGN,            // $
    TOKENID_CARET,                  // ^

    // Operators 
    TOKENID_SCOPE_RESOLUTION,       // ::
    TOKENID_TERNARY,                // ?
    TOKENID_ARROW,                  // ->
    TOKENID_COPY_ASSIGN,            // =
    TOKENID_RELOC_ASSIGN,           // <-
    TOKENID_PLUS,                   // +
    TOKENID_PLUS_ASSIGN,            // +=
    TOKENID_MINUS,                  // -        
    TOKENID_MINUS_ASSIGN,           // -=
    TOKENID_ASTERISK,               // *
    TOKENID_ASTERISK_ASSIGN,        // *=
    TOKENID_FORW_SLASH,             // /
    TOKENID_FORW_SLASH_ASSIGN,      // /=
    TOKENID_BACK_SLASH,             /** \ */
    TOKENID_MODULO,                 // %
    TOKENID_MODULO_ASSIGN,          // %=
    TOKENID_BIT_AND_ASSIGN,         // &=
    TOKENID_BIT_OR_ASSIGN,          // |=
    TOKENID_BIT_XOR_ASSIGN,         // ^=
    TOKENID_VERT_BAR,               // |
    TOKENID_EQUAL_EQUAL,            // ==
    TOKENID_PLUS_PLUS,              // ++
    TOKENID_MINUS_MINUS,            // --
    TOKENID_NOT_EQUAL,              // !=
    TOKENID_LESS_EQUAL,             // <=
    TOKENID_GREATER_EQUAL,          // >=
    TOKENID_LOGIC_AND,              // &&
    TOKENID_LOGIC_OR,               // ||
    TOKENID_SHIFT_LEFT,             // <<
    TOKENID_SHIFT_LEFT_ASSIGN,      // <<=
    TOKENID_SHIFT_RIGHT,            // >>                              
    TOKENID_SHIFT_RIGHT_ASSIGN,     // >>=
    TOKENID_ENUM_END // Invalid sentinel
} TokenID;

/**
 * @brief Lexical token that contains its TokenID, line and column where it was 
 * defined and its text.  
 * 
 * A token owns no data and needs no freeing.
 */
typedef struct
{
    TokenID id;
    u32 line;
    u32 col;
    Tundra_StringView text;
} Token;


// Lookup for readable versions of TokenIDs. Each index corresponds to the value
// of the id.
static const char *const tokID_rdbl_lookup[TOKENID_ENUM_END] =
{
    [TOKENID_END_OF_FILE]       = "END_OF_FILE",
    [TOKENID_IDENTIFIER]        = "IDENTIFIER",
    [TOKENID_COMPILE_DIRECTIVE] = "COMPILE_DIRECTIVE",

    // Literals
    [TOKENID_INT_LITERAL]     = "INT_LITERAL",
    [TOKENID_FLOAT_LITERAL]   = "FLOAT_LITERAL",
    [TOKENID_STR_LITERAL]     = "STR_LITERAL",
    [TOKENID_BOOL_LITERAL]    = "BOOL_LITERAL",
    [TOKENID_NULLPTR_LITERAL] = "NULLPTR_LITERAL",
    [TOKENID_CHAR_LITERAL]    = "CHAR_LITERAL",

    // Keywords
    [TOKENID_KW_FN]       = "KW_FN",
    [TOKENID_KW_MODULE]   = "KW_MODULE",
    [TOKENID_KW_TYPE]     = "KW_TYPE",
    [TOKENID_KW_TYPENAME] = "KW_TYPENAME",
    [TOKENID_KW_PUB]      = "KW_PUB",
    [TOKENID_KW_MUT]      = "KW_MUT",
    [TOKENID_KW_REF]      = "KW_REF",
    [TOKENID_KW_RET]      = "KW_RET",
    [TOKENID_KW_IF]       = "KW_IF",
    [TOKENID_KW_ELSE]     = "KW_ELSE",
    [TOKENID_KW_WHILE]    = "KW_WHILE",
    [TOKENID_KW_FOR]      = "KW_FOR",
    [TOKENID_KW_BREAK]    = "KW_BREAK",
    [TOKENID_KW_CONTINUE] = "KW_CONTINUE",
    [TOKENID_KW_LET]      = "KW_LET",

    // Brackets
    [TOKENID_LPAREN]         = "LPAREN",
    [TOKENID_RPAREN]         = "RPAREN",
    [TOKENID_LBRACE]         = "LBRACE",
    [TOKENID_RBRACE]         = "RBRACE",
    [TOKENID_LBRACKET]       = "LBRACKET",
    [TOKENID_RBRACKET]       = "RBRACKET",
    [TOKENID_LANGLE_BRACKET] = "LANGLE_BRACKET",
    [TOKENID_RANGLE_BRACKET] = "RANGLE_BRACKET",

    // Punctuation
    [TOKENID_COMMA]     = "COMMA",
    [TOKENID_SEMICOLON] = "SEMICOLON",
    [TOKENID_DOT]       = "DOT",
    [TOKENID_COLON]     = "COLON",

    // Special characters
    [TOKENID_EXCLAMATION_POINT] = "EXCLAMATION_POINT",
    [TOKENID_AMPERSAND]         = "AMPERSAND",
    [TOKENID_TILDE]             = "TILDE",
    [TOKENID_DOLLAR_SIGN]       = "DOLLAR_SIGN",
    [TOKENID_CARET]             = "CARET",

    // Operators
    [TOKENID_SCOPE_RESOLUTION]   = "SCOPE_RESOLUTION",
    [TOKENID_TERNARY]            = "TERNARY",
    [TOKENID_ARROW]              = "ARROW",
    [TOKENID_COPY_ASSIGN]        = "COPY_ASSIGN",
    [TOKENID_RELOC_ASSIGN]       = "RELOC_ASSIGN",
    [TOKENID_PLUS]               = "PLUS",
    [TOKENID_PLUS_ASSIGN]        = "PLUS_ASSIGN",
    [TOKENID_MINUS]              = "MINUS",
    [TOKENID_MINUS_ASSIGN]       = "MINUS_ASSIGN",
    [TOKENID_ASTERISK]           = "ASTERISK",
    [TOKENID_ASTERISK_ASSIGN]    = "ASTERISK_ASSIGN",
    [TOKENID_FORW_SLASH]         = "FORW_SLASH",
    [TOKENID_FORW_SLASH_ASSIGN]  = "FORW_SLASH_ASSIGN",
    [TOKENID_BACK_SLASH]         = "BACK_SLASH",
    [TOKENID_MODULO]             = "MODULO",
    [TOKENID_MODULO_ASSIGN]      = "MODULO_ASSIGN",
    [TOKENID_BIT_AND_ASSIGN]     = "BIT_AND_ASSIGN",
    [TOKENID_BIT_OR_ASSIGN]      = "BIT_OR_ASSIGN",
    [TOKENID_BIT_XOR_ASSIGN]     = "BIT_XOR_ASSIGN",
    [TOKENID_VERT_BAR]           = "VERT_BAR",
    [TOKENID_EQUAL_EQUAL]        = "EQUAL_EQUAL",
    [TOKENID_PLUS_PLUS]          = "PLUS_PLUS",
    [TOKENID_MINUS_MINUS]        = "MINUS_MINUS",
    [TOKENID_NOT_EQUAL]          = "NOT_EQUAL",
    [TOKENID_LESS_EQUAL]         = "LESS_EQUAL",
    [TOKENID_GREATER_EQUAL]      = "GREATER_EQUAL",
    [TOKENID_LOGIC_AND]          = "LOGIC_AND",
    [TOKENID_LOGIC_OR]           = "LOGIC_OR",
    [TOKENID_SHIFT_LEFT]         = "SHIFT_LEFT",
    [TOKENID_SHIFT_LEFT_ASSIGN]  = "SHIFT_LEFT_ASSIGN",
    [TOKENID_SHIFT_RIGHT]        = "SHIFT_RIGHT",
    [TOKENID_SHIFT_RIGHT_ASSIGN] = "SHIFT_RIGHT_ASSIGN",
    [TOKENID_ENUM_END]           = "ENUM_END"
};

/**
 * @brief Given a TokenID, returns a readable version of it, or errors if
 * the ID is not valid.
 * 
 * @param id ID to lookup.
 * 
 * @return const char* Readable version of the ID. 
 */
static const char *get_rdbl_tokID(TokenID id)
{
    TUNDRA_RT_ASSERT(
        id >= 0 && id < TOKENID_ENUM_END,
        "Invalid TokenID: \"%d\".\n",
        id
    );

    const char *rdbl = tokID_rdbl_lookup[id];

    TUNDRA_RT_ASSERT(
        rdbl != NULL,
        "No readable name defined for TokenID: \"%d\".\n",
        id
    );

    return rdbl;
}

/**
 * @brief Outputs a readable version of a Token to stdout.
 * 
 * @param t Token.
 */
static void stdout_Token(const Token *t)
{
    Tundra_printf("ID: %s\nLine: %u\nCol: %u",
        get_rdbl_tokID(t->id),
        t->line,
        t->col);
    
    if(t->text.view == NULL) return;

    Tundra_print_cstr("\nText: \"");
    Tundra_print_cstr_w_len(t->text.view, t->text.num_char);
    Tundra_print_cstr("\"");
    

}

/**
 * @brief Given a readable keyword, attempts to return the TokenID corresponding 
 * to it or returns TOKENID_ENUM_END if the readable was not a keyword.
 * 
 * `text` does not need to be null terminated. If it is, ensure it is not 
 * counted in `len`.
 * 
 * @param text Readable keyword.
 * @param len Number of characters in `text`, not including any null terminator.
 * 
 * @return `TokenID` Id of keyword or `TOKENID_ENUM_END` if `text` does not 
 * represent a keyword.
 */
static TokenID tokenID_from_rdbl_kw(const char *text, u64 len)
{
    switch (len)
    {
        case 2:
            if (Tundra_cmp_mem(text, "fn", 2))
                return TOKENID_KW_FN;
            if (Tundra_cmp_mem(text, "if", 2))
                return TOKENID_KW_IF;
            break;

        case 3:
            if (Tundra_cmp_mem(text, "mut", 3))
                return TOKENID_KW_MUT;
            if (Tundra_cmp_mem(text, "ref", 3))
                return TOKENID_KW_REF;
            if (Tundra_cmp_mem(text, "ret", 3))
                return TOKENID_KW_RET;
            if (Tundra_cmp_mem(text, "for", 3))
                return TOKENID_KW_FOR;
            if (Tundra_cmp_mem(text, "let", 3))
                return TOKENID_KW_LET;
			if (Tundra_cmp_mem(text, "pub", 3))
                return TOKENID_KW_PUB;
			
            break;

        case 4:
            if (Tundra_cmp_mem(text, "type", 4))
                return TOKENID_KW_TYPE;
            if (Tundra_cmp_mem(text, "else", 4))
                return TOKENID_KW_ELSE;
            break;

        case 5:
            if (Tundra_cmp_mem(text, "while", 5))
                return TOKENID_KW_WHILE;
            if (Tundra_cmp_mem(text, "break", 5))
                return TOKENID_KW_BREAK;
            break;

        case 6:
            if (Tundra_cmp_mem(text, "module", 6))
                return TOKENID_KW_MODULE;
            break;

        case 8:
            if (Tundra_cmp_mem(text, "continue", 8))
                return TOKENID_KW_CONTINUE;
            if(Tundra_cmp_mem(text, "typename", 8))
                return TOKENID_KW_TYPENAME;
            break;

		default: break;
    }

    return TOKENID_ENUM_END;
}

#endif

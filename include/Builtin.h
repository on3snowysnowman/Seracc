/**
 * @file Builtin.h
 * @author Joel Height (On3SnowySnowman@gmail.com)
 * @brief Definitions for builtin types, such as int and u64.
 * @version 0.1
 * @date 2026-07-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef BUILTIN_H
#define BUILTIN_H

#include "tundra/common/Core.h"
#include "tundra/utils/ConsoleIO.h"
#include "tundra/utils/MemUtils.h"

typedef enum 
{
    BUILTIN_U8,
    BUILTIN_I8,
    BUILTIN_U16,
    BUILTIN_I16,
    BUILTIN_U32,
    BUILTIN_I32,
    BUILTIN_U64,
    BUILTIN_I64,
    BUILTIN_VOID,
    BUILTIN_BOOL,
    BUILTIN_CHAR,
    BUILTIN_FLOAT,
    BUILTIN_OPQ_PTR, // opaque_ptr 
    BUILTIN_ENUM_END // Invalid sentinel
} BuiltinType;

static const char *const bt_type_rdbl_lookup[] = 
{
    [BUILTIN_U8]        = "U8",
    [BUILTIN_I8]        = "I8",
    [BUILTIN_U16]       = "U16",
    [BUILTIN_I16]       = "I16",
    [BUILTIN_U32]       = "U32",
    [BUILTIN_I32]       = "I32",
    [BUILTIN_U64]       = "u64",
    [BUILTIN_I64]       = "I64",
    [BUILTIN_VOID]      = "VOID",
    [BUILTIN_BOOL]      = "BOOL",
    [BUILTIN_CHAR]      = "CHAR",
    [BUILTIN_FLOAT]     = "FLOAT",
    [BUILTIN_OPQ_PTR]   = "OPQ_PTR",
    [BUILTIN_ENUM_END]  = "ENUM_END"
};

/**
 * @brief Given a BuiltinType, returns a readable version of it, or errors if
 * the type is not valid.
 * 
 * @param type type to lookup.
 * 
 * @return const char* Readable version of the type. 
 */
static const char *get_rdbl_bt_type(BuiltinType type)
{
    TUNDRA_RT_ASSERT(
        type >= 0 && type < BUILTIN_ENUM_END,
        "Invalid BuiltinType: \"%d\".\n",
        type
    );

    const char *rdbl = bt_type_rdbl_lookup[type];

    TUNDRA_RT_ASSERT(
        rdbl != NULL,
        "No readable name defined for BuiltinType: \"%d\".\n",
        type
    );

    return rdbl;
}

/**
 * @brief Outputs a readable version of a BuiltinType to stdout, erroring if
 * the type is not valid.
 * 
 * @param type Type.
 */
static void stdout_bt_type(BuiltinType type)
{
    Tundra_print_cstr(get_rdbl_bt_type(type));
}

/**
 * @brief Given a string, checks if it corresponds to any BuiltinTypes, 
 * returning the BuiltinType if it does. Otherwise returns BUILTIN_ENUM_END if
 * `text` was not a readable BuiltinType.
 * 
 * `text` does not need to be null terminated. If it is, ensure it is not 
 * counted in `len`.
 * 
 * @param text Potential readable BuiltinType.
 * @param len Number of characters in `text`, not including any null terminator.
 * 
 * @return `BuiltinType` Type found in the string or `BUILTIN_ENUM_END` if 
 * `text` does not represent a BuiltinType.
 */
static BuiltinType bt_type_from_rdbl(const char *text, u64 len)
{
    switch(len)
    {
        case 2:

            if(Tundra_cmp_mem(text, "u8", 2)) { return BUILTIN_U8; }
            if(Tundra_cmp_mem(text, "i8", 2)) { return BUILTIN_I8; }
            break;

        case 3:

            if(Tundra_cmp_mem(text, "u16", 3)) { return BUILTIN_U16; }
            if(Tundra_cmp_mem(text, "i16", 3)) { return BUILTIN_I16; }
            if(Tundra_cmp_mem(text, "u32", 3)) { return BUILTIN_U32; }
            if(Tundra_cmp_mem(text, "i32", 3)) { return BUILTIN_I32; }
            if(Tundra_cmp_mem(text, "int", 3)) { return BUILTIN_I32; }
            if(Tundra_cmp_mem(text, "u64", 3)) { return BUILTIN_U64; }
            if(Tundra_cmp_mem(text, "i64", 3)) { return BUILTIN_I64; }
            break;

        case 4:

            if(Tundra_cmp_mem(text, "void", 4)) { return BUILTIN_VOID; }
            if(Tundra_cmp_mem(text, "bool", 4)) { return BUILTIN_BOOL; }
            if(Tundra_cmp_mem(text, "char", 4)) { return BUILTIN_CHAR; }
            break;

        case 5:

            if(Tundra_cmp_mem(text, "float", 5)) { return BUILTIN_FLOAT; }
            break;

        case 10:

            if(Tundra_cmp_mem(text, "opaque_ptr", 10)) 
                { return BUILTIN_OPQ_PTR; }
            break;

        default: break;
    }

    return BUILTIN_ENUM_END;
}

#endif
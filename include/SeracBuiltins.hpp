// SeracBuiltins.hpp

#pragma once

#include <array>
#include <vector>

static const char * const INT_LIT_IDENT = "INT_LIT";
static const char * const BIN_LIT_IDENT = "BIN_LIT";
static const char * const HEX_LIT_IDENT = "HEX_LIT";
static const char * const FLOAT_LIT_IDENT = "FLOAT_LIT";

enum class BuiltinType
{
    U8, 
    I8,
    U16,
    I16,
    U32,
    I32,
    U64,
    I64,
    BOOL,
    NULLPTR,
    OPAQUE,
    VOID,
    FLOAT,
    DOUBLE,
    INT_LIT,
    BIN_LIT,
    HEX_LIT,
    FLOAT_LIT, 
    INVALID
};

const std::vector<std::pair<const char * const, BuiltinType>> 
    readable_to_builtin
{
    {"u8", BuiltinType::U8},
    {"i8", BuiltinType::I8},
    {"u16", BuiltinType::U16},
    {"i16", BuiltinType::I16},
    {"u32", BuiltinType::U32},
    {"i32", BuiltinType::I32},
    {"int", BuiltinType::U32},
    {"u8", BuiltinType::U64},
    {"i8", BuiltinType::I64},
    {"bool", BuiltinType::BOOL},
    {"nullptr", BuiltinType::NULLPTR},
    {"opaque", BuiltinType::OPAQUE},
    {"void", BuiltinType::VOID},
    {"float", BuiltinType::FLOAT},
    {"double", BuiltinType::DOUBLE},
    {INT_LIT_IDENT, BuiltinType::INT_LIT},
    {BIN_LIT_IDENT, BuiltinType::BIN_LIT},
    {HEX_LIT_IDENT, BuiltinType::HEX_LIT},
    {FLOAT_LIT_IDENT, BuiltinType::FLOAT_LIT}
};


// const std::vector<const char *> builtin_to_symbol_ident = 
// {
//     "U8", 
//     "I8",
//     "U16",
//     "I16",
//     "U32",
//     "I32",
//     "U64",
//     "I64",
//     "BOOL",
//     "NULLPTR",
//     "OPAQUE",
//     "VOID",
//     "FLOAT",
//     "DOUBLE",
//     "INT_LIT",
//     "BIN_LIT",
//     "HEX_LIT",
//     "FLOAT_LIT"
// };

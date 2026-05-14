// Builtins.hpp

#pragma once

#include <vector>
#include <iostream>

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
    VOID,
    FLOAT,
    DOUBLE,
    INVALID
};

static inline std::ostream& operator<<(std::ostream &os, BuiltinType type)
{
    switch(type)
    {
        case BuiltinType::U8:
            os << "U8";
            break;
        
        case BuiltinType::I8:
            os << "I8";
            break;

        case BuiltinType::U16:
            os << "U16";
            break;

        case BuiltinType::I16:
            os << "I16";
            break;

        case BuiltinType::U32:
            os << "U32";
            break;

        case BuiltinType::I32:
            os << "I32";
            break;

        case BuiltinType::U64:
            os << "U64";
            break;

        case BuiltinType::I64:
            os << "I64";
            break;

        case BuiltinType::BOOL:
            os << "BOOL";
            break;

        case BuiltinType::VOID:
            os << "VOID";
            break;

        case BuiltinType::FLOAT:
            os << "FLOAT";
            break;

        case BuiltinType::DOUBLE:
            os << "DOUBLE";
            break;

        case BuiltinType::INVALID:
            os << "INVALID";
            break;
    }

    return os;
}

enum class BuiltinPtrType
{
    NULL_PTR,
    OPAQUE_PTR,
    CSTR_PTR
};

const std::vector<std::pair<const char * const, BuiltinType>> 
    readable_to_builtin
{
    {"char", BuiltinType::U8},
    {"u8", BuiltinType::U8},
    {"i8", BuiltinType::I8},
    {"u16", BuiltinType::U16},
    {"i16", BuiltinType::I16},
    {"u32", BuiltinType::U32},
    {"i32", BuiltinType::I32},
    {"int", BuiltinType::I32},
    {"u64", BuiltinType::U64},
    {"i64", BuiltinType::I64},
    {"bool", BuiltinType::BOOL},
    {"void", BuiltinType::VOID},
    {"float", BuiltinType::FLOAT},
    {"double", BuiltinType::DOUBLE},
};

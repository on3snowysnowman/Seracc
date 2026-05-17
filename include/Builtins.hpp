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
    CHAR,
    OPAQUE_PTR,
    INVALID
};

static inline std::ostream& operator<<(std::ostream &os, BuiltinType type)
{
    switch(type)
    {
        case BuiltinType::U8:
            os << "u8";
            break;
        
        case BuiltinType::I8:
            os << "i8";
            break;

        case BuiltinType::U16:
            os << "u16";
            break;

        case BuiltinType::I16:
            os << "i16";
            break;

        case BuiltinType::U32:
            os << "u32";
            break;

        case BuiltinType::I32:
            os << "i32";
            break;

        case BuiltinType::U64:
            os << "u64";
            break;

        case BuiltinType::I64:
            os << "i64";
            break;

        case BuiltinType::BOOL:
            os << "bool";
            break;

        case BuiltinType::VOID:
            os << "void";
            break;

        case BuiltinType::FLOAT:
            os << "float";
            break;

        case BuiltinType::DOUBLE:
            os << "double";
            break;

        case BuiltinType::CHAR:
            os << "char";
            break;

        case BuiltinType::OPAQUE_PTR:
            os << "opaque_ptr";
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
    {"char", BuiltinType::CHAR},
    {"opaque_ptr", BuiltinType::OPAQUE_PTR}
};

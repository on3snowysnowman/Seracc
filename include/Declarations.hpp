// Declarations.hpp

#pragma once

#include <string>
#include <cstdint>
#include <vector>


enum class ReferenceType
{
    NONE,
    REF,
    REFMUT
};

struct TypeDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    ReferenceType ref_type;
    std::vector<bool> ptr_depth_mutability;
};

struct FieldDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    bool binding_mutable;
    TypeDecl type_decl;
};

struct ExpressionDecl
{
    uint32_t line;
    uint32_t col;

};

struct ScopeDecl
{
    uint32_t line;
    uint32_t col;


};

struct ParameterDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    bool binding_mutable;
    TypeDecl type_decl;
};

struct FunctionDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    
    TypeDecl ret_type_decl;
};


struct StructDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
};

struct NamespaceDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    std::vector<NamespaceDecl> namespace_decls;
    std::vector<StructDecl> structs_decls;
    std::vector<FunctionDecl> function_decls;
};


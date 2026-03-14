// ASTDeclarations.hpp

#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

#include "Token.hpp"


enum class ReferenceType
{
    NONE,
    REF,
    REFMUT
};

struct TypeRef
{
    std::string name;

    // Bool list for storing pointee mutability at each depth. The first bool 
    // would denote the mutability of the first pointed to object. So if the 
    // first (zeroth) bool is set, we would have a type like *mut T. If ptr_depth
    // is 2, and both flags were set in this boolset, we would have *mut *mut T.
    // If first bool was set and second was low: *mut * T. "* T" is mutable in
    // this context, while "T" is not. 
    std::vector<bool> ptr_pointee_mut; 
    ReferenceType ref_type;

    uint64_t resolved_type_id = 0;
};

struct ReceiverMeta
{
    TypeRef receiver_type;
    std::string receiver_name;
};

struct Decl
{
    virtual ~Decl() = default;
    std::string name;

    uint32_t line;
    uint32_t col;
};

struct Field
{
    bool is_mut;
    bool is_pub;
    TypeRef t;
    std::string name;
};

struct Param 
{
    bool is_mut;
    bool is_unqual_param;
    TypeRef t;
    std::string name;
};

struct BlockStub
{
    uint32_t start_idx; // Source start idx 
    uint32_t end_idx; // Source end idx
};

struct NamespaceDecl : Decl 
{
    std::vector<std::unique_ptr<Decl>> decls;
};

struct StructDecl : Decl 
{
    std::vector<Field> fields;
};

struct FunctionDecl : Decl 
{
    std::optional<ReceiverMeta> receiver_meta;
    std::vector<Param> parameters;
    TypeRef return_type;
    BlockStub body;
    bool is_pub;
};

struct NestedContainer
{
    std::unique_ptr<Decl> decl;
    bool is_pub;
};

struct ComponentDecl : Decl 
{
    std::vector<Field> fields;
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    std::vector<NestedContainer> nested_cnts;
};

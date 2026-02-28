// Declarations.hpp

#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

#include "Token.hpp"


enum class RefType
{
    NONE,
    REF,
    MUTREF
};


struct TypeRef
{
    std::string name;

    // Bit mask for storing pointee mutability at each depth.
    std::vector<bool> ptr_pointee_mut; 
    RefType ref_type;
    uint32_t ptr_depth;
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
    TypeRef t;
    std::string name;
};

struct Param 
{
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
    std::optional<ReceiverMeta> receiver;
    std::vector<Param> parameters;
    TypeRef return_type;
    BlockStub body;
    bool is_pub;
};

struct ComponentDecl : Decl 
{
    std::vector<Field> fields;
    std::vector<std::unique_ptr<FunctionDecl>> functions;
};

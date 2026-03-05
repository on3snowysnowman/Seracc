// Resolver.hpp

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "Declarations.hpp"
#include "STBlder.hpp"
#include "Parser.hpp"

// A fully-resolved type (base + ref + pointer chain).
// TypeId is index into Resolver::types.
struct ConstructedType
{
    enum class BaseKind : uint8_t
    {
        BUILTIN,
        USER_TYPE
    };

    // Base identity
    BaseKind base_kind = BaseKind::BUILTIN;

    // If BUILTIN
    std::string builtin_name; // e.g. "i32", "u64", "bool", "void", "opaque"

    // If USER_TYPE
    const TypeSymbol* user_sym = nullptr; // points into symbol table entry
    std::string user_qual_name;           // for diagnostics/debug (optional)

    // Modifiers
    ReferenceType ref_type = ReferenceType::NONE;
    std::vector<bool> ptr_pointee_mut; // same semantics as in TypeRef
};

// Hashable key to intern constructed types.
struct TypeKey
{
    // Base identity
    bool is_builtin = true;
    std::string base_builtin;           // if builtin
    const TypeSymbol* base_user = nullptr; // if user

    // Modifiers
    ReferenceType ref_type = ReferenceType::NONE;
    std::vector<bool> ptr_pointee_mut;

    bool operator==(const TypeKey& o) const;
};

struct TypeKeyHash
{
    std::size_t operator()(const TypeKey& k) const noexcept;
};

class Resolver
{
public:
    Resolver();

    // Mutates the AST: fills TypeRef.resolved_type_id.
    // Returns the constructed type table (TypeId -> ConstructedType).
    const std::vector<ConstructedType>& resolve(
        Program& p,
        const NamespaceSymbol* global_ns
    );

private:
    // ---- Traversal
    void resolve_decl_list(std::vector<std::unique_ptr<Decl>>& decls);
    void resolve_namespace(NamespaceDecl* ns_decl);
    void resolve_struct(StructDecl* s_decl);
    void resolve_component(ComponentDecl* c_decl);
    void resolve_function(FunctionDecl* f_decl);

    void resolve_type_ref(TypeRef& tr, uint32_t line, uint32_t col);

    // ---- Lookup helpers
    const TypeSymbol* lookup_user_type(const std::string& name, uint32_t line, uint32_t col);
    const NamespaceSymbol* resolve_namespace_path(const std::vector<std::string>& parts, uint32_t line, uint32_t col);

    bool is_builtin_name(const std::string& name) const;
    std::string normalize_builtin(const std::string& name) const;

    // ---- Interning
    uint64_t intern_type(const TypeKey& key);

    // ---- Diagnostics
    [[noreturn]] void error_at(uint32_t line, uint32_t col, const std::string& msg) const;

private:
    Program* current_prog = nullptr;
    const NamespaceSymbol* global_ns = nullptr;
    const NamespaceSymbol* current_ns = nullptr;

    // Interning store
    std::vector<ConstructedType> types;
    std::unordered_map<TypeKey, uint64_t, TypeKeyHash> type_intern;

    // Builtin set / normalization
    std::unordered_map<std::string, std::string> builtin_alias; // "int"->"i32", "char"->"u8"
};
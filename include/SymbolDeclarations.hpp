// SymbolDeclarations.hpp

#pragma once

#include <optional>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>

#include "ASTDeclarations.hpp"
#include "SeracBuiltins.hpp"

enum class SymbolType
{
    BUILTIN,
    STRUCT,
    COMPONENT,
    FN,
    VAR,
    PARAM,
    MODULE,
    FIELD,
    SCOPE,
    INVALID
};


struct Symbol
{
    SymbolType sym_type = SymbolType::INVALID;
    std::optional<uint64_t> scope_idx {}; // Idx of the scope this symbol is in.
    virtual ~Symbol() = default;
};

struct BuiltinSymbol : Symbol
{
    BuiltinSymbol() { sym_type = SymbolType::BUILTIN; }

    BuiltinType b_type = BuiltinType::INVALID;
};

struct StructSymbol : Symbol
{
    StructSymbol() { sym_type = SymbolType::STRUCT; }

    uint64_t created_scope_idx = 0;
    StructDecl * ast_node_ptr = nullptr;
};

struct ComponentSymbol : Symbol
{
    ComponentSymbol() { sym_type = SymbolType::COMPONENT; }

    uint64_t created_scope_idx = 0;
    ComponentDecl * ast_node_ptr = nullptr;
};


struct FunctionSymbol : Symbol
{
    FunctionSymbol() { sym_type = SymbolType::FN; }

    uint64_t created_scope_idx = 0;
    FunctionDecl * ast_node_ptr = nullptr;
};

struct VarSymbol : Symbol
{
    VarSymbol() { sym_type = SymbolType::VAR; }
    VarDeclStmt * ast_node_ptr = nullptr;
};

struct ParamSymbol : Symbol
{
    ParamSymbol() { sym_type = SymbolType::PARAM; }
    Parameter * ast_node_ptr = nullptr;
};

struct ModuleSymbol : Symbol
{
    ModuleSymbol() { sym_type = SymbolType::MODULE; }

    uint64_t created_scope_idx = 0;
    // ModuleDecl * ast_node_ptr = nullptr;

    // We have a vector of pointers here since we allow multiple definitions 
    // for the same module name.
    std::vector<ModuleDecl*> ast_node_ptrs;
};

struct FieldSymbol : Symbol
{
    FieldSymbol() { sym_type = SymbolType::FIELD; }
    FieldDecl * ast_node_ptr = nullptr;
};

struct Scope
{
    // Type of the symbol that owns this scope.
    SymbolType owning_symbol_type = SymbolType::INVALID; 
    std::optional<uint64_t> parent_scope_idx;
    std::unordered_map<std::string, uint64_t> sym_name_to_symbol_idx;
};

// =============================================================================


struct SymbolTable
{
    uint64_t global_symbol_idx = 0;
    std::vector<std::unique_ptr<Symbol>> symbols;
    std::vector<Scope> scopes;
};

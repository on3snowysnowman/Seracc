// SymbolDeclarations.hpp

#pragma once

#include <optional>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>

#include "ASTDeclarations.hpp"

enum class SymbolType
{
    STRUCT,
    COMPONENT,
    FN,
    VAR,
    PARAM,
    MODULE,
    FIELD,
    INVALID
};


struct Symbol
{
    SymbolType sym_type = SymbolType::INVALID;
    std::optional<uint64_t> scope_idx {}; // Idx of the scope this symbol is in.
    virtual ~Symbol() = default;
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
    FunctionDecl * ast_node_ptr;
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
    ModuleDecl * ast_node_ptr = nullptr;
};

struct FieldSymbol : Symbol
{
    FieldSymbol() { sym_type = SymbolType::FIELD; }
    FieldDecl * ast_node_ptr = nullptr;
};

struct Scope
{
    std::optional<uint64_t> parent_scope_idx;
    std::unordered_map<std::string, uint64_t> sym_name_to_symbol_idx;
};

// =============================================================================


struct SymbolTable
{
    std::vector<std::unique_ptr<Symbol>> symbols;
    std::vector<Scope> scopes;
};

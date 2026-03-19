// SymbolDeclarations.hpp

#pragma once

#include <optional>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>

enum class SymbolType
{
    TYPE,
    FN,
    VAR,
    MODULE,
    FIELD,
    INVALID
};


struct Symbol
{
    // std::string name;
    SymbolType sym_type = SymbolType::INVALID;
    std::optional<uint64_t> scope_idx {}; // Idx of the scope this symbol is in.
    virtual ~Symbol() = default;
};

struct TypeSymbol : Symbol
{
    TypeSymbol() { sym_type = SymbolType::TYPE; }

    uint64_t created_scope_idx = 0;
};

struct FunctionSymbol : Symbol
{
    FunctionSymbol() { sym_type = SymbolType::FN; }

    uint64_t created_scope_idx = 0;
};

struct VarSymbol : Symbol
{
    VarSymbol() { sym_type = SymbolType::VAR; }
};

struct ModuleSymbol : Symbol
{
    ModuleSymbol() { sym_type = SymbolType::MODULE; }

    uint64_t created_scope_idx = 0;
};

struct FieldSymbol : Symbol
{
    FieldSymbol() { sym_type = SymbolType::FIELD; }
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

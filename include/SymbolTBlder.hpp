// SymbolTBlder.hpp

#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "Program.hpp"


// DECLARATIONS ================================================================

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
    std::string name;
    SymbolType sym_type = SymbolType::INVALID;
    uint64_t declr_scope_id = 0;
    virtual ~Symbol() = default;
};

struct TypeSymbol : Symbol
{
    TypeSymbol() { sym_type = SymbolType::TYPE; }

    std::optional<uint64_t> child_scope_id;
};

struct FunctionSymbol : Symbol
{
    FunctionSymbol() { sym_type = SymbolType::FN; }

    uint64_t child_scope_id = 0;
};

struct VarSymbol : Symbol
{
    VarSymbol() { sym_type = SymbolType::VAR; }
};

struct ModuleSymbol : Symbol
{
    ModuleSymbol() { sym_type = SymbolType::MODULE; }
    
    uint64_t child_scope_id = 0;
};

struct FieldSymbol : Symbol
{
    FieldSymbol() { sym_type = SymbolType::FIELD; }
};

struct Scope
{
    std::optional<uint64_t> parent_scope_id;
    std::unordered_map<std::string, uint64_t> sym_name_to_id;
};

// =============================================================================


struct SymbolTable
{
    std::vector<std::unique_ptr<Symbol>> symbols;
    std::vector<Scope> scope_syms;
};


class SymbolTBlder
{

public:

    SymbolTBlder();

    SymbolTable build(const Program &p);

private:

    // Members

    std::vector<std::unique_ptr<Symbol>> symbols;
    std::vector<Scope> scope_syms;

    // Methods

    uint64_t build_namespace(const NamespaceDecl * const ptr, 
        std::optional<uint64_t> parent_scope_id);

    uint64_t add_scope_assign_id(Scope &&s);
};

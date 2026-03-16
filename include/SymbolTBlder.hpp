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


class SymbolTBlder
{

public:

    SymbolTBlder();

    SymbolTable build(Program &p);

private:

    // Members

    const char * parsed_file = nullptr;

    std::vector<std::unique_ptr<Symbol>> symbols;
    std::vector<Scope> scopes;

    
    // Methods

    void add_symbol_to_scope(uint64_t scope_idx, uint64_t symbol_idx,
        std::string symbol_name, uint32_t symbol_line, uint32_t symbol_col, 
        bool is_variable = false);

    void build_top_level(ModuleDecl * const ptr);

    void build_statement(Statement * const ptr, uint64_t parent_scope_idx);

    void build_scope_body(ScopeBody &body, uint64_t parent_scope_idx,
        std::optional<uint64_t> existing_scope);

    void build_field(FieldDecl * const ptr, 
        uint64_t parent_scope_id);

    void build_struct(StructDecl * const ptr,
        uint64_t parent_scope_id);

    void build_function(FunctionDecl * const ptr,
        uint64_t parent_scope_id);

    void build_component(ComponentDecl * const ptr,
        uint64_t parent_scope_id);

    void build_module(ModuleDecl * const ptr, 
        uint64_t parent_scope_id);

    uint64_t get_next_scope_idx();
    uint64_t get_next_symbol_idx();
};

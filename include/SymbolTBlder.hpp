// SymbolTBlder.hpp

#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "Program.hpp"
#include "SymbolDeclarations.hpp"


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

    void add_builtin_symbol(uint64_t global_scope_idx, 
        const std::string &symbol_name, BuiltinType b_type);

    void add_builtin_symbols(uint64_t global_scope_idx);

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

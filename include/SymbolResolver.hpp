// SymbolResolver.hpp 


#pragma once

#include "Program.hpp"
#include "SymbolDeclarations.hpp"


class SymbolResolver
{

public:

    SymbolResolver();

    void resolve(Program &p, const SymbolTable &s_table);

private:

    void resolve_module(ModuleDecl * const ptr);
    void resolve_field(FieldDecl * const ptr);
    void resolve_struct(StructDecl * const ptr);
    void resolve_function(FunctionDecl * const ptr);
    void resolve_component(ComponentDecl * const ptr);
    void resolve_type_decl(TypeDecl * const ptr, uint64_t scope_idx);
    
    void resolve_statement(Statement * const ptr);
    
    void resolve_expression(Expression * const ptr, uint64_t scope_idx);

    uint64_t find_symbol_idx(const std::string &ident, uint64_t scope_idx,
        uint64_t symbol_line, uint64_t symbol_col);

    const char * parsed_file = nullptr;
    const SymbolTable * s_table_ptr = nullptr;
};

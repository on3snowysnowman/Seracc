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

    // Members

    const char * parsed_file = nullptr;
    const SymbolTable * s_table = nullptr;


    // Methods

    void print_error_location(uint32_t line, uint32_t col) const;

    void resolve_module(ModuleDecl * const ptr);
    void resolve_field(FieldDecl * const ptr);
    void resolve_struct(StructDecl * const ptr);
    void resolve_function(FunctionDecl * const ptr, uint64_t scope_idx);
    void resolve_component(ComponentDecl * const ptr);
    void resolve_block(ScopeBody * const ptr);
    void resolve_type_decl(TypeDecl * const ptr, uint64_t scope_idx);
    void resolve_declaration(Declaration * const ptr, uint64_t scope_idx);

    void resolve_statement(Statement * const ptr, uint64_t scope_idx);
    
    // void recurse_resolve_arr_init_expr(ArrInitExpr * const ptr);
    void resolve_expression(Expression * const ptr, uint64_t scope_idx);

    // Given a symbol, gets the position in the code that the symbol points to.
    std::pair<uint32_t, uint32_t> get_symbol_pos(const Symbol *ptr) const;

    uint64_t find_symbol_idx(const std::vector<std::string> &ident_path, 
        uint64_t scope_idx, uint32_t symbol_line, 
        uint32_t symbol_col);
};

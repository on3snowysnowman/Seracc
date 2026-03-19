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

    void parse_module(ModuleDecl * const ptr);
    void parse_field(FieldDecl * const ptr);
    void parse_struct(StructDecl * const ptr);
    void parse_function(FunctionDecl * const ptr);
    void parse_component(ComponentDecl * const ptr);

    const SymbolTable * s_table_ptr = nullptr;

};

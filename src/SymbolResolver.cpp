// SymbolResolver.cpp

#include "SymbolResolver.hpp"

#include <iostream>


// Ctors / Dtors

SymbolResolver::SymbolResolver() {}


// Public

void SymbolResolver::resolve(Program &p, const SymbolTable &s_table)
{
    s_table_ptr = &s_table;
       
    parse_module(p.ast.get());

    s_table_ptr = nullptr;
}



// Private

void SymbolResolver::parse_module(ModuleDecl * const ptr)
{
    for(const std::unique_ptr<Declaration> &ptr: ptr->decls)
    {
        switch(ptr->kind)
        {
            case DeclKind::FIELD:

                break;

            case DeclKind::MODULE:

                break;
            
            case DeclKind::STRUCT:

                break;

            case DeclKind::FUNCTION:

                break;

            case DeclKind::COMPONENT:

                break;

            default:

                std::cerr << "Invalid declaration type found for: " << ptr->name
                    << '\n';
                exit(1); 
                break;
        }

    }
}

void parse_field(FieldDecl * const ptr)
{
    TypeDecl * const type_decl_ptr = ptr->type_decl.get();
}

void parse_struct(StructDecl * const ptr)
{

}

void parse_function(FunctionDecl * const ptr)
{

}

void parse_component(ComponentDecl * const ptr)
{

}


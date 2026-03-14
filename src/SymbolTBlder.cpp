// SymbolTBlder.cpp

#include "SymbolTBlder.hpp"


// Ctors / Dtor

SymbolTBlder::SymbolTBlder() {}


// Public

SymbolTable SymbolTBlder::build(const Program &p) 
{
    symbols.clear();
    scope_syms.clear();

    build_namespace(p.ast.get(), {});

    SymbolTable st;

    st.symbols = std::move(symbols);
    st.scope_syms = std::move(scope_syms);

    return st;
}

// Private

uint64_t SymbolTBlder::build_namespace(const NamespaceDecl * const ptr,
    std::optional<uint64_t> parent_scope_id)
{
    Scope s;
    s.parent_scope_id = parent_scope_id;

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {

    }

    return add_scope_assign_id(std::move(s));
}

uint64_t SymbolTBlder::add_scope_assign_id(Scope &&s)
{
    scope_syms.push_back(s);

    return scope_syms.size() - 1;
}
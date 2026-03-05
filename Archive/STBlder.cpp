// STBlder.cpp

#include "STBlder.hpp"

#include <iostream>

// Ctor / Dtors

STBlder::STBlder() {}



// Public

NamespaceSymbol* STBlder::build(const Program &p)
{
    current_prog = &p;

    NamespaceSymbol *global_ns = new NamespaceSymbol;

    global_ns->name = "global";

    current_ns = global_ns;

    for(const std::unique_ptr<Decl> &decl : p.decls)
    {
        if(auto deduced_decl = dynamic_cast<NamespaceDecl*>(decl.get()))
        {
            parse_namespace(deduced_decl);
        }

        else if(auto deduced_decl = dynamic_cast<FunctionDecl*>(decl.get()))
        {
            parse_function(deduced_decl);
        }

        else if(auto deduced_decl = dynamic_cast<StructDecl*>(decl.get()))
        {
            parse_struct(deduced_decl);
        }

        else if(auto deduced_decl = dynamic_cast<ComponentDecl*>(decl.get()))
        {
            parse_component(deduced_decl);
        }

        else
        {
            std::cerr << "Invalid declaration: " << decl->name << '\n';
            exit(1);
        }
    }

    current_prog = nullptr;
    current_ns = nullptr;

    return global_ns;
}


// Private

void STBlder::parse_namespace(const NamespaceDecl * const ptr) 
{
    std::cout << "Building Namespace: " << ptr->name << '\n';

    NamespaceSymbol *prev_ns = current_ns;

    // This namespace already exists in this namespace, simply add to it
    if(prev_ns->nspaces.find(ptr->name) != prev_ns->nspaces.end())
    {
        current_ns = prev_ns->nspaces.find(ptr->name)->second;
    }
    else // New namespace
    {
        current_ns = new NamespaceSymbol;
        current_ns->name = ptr->name;

        prev_ns->nspaces.emplace(ptr->name, current_ns);
    }
    
    // Fill new namespace
    for(const std::unique_ptr<Decl> &decl : ptr->decls)
    {
        if(auto deduced_decl = dynamic_cast<NamespaceDecl*>(decl.get()))
        {
            parse_namespace(deduced_decl);
        }

        else if(auto deduced_decl = dynamic_cast<FunctionDecl*>(decl.get()))
        {
            parse_function(deduced_decl);
        }

        else if(auto deduced_decl = dynamic_cast<StructDecl*>(decl.get()))
        {
            parse_struct(deduced_decl);
        }

        else if(auto deduced_decl = dynamic_cast<ComponentDecl*>(decl.get()))
        {
            parse_component(deduced_decl);
        }

        else
        {
            std::cerr << "Invalid declaration: " << decl->name << '\n';
            exit(1);
        }
    }

    current_ns = prev_ns;
}

void STBlder::parse_function(const FunctionDecl * const ptr) 
{
    std::cout << "Building Function: " << ptr->name << '\n'; 

    if(current_ns->functions.find(ptr->name) != current_ns->functions.end())
    {
        // MVP doesn't allow overloaded functions
        std::cerr << current_prog->file_path << ':' << ptr->line << ':' <<
            ptr->col << ": " << "Function \"" << ptr->name << "\" already " 
            "defined.\n";
        exit(1);
    }

    current_ns->functions[ptr->name].push_back(ptr);
}

void STBlder::parse_struct(const StructDecl * const ptr) 
{
    std::cout << "Building Struct: " << ptr->name << '\n'; 

    const auto &it = current_ns->types.find(ptr->name);

    // This struct has been declared already.
    if(it != current_ns->types.end())
    {
        std::cerr << current_prog->file_path << ':' << ptr->line << ':' <<
            ptr->col << ": " << "Struct \"" << ptr->name << "\" already " 
            "defined.\n";
        exit(1);
    }

    TypeSymbol sym;
    sym.kind = TypeSymbol::STRUCT;
    sym.decl_ptr = ptr;

    current_ns->types.emplace(ptr->name, sym);
}

void STBlder::parse_component(const ComponentDecl * const ptr) 
{
        std::cout << "Building Component: " << ptr->name << '\n'; 

    const auto &it = current_ns->types.find(ptr->name);

    // This struct has been declared already.
    if(it != current_ns->types.end())
    {
        std::cerr << current_prog->file_path << ':' << ptr->line << ':' <<
            ptr->col << ": " << "Component \"" << ptr->name << "\" already " 
            "defined.\n";
        exit(1);
    }

    TypeSymbol sym;
    sym.kind = TypeSymbol::COMPONENT;
    sym.decl_ptr = ptr;

    current_ns->types.emplace(ptr->name, sym);

}

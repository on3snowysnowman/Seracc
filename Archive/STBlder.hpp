// STBlder.hpp

#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

#include "ASTDeclarations.hpp"
#include "Parser.hpp"

struct TypeSymbol
{
    enum Kind
    {
        STRUCT,
        COMPONENT
    };

    Kind kind;
    const Decl *decl_ptr;
};


struct NamespaceSymbol
{
    std::string name;
    std::unordered_map<std::string, TypeSymbol> types;
    std::unordered_map<std::string, 
        std::vector<const FunctionDecl*>> functions;
    std::unordered_map<std::string, NamespaceSymbol*> nspaces;
};

class STBlder
{

public:

    STBlder();

    NamespaceSymbol* build(const Program &p);

private:

    void parse_namespace(const NamespaceDecl * const ptr);
    void parse_function(const FunctionDecl * const ptr);
    void parse_struct(const StructDecl * const ptr);
    void parse_component(const ComponentDecl * const ptr);

    NamespaceSymbol *current_ns = nullptr;

    const Program *current_prog = nullptr;
};


namespace STPrinter
{

static inline void print_namespace(const NamespaceSymbol * const ptr)
{
    std::cout << "Namespace: " << ptr->name << '\n';

    std::cout << "Functions: \n{\n";

    for(const auto &elem : ptr->functions)
    {
        std::cout << elem.first << '\n';
    }

    std::cout << "}\n\nTypes: \n{\n";

    for(const auto &elem : ptr->types)
    {
        std::cout << elem.first << '\n';
    }

    std::cout << "}\n\nNamespaces: \n{\n";

    for(const auto &elem : ptr->nspaces)
    {
        print_namespace(elem.second);
        std::cout << '\n';
    }

    std::cout << "} // namespace " << ptr->name << '\n';
}

static inline void print_symbol_table(const NamespaceSymbol * const ptr)
{
    std::cout << "--Printing out built symbol table--\n";

    print_namespace(ptr);
}

}

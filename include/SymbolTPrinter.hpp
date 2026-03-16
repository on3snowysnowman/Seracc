// SymbolTPrinter.hpp

#include <vector>

#include "SymbolTBlder.hpp"
#include "Program.hpp"
#include "Parser.hpp"

namespace SymbolTPrinter
{

static void handle_tab_print(int tab_depth)
{
    for(int i = 0; i < tab_depth; ++i)
    {
        std::cout << "   ";
    }
}

static void handle_newline(int tab_depth)
{
    std::cout << '\n';
    handle_tab_print(tab_depth);
}

void print_scope(const Scope * const ptr, const SymbolTable &sym_t,
    int tab_depth);

void print_type_symbol(const TypeSymbol * const ptr, const SymbolTable &sym_t,
    int tab_depth)
{
    std::cout << "Type Symbol:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(&sym_t.scopes.at(ptr->created_scope_idx), sym_t, tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n\n";
}

void print_fn_symbol(const FunctionSymbol * const ptr, 
    const SymbolTable &sym_t, int tab_depth)
{
    std::cout << "Function Symbol:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(&sym_t.scopes.at(ptr->created_scope_idx), sym_t, tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n\n";
}

void print_module_symbol(const ModuleSymbol * const ptr, 
    const SymbolTable &sym_t, int tab_depth)
{
    std::cout << "Module Symbol:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(&sym_t.scopes.at(ptr->created_scope_idx), sym_t, tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n\n";
}

void print_symbol(uint64_t sym_idx, const SymbolTable &sym_t, int tab_depth)
{
    const Symbol *sym = sym_t.symbols.at(sym_idx).get();

    switch(sym->sym_type)
    {
        case SymbolType::TYPE:
            
            print_type_symbol(
                static_cast<const TypeSymbol*>(sym), sym_t, tab_depth);
            break;

        case SymbolType::FN:

            print_fn_symbol(
                static_cast<const FunctionSymbol*>(sym), sym_t, tab_depth);
            break;

        case SymbolType::VAR:

            std::cout << "Var Symbol\n\n";
            break;

        case SymbolType::MODULE:

            print_module_symbol(
                static_cast<const ModuleSymbol*>(sym), sym_t, tab_depth);
            break;

        case SymbolType::FIELD:

            std::cout << "Var Field\n\n";
            break;

        default:

            std::cerr << "Invalid symbol type\n";
            exit(1);
    }
}

void print_scope(const Scope * const ptr, const SymbolTable &sym_t,
    int tab_depth)
{
    if(ptr->sym_name_to_symbol_idx.size() == 0) return;

    // handle_tab_print(tab_depth);
    // std::cout << "{";

    for(const auto &elem : ptr->sym_name_to_symbol_idx)
    {
        handle_tab_print(tab_depth);
        std::cout << elem.first << " -> ";
        print_symbol(elem.second, sym_t, tab_depth);
    }

    // handle_tab_print(tab_depth);
    // std::cout << "}\n";
}




}

void print_symT_results(const char *in_file_path)
{
    Parser parser;
    Program prog = parser.parse(in_file_path);
    
    SymbolTBlder builder;

    SymbolTable sym_t = builder.build(prog);

    if(sym_t.scopes.size() == 0) return;

    // First scope is the global scope
    SymbolTPrinter::print_scope(&sym_t.scopes.at(0), sym_t, 0);
}

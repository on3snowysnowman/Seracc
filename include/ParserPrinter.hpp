// ParserPrinter.hpp

#include "Parser.hpp"
#include "Program.hpp"

namespace ParserPrinter
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

static void print_struct(const std::unique_ptr<StructDecl> &ptr,
    int tab_depth)
{

}

static void print_function(const std::unique_ptr<FunctionDecl> &ptr,
    int tab_depth)
{
    
}

static void print_component(const std::unique_ptr<ComponentDecl> &ptr,
    int tab_depth)
{

}

static void print_statement(const std::unique_ptr<Statement> &ptr,
    int tab_depth)
{

}

static void print_expression(const std::unique_ptr<Expression> &ptr,
    int tab_depth)
{

}

static void print_type_decl(const std::unique_ptr<TypeDecl> &ptr,
    int tab_depth)
{

}

static void print_field(const FieldDecl &field, int tab_depth)
{

}

static void print_param(const ParameterDecl &param, int tab_depth)
{

}

static void print_namespace(const std::unique_ptr<NamespaceDecl> &ptr, 
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Namespace: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col << '\n';

    if(ptr->struct_decls.size() > 0)
    {
        for(const std::unique_ptr<StructDecl> &decl : ptr->struct_decls)
        {
            print_struct(decl, tab_depth + 3);
        }

        std::cout << '\n';
    }

    if(ptr->component_decls.size() > 0)
    {
        for(const std::unique_ptr<ComponentDecl> &decl : ptr->component_decls)
        {
            print_component(decl, tab_depth + 3);
        }

        std::cout << '\n';
    }

    if(ptr->function_decls.size() > 0)
    {
        for(const std::unique_ptr<FunctionDecl> &decl : ptr->function_decls)
        {
            print_function(decl, tab_depth + 3);
        }

        std::cout << '\n';
    }

    if(ptr->namespace_decls.size() > 0)
    {
        for(const std::unique_ptr<NamespaceDecl> &decl : ptr->namespace_decls)
        {
            print_namespace(decl, tab_depth + 3);
        }

        std::cout << '\n';
    }
}

}

static void print_parse_results(const char *in_file_path)
{
    Parser pars;
    Program prog = pars.parse(in_file_path);

    ParserPrinter::print_namespace(prog.ast, 0);
}

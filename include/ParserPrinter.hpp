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

static void print_type_decl(const TypeDecl * const ptr,
    int tab_depth)
{
    (void)ptr;
    (void)tab_depth;
}

static void print_field(const FieldDecl * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Field: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "{";
    handle_newline(tab_depth);
}

static void print_param(const Parameter &param, int tab_depth)
{
    (void)param;
    (void)tab_depth;
}

static void print_struct(const StructDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Struct: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "{";
    handle_newline(tab_depth);

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::FIELD:

                print_field(
                    reinterpret_cast<FieldDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::STRUCT:

                print_struct(
                    reinterpret_cast<StructDecl*>(decl.get()),
                    tab_depth + 1);
                break;
        }
    }
}

static void print_statement(const Statement * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Statement: " << static_cast<int>(ptr->stmt_type);
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "{";
    handle_newline(tab_depth);
}

static void print_expression(const Expression * const ptr,
    int tab_depth)
{
    (void)ptr;
    (void)tab_depth;
}

static void print_scope(const ScopeBody &s, int tab_depth)
{
    
}

static void print_function(const FunctionDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Function: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Public: " << ptr->is_pub;
    std::cout << "Return type: " << print_type_decl(ptr->ret_type.get());
    handle_newline(tab_depth);
    
    if(ptr->params.size() > 0)
    {
        std::cout << "Parameters:";
        handle_newline(tab_depth);
        std::cout << "{\n";

        for(const Parameter &p : ptr->params)
        {
            print_param(p, tab_depth + 1);
        }

        std::cout << "}";
        handle_newline(tab_depth);
    }

    std::cout << "{";
    handle_newline(tab_depth);

    std::cout << "}\n";
}

static void print_component(const ComponentDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Component: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "{";
    handle_newline(tab_depth);
}

static void print_namespace(const NamespaceDecl * const ptr, 
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Namespace: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "{";
    handle_newline(tab_depth);

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::FIELD:

                print_field(
                    reinterpret_cast<FieldDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::NAMESPACE:

                print_namespace(
                    reinterpret_cast<NamespaceDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::STRUCT:

                print_struct(
                    reinterpret_cast<StructDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::FUNCTION:

                print_function(
                    reinterpret_cast<FunctionDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::COMPONENT:

                print_component(
                    reinterpret_cast<ComponentDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            default:

                std::cerr << "Invalid declaration type\n";
                exit(1);
        }
    }

    std::cout << "}\n";
}

}

static void print_parse_results(const char *in_file_path)
{
    Parser pars;
    Program prog = pars.parse(in_file_path);

    ParserPrinter::print_namespace(prog.ast.get(), 0);
}

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

static void print_named_type(const NamedTypeDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Name: " << ptr->type_name << '\n';
}

static void print_type_decl(const TypeDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Type Kind: ";

    switch(ptr->kind)
    {
        case TypeKind::INVALID:

            std::cout << "INVALID\n";
            break;

        case TypeKind::NAMED:

            std::cout << "NAMED\n";
            print_named_type(reinterpret_cast<const NamedTypeDecl*>(ptr), 
                tab_depth);
            break;

        case TypeKind::PTR:
        {
            const PtrTypeDecl * const reint_ptr = 
                reinterpret_cast<const PtrTypeDecl*>(ptr);

            std::cout << "PTR";
            handle_newline(tab_depth);
            std::cout << "Points to mutable: " << reint_ptr->points_to_mutable;
            handle_newline(tab_depth);
            std::cout << "Pointed to type: ";
            handle_newline(tab_depth);
            std::cout << "{\n";
            print_type_decl(reint_ptr->pointee.get(), tab_depth + 1);
            handle_tab_print(tab_depth);
            std::cout << "}\n";
            break;
        }
        
        case TypeKind::REF:
        {
            const RefTypeDecl * const reint_ptr = 
                reinterpret_cast<const RefTypeDecl*>(ptr);

            std::cout << "REF";
            handle_newline(tab_depth);
            std::cout << "Reference to mutable: " << 
                reint_ptr->ref_to_mutable;
            handle_newline(tab_depth);
            std::cout << "Referenced to type: ";
            handle_newline(tab_depth);
            std::cout << "{\n";
            print_type_decl(reint_ptr->referred.get(), tab_depth + 1);
            handle_tab_print(tab_depth);
            std::cout << "}\n";
            break;
        }

        case TypeKind::ARRAY:

            std::cout << "ARRAY\n";
            break;

        case TypeKind::FUNC_PTR:

            std::cout << "FUNC_PTR\n";
            break;

        default:

            std::cout << "UNKNOWN TYPE\n";
            break;
    }
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
    
    
    if(ptr->decls.size() == 0)
    {
        std::cout << '\n';
        return;
    }
    
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

            default:

                std::cout << "Invalid declaration type: "
                    << static_cast<int>(decl->kind) << '\n';
        }
    }

    handle_newline(tab_depth);
    std::cout << "}\n";
}

// static void print_statement(const Statement * const ptr,
//     int tab_depth)
// {
//     handle_tab_print(tab_depth);
//     std::cout << "Statement: " << static_cast<int>(ptr->stmt_type);
//     handle_newline(tab_depth);
//     std::cout << "Line: " << ptr->line;
//     handle_newline(tab_depth);
//     std::cout << "Col: " << ptr->col;
//     handle_newline(tab_depth);
//     std::cout << "{";
//     handle_newline(tab_depth);
// }

// static void print_expression(const Expression * const ptr,
//     int tab_depth)
// {
//     (void)ptr;
//     (void)tab_depth;
// }

static void print_var_decl(const VarDeclStmt * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Variable Declaration:";
    handle_newline(tab_depth);
    std::cout << "Name: " << ptr->var_name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Binding Mutable: " << ptr->is_binding_mutable;
    handle_newline(tab_depth);
    std::cout << "Type:";
    handle_newline(tab_depth);
    std::cout << "{\n"; 
    print_type_decl(ptr->type_decl.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_scope(const ScopeBody &s, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << s.line;
    handle_newline(tab_depth);
    std::cout << "Col: " << s.col;
    
    if(s.statements.size() > 0)
    {
        handle_newline(tab_depth);
        std::cout << "Statements:";
        handle_newline(tab_depth);
        std::cout << "{";

        for(const std::unique_ptr<Statement> &s : s.statements)
        {
            std::cout << '\n';

            switch(s->stmt_type)
            {   
                case StatementType::BLOCK:

                    std::cout << "Block";
                    break;

                case StatementType::COMPONENT_DECL:

                    std::cout << "Component Decl";
                    break;

                case StatementType::EXPR:

                    std::cout << "Expression";
                    break;

                case StatementType::FOR:

                    std::cout << "For";
                    break;

                case StatementType::IF:

                    std::cout << "If";
                    break;

                case StatementType::RETURN:

                    std::cout << "Return";
                    break;

                case StatementType::STRUCT_DECL:

                    std::cout << "Struct Decl";
                    break;

                case StatementType::VAR_DECL:

                    print_var_decl(
                        reinterpret_cast<const VarDeclStmt*>(s.get()), 
                        tab_depth + 1);
                    break;

                case StatementType::WHILE:

                    std::cout << "While";
                    break;

                default:

                    std::cout << "Invalid";
                    break;
            }
        }

        handle_newline(tab_depth);
        std::cout << "}";
    }

    std::cout << '\n';
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
    handle_newline(tab_depth);
    std::cout << "Return type:";
    handle_newline(tab_depth);
    std::cout << "{\n"; 
    print_type_decl(ptr->ret_type.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
    // handle_tab_print(tab_depth);
    
    if(ptr->params.size() > 0)
    {
        std::cout << "Parameters:";
        handle_newline(tab_depth);
        std::cout << "{\n";

        for(const Parameter &p : ptr->params)
        {
            print_param(p, tab_depth + 1);
        }

        handle_newline(tab_depth);
        std::cout << "}\n";
    }

    handle_tab_print(tab_depth);
    std::cout << "Body:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(ptr->body, tab_depth + 1);
    handle_tab_print(tab_depth);
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
    std::cout << "Declarations:";
    handle_newline(tab_depth);
    std::cout << "{\n";

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
        std::cout << "\n";
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

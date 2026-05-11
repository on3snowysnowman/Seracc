// TypeChecker.cpp

#include "TypeChecker.hpp"

#include <iostream>


// Ctors / Dtor

TypeChecker::TypeChecker() {}

TypeChecker::~TypeChecker() {}


// Public

void TypeChecker::type_check(const Program &p, const SymbolTable &sym_table)
{
    program = &p;
    s_table = &sym_table;

    check_module(p.ast.get());
}

// Private

void TypeChecker::print_error_location(uint32_t line, uint32_t col) const
{
    std::cerr << program->source_file_name << ':' << line << ':' << col;
}

uint64_t TypeChecker::resolve_type_decl(const TypeDecl * ptr) const
{
    switch(ptr->kind)
    {
        // case TypeKind::ARRAY:
        // {
        //     const ArrTypeDecl *reint_ptr = 
        //         static_cast<const ArrTypeDecl*>(ptr);

        //     return resolve_type_decl(reint_ptr->element_type.get());
        // }

        // case TypeKind::FUNC_PTR:
        // {
            
        // }

        case TypeKind::NAMED:
        {
            const NamedTypeDecl *reint_ptr = 
                static_cast<const NamedTypeDecl*>(ptr);

            return reint_ptr->resolved_symbol_idx.value();
        }

        // case TypeKind::PTR:
        // {
        //     break;
        // }

        case TypeKind::REF:
        {
            const RefTypeDecl *reint_ptr =
                static_cast<const RefTypeDecl*>(ptr);

            return resolve_type_decl(reint_ptr->referred.get());
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid kind found for TypeDecl\n";
            exit(1);
    }   
}

void TypeChecker::check_module(const ModuleDecl * ptr)
{   
    std::cerr << "Checking Module: " << ptr->ident << '\n';

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
        check_decl(decl.get());
}

void TypeChecker::check_component(const ComponentDecl * ptr)
{
    std::cerr << "Checking Component: " << ptr->name << '\n';

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
        check_decl(decl.get(), ptr->symbol_idx.value());
}

void TypeChecker::check_function(const FunctionDecl * ptr,
    std::optional<uint64_t> owning_component_sym_idx)
{
    std::cerr << "Checking Function: " << ptr->name << '\n';

    // This function has a receiver component
    if(ptr->receiver_data.has_value())
    {
        // This function is not defined inside a Component.
        if(!owning_component_sym_idx.has_value())
        {
            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Function with receiver component declared outside"
                " of component body.\n";
            exit(1);
        }

        if(ptr->receiver_data->type_decl->kind != TypeKind::REF)
        {
            print_error_location(ptr->receiver_data->type_decl->line,
                ptr->receiver_data->type_decl->col);
            std::cerr << " -> Receiver components must be reference types.\n";
            exit(1);
        }

        const RefTypeDecl * receiver_type_decl = static_cast<const RefTypeDecl*>(
                ptr->receiver_data->type_decl.get());

        if(receiver_type_decl->referred->kind != TypeKind::NAMED)
        {
            print_error_location(receiver_type_decl->line, 
                receiver_type_decl->col);
            std::cerr << " -> Receiver component must be a reference to a named"
                " type\n";
            exit(1);
        }

        // Check that the receiver component symbol idx matches that of the 
        // owning component.
        if(*owning_component_sym_idx != 
            resolve_type_decl(ptr->receiver_data->type_decl.get()))
        {
            print_error_location(ptr->receiver_data->type_decl->line, 
                ptr->receiver_data->type_decl->col);
            std::cerr << " -> Receiver Component type does not match the owning"
                " Component type\n";
            exit(1);
        }
    }

    for(const Parameter & param : ptr->params)
        check_param(&param);

    check_scope_body(&ptr->body, ptr->ret_type.get());
}

void TypeChecker::check_scope_body(const ScopeBody * ptr,
    std::optional<const TypeDecl*> expected_return_type)
{
    std::cerr << "Checking Scope Body\n";

    for(const std::unique_ptr<Statement> &stmt : ptr->statements)
        check_statement(stmt.get(), expected_return_type);
}

void TypeChecker::check_statement(const Statement * ptr,
    std::optional<const TypeDecl*> expected_return_type)
{
    std::cerr << "Checking Statement of type: " << ptr->stmt_type << '\n';

    switch(ptr->stmt_type)
    {
        // case StatementType::EXPR:
        // {
        //     break;
        // }

        // case StatementType::VAR_DECL:
        // {
        //     break;
        // }

        // case StatementType::STRUCT_DECL:
        // {
        //     break;
        // }

        // case StatementType::COMPONENT_DECL:
        // {
        //     break;
        // }

        // case StatementType::RETURN:
        // {
        //     const RetStmt *reint_ptr = static_cast<const RetStmt*>(ptr); 

        //     // If we are not expecting a return expression but we have one.
        //     if(!expected_return_type.has_value() && 
        //         reint_ptr->ret_expr != nullptr)
        //     {
        //         print_error_location(ptr->line, ptr->col);
        //         std::cerr << " -> Unexpected return expression for function "
        //             "with void return type.";
        //     }

        //     // If we have no return expression.
        //     if(reint_ptr->ret_expr == nullptr)
        //     {
        //         // Make sure that the return type is void.

        //         const uint64_t void_symbol_idx = 
        //             s_table->builtin_to_id.at("void");

        //         // If the expected return type is not void 
        //         if
        //         (
        //             (*expected_return_type)->kind != TypeKind::NAMED || 
        //             static_cast<const NamedTypeDecl*>(*expected_return_type)->
        //                 resolved_symbol_idx != void_symbol_idx
        //         )
        //         {
        //             print_error_location(ptr->line, ptr->col);
        //             std::cerr << " -> Expected a return expression for "
        //                 "function with non void return type.\n";
        //         }
        //     }
            


        //     break;
        // }

        // case StatementType::IF:
        // {
        //     break;
        // }

        // case StatementType::WHILE:
        // {
        //     break;
        // }

        // case StatementType::FOR:
        // {
        //     break;
        // }

        // case StatementType::BLOCK:
        // {
        //     break;
        // }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid StatementType found.\n";
            exit(1);
    }
}

void TypeChecker::check_param(const Parameter * ptr)
{
    std::cerr << "Checking Parameter: " << ptr->name << '\n';
    
    std::cerr << "Not implemented\n";
    exit(1);
}

const TypeDecl* TypeChecker::check_expression(const Expression *ptr)
{
    std::cerr << "Checking expression of type: " << ptr->exp_type << '\n';

    switch(ptr->exp_type)
    {       
        case ExpressionType::BIN_LITERAL:
        {
            break;
        }

        case ExpressionType::HEX_LITERAL:
        {
            break;
        }

        case ExpressionType::INT_LITERAL:
        {
            break;
        }

        case ExpressionType::FLOAT_LITERAL:
        {
            break;
        }

        case ExpressionType::STR_LITERAL:
        {
            break;
        }

        case ExpressionType::CHAR_LITERAL:
        {
            break;
        }

        case ExpressionType::BOOL_LITERAL:
        {
            break;
        }

        case ExpressionType::NULLPTR_LITERAL:
        {
            break;
        }

        case ExpressionType::IDENTIFIER:
        {
            break;
        }

        case ExpressionType::STRUCT_INIT:
        {
            break;
        }

        case ExpressionType::ARR_INIT:
        {
            break;
        }

        case ExpressionType::UNARY:
        {
            break;
        }

        case ExpressionType::BINARY:
        {
            break;
        }

        case ExpressionType::TERNARY:
        {
            break;
        }

        case ExpressionType::ASSIGN:
        {
            break;
        }

        case ExpressionType::CALL:
        {
            break;
        }

        case ExpressionType::CAST:
        {
            break;
        }

        case ExpressionType::SUBSCRIPT:
        {
            break;
        }

        case ExpressionType::MEMBER_ACCESS:
        {
            break;
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid ExpressionType found\n";
            exit(1);
    }
}

void TypeChecker::check_decl(const Declaration * ptr, 
    std::optional<uint64_t> owning_component_sym_idx)
{
    std::cerr << "Checking Declaration of type: " << ptr->kind << '\n';

    switch(ptr->kind)
    {
        case DeclKind::COMPONENT:
            check_component(static_cast<const ComponentDecl*>(ptr));
            break;

        // case DeclKind::FIELD:
        //     break;

        case DeclKind::FUNCTION:
            check_function(static_cast<const FunctionDecl*>(ptr),
                owning_component_sym_idx);
            break;
        
        case DeclKind::MODULE:
            check_module(static_cast<const ModuleDecl*>(ptr));
            break;

        // case DeclKind::STRUCT:
        //     break;

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid DeclKind found\n";
            exit(1);
    }
}

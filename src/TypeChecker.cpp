// TypeChecker.cpp

#include "TypeChecker.hpp"

#include <iostream>


// Ctors / Dtor

TypeChecker::TypeChecker() {}


// Public

void TypeChecker::type_check(const Program &p, const SymbolTable &sym_table)
{
    parsed_file = p.source_file_name;
    s_table = &sym_table;

    check_module(p.ast.get());

    parsed_file = nullptr;
    s_table = nullptr;
}


// Private

void TypeChecker::check_module(ModuleDecl * const ptr)
{
    for(const auto &decl : ptr->decls)
        check_declaration(decl.get());
}

void TypeChecker::check_struct(StructDecl * const ptr)
{
    for(const auto &decl: ptr->decls)
        check_declaration(decl.get());
}

void TypeChecker::check_component(ComponentDecl * const ptr)
{
    for(const auto &decl: ptr->decls)
        check_declaration(decl.get());
}

void TypeChecker::check_function(FunctionDecl * const ptr)
{
    for(Parameter &p : ptr->params)
        check_param(&p);
}

void TypeChecker::check_block(ScopeBody * const ptr)
{
    for(const auto &stmt: ptr->statements)
        check_statement(stmt.get());
}

uint64_t TypeChecker::resolve_type_from_expr(Expression * const ptr)
{
    switch(ptr->exp_type)
    {
        case ExpressionType::ASSIGN:

            break;

        case ExpressionType::BIN_LITERAL:

            break;

        case ExpressionType::BINARY:

            break;

        case ExpressionType::BOOL_LITERAL:

            break;

        case ExpressionType::CALL:

            break;

        case ExpressionType::CAST:

            break;

        case ExpressionType::CHAR_LITERAL:

            break;

        case ExpressionType::FLOAT_LITERAL:

            break;

        case ExpressionType::HEX_LITERAL:

            break;

        case ExpressionType::IDENTIFIER:

            break;

        case ExpressionType::INT_LITERAL:

            break;

        case ExpressionType::MEMBER_ACCESS:

            break;

        case ExpressionType::NULLPTR_LITERAL:

            break;

        case ExpressionType::STR_LITERAL:

            break;

        case ExpressionType::STRUCT_INIT:

            break;

        case ExpressionType::SUBSCRIPT:

            break;

        case ExpressionType::TERNARY:

            break;

        case ExpressionType::UNARY:

            break;

        default:

            break;
    }

    return 0;
}


void TypeChecker::check_field(FieldDecl * const ptr)
{
    ;
}

void TypeChecker::check_param(Parameter * const ptr)
{
    ;
}

void TypeChecker::check_var_decl(VarDeclStmt * const ptr)
{
    ;
}

void TypeChecker::check_type_decl(TypeDecl * const ptr)
{
    ;
}

void TypeChecker::check_declaration(Declaration * const ptr)
{   
    switch(ptr->kind)
    {
        case DeclKind::COMPONENT:

            check_component(static_cast<ComponentDecl*>(ptr));
            break;

        case DeclKind::FIELD:

            check_field(static_cast<FieldDecl*>(ptr));
            break;

        case DeclKind::FUNCTION:

            check_function(static_cast<FunctionDecl*>(ptr));
            break;

        case DeclKind::MODULE:

            check_module(static_cast<ModuleDecl*>(ptr));
            break;

        case DeclKind::STRUCT:

            check_struct(static_cast<StructDecl*>(ptr));
            break;

        default:

            std::cerr << parsed_file << ':' << ptr->line << ':' << 
                ": Invalid declaration kind.\n";
            exit(1);
    }
}

void TypeChecker::check_statement(Statement * const ptr)
{
    switch(ptr->stmt_type)
    {
        case StatementType::BLOCK:

            check_block(&static_cast<BlockStmt*>(ptr)->block_decl);
            break;

        case StatementType::COMPONENT_DECL:

            check_component(static_cast<ComponentDeclStmt*>(ptr)->decl.get());
            break;

        case StatementType::EXPR:

            check_expression(static_cast<ExprStmt*>(ptr)->expr.get());
            break;

        case StatementType::FOR:
        {
            ForStmt * const reint_ptr = 
                static_cast<ForStmt*>(ptr);

            check_expression(reint_ptr->condition_expr.get());
            check_expression(reint_ptr->increment_expr.get());
            check_statement(reint_ptr->init_stmt.get());
            check_block(&reint_ptr->body);
            break;
        }

        case StatementType::IF:
        {
            IfStmt * const reint_ptr =
                static_cast<IfStmt*>(ptr);

            check_expression(reint_ptr->condition_expr.get());
            check_block(&reint_ptr->then_body);
            
            if(reint_ptr->else_branch != nullptr)
            {
                check_statement(reint_ptr->else_branch.get());
            }

            break;
        }
                
        case StatementType::RETURN:

            check_expression(static_cast<RetStmt*>(ptr)->ret_expr.get());
            break;

        case StatementType::STRUCT_DECL:

            check_struct(static_cast<StructDeclStmt*>(ptr)->decl.get());
            break;

        case StatementType::VAR_DECL:
        
            check_var_decl(static_cast<VarDeclStmt*>(ptr));
            break;

        case StatementType::WHILE:
        {
            WhileStmt * const reint_ptr =   
                static_cast<WhileStmt*>(ptr);

            check_expression(reint_ptr->condition_expr.get());
            check_block(&reint_ptr->body);
            break;
        }
            
    default:

        break;
    }
}

void TypeChecker::check_expression(Expression * const ptr)
{

}

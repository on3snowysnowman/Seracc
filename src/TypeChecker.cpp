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

void TypeChecker::print_error_location(uint32_t line, uint32_t col) const
{
    std::cerr << parsed_file << ":" << line << ':' << col;
}

uint64_t TypeChecker::get_resolved_idx_from_type_decl(
    const TypeDecl * const ptr) const
{
    switch(ptr->kind)
    {
        case TypeKind::ARRAY:
        {
            const ArrTypeDecl * const reint_ptr = 
                static_cast<const ArrTypeDecl*>(ptr);

            return get_resolved_idx_from_type_decl(
                reint_ptr->element_type.get());
        }

        case TypeKind::FUNC_PTR:
        {
            std::cerr << "Func ptr not implemented for type decl symbol "
                "fetching\n";
            exit(1);
        }

        case TypeKind::NAMED:
        {
            const NamedTypeDecl * const reint_ptr = 
                static_cast<const NamedTypeDecl*>(ptr);

            if(!reint_ptr->resolved_symbol_idx.has_value())
            {
                print_error_location(reint_ptr->line , reint_ptr->col);
                std::cerr << ": Attempted to get resolved index of named type "
                    "decl but it doesn't have one.\n";
                exit(1);
            }

            return reint_ptr->resolved_symbol_idx.value();
        };

        case TypeKind::PTR:
        {
            const PtrTypeDecl * const reint_ptr = 
                static_cast<const PtrTypeDecl*>(ptr);

            return get_resolved_idx_from_type_decl(reint_ptr->pointee.get());
        }

        case TypeKind::REF:
        {
            const RefTypeDecl * const reint_ptr = 
                static_cast<const RefTypeDecl*>(ptr);

            return get_resolved_idx_from_type_decl(reint_ptr->referred.get());
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << ": Invalid TypeKind for TypeDecl\n";
            exit(1);
    }
}

void TypeChecker::check_module(ModuleDecl * const ptr)
{
    std::cerr << "Checking: " << ptr->ident << '\n';

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
        check_declaration(decl.get(), std::optional<ComponentDecl*>{ptr});
}

void TypeChecker::check_function(FunctionDecl * const ptr, 
    std::optional<ComponentDecl*> enclosing_component)
{
    std::cerr << "Checking function: " << ptr->name << '\n';

    if(ptr->receiver_data.has_value())
    {
        std::cerr << "Checking receiver value\n";

        if(!enclosing_component.has_value())
        {
            print_error_location(ptr->receiver_data->type_decl->line,
                ptr->receiver_data->type_decl->col);
            std::cerr << ": Receiver function declared outside of Component "
                "scope\n";
            exit(1);
        }

        if(get_resolved_idx_from_type_decl(ptr->receiver_data->type_decl.get()) 
            != enclosing_component.value()->symbol_idx)
        {
            print_error_location(ptr->receiver_data->type_decl->line,
                ptr->receiver_data->type_decl->col);
            std::cerr << ": Component receiver object type does not match the "
            "enclosing Component type.\n";
            exit(1);
        }
    }

    for(Parameter &p : ptr->params)
        check_param(&p);

    check_block(&ptr->body, std::optional<TypeDecl*>(ptr->ret_type.get()));
}

void TypeChecker::check_block(ScopeBody * const ptr, 
    std::optional<const TypeDecl*> expected_ret_type)
{
    std::cerr << "Checking block\n";

    for(const auto &stmt: ptr->statements)
        check_statement(stmt.get(), expected_ret_type);
}

// uint64_t TypeChecker::resolve_type_from_expr(Expression * const ptr)
// {
//     switch(ptr->exp_type)
//     {
//         case ExpressionType::ASSIGN:

//             break;

//         case ExpressionType::BIN_LITERAL:

//             break;

//         case ExpressionType::BINARY:

//             break;

//         case ExpressionType::BOOL_LITERAL:

//             break;

//         case ExpressionType::CALL:

//             break;

//         case ExpressionType::CAST:

//             break;

//         case ExpressionType::CHAR_LITERAL:

//             break;

//         case ExpressionType::FLOAT_LITERAL:

//             break;

//         case ExpressionType::HEX_LITERAL:

//             break;

//         case ExpressionType::IDENTIFIER:

//             break;

//         case ExpressionType::INT_LITERAL:

//             break;

//         case ExpressionType::MEMBER_ACCESS:

//             break;

//         case ExpressionType::NULLPTR_LITERAL:

//             break;

//         case ExpressionType::STR_LITERAL:

//             break;

//         case ExpressionType::STRUCT_INIT:

//             break;

//         case ExpressionType::SUBSCRIPT:

//             break;

//         case ExpressionType::TERNARY:

//             break;

//         case ExpressionType::UNARY:

//             break;

//         default:

//             break;
//     }

//     return 0;
// }


void TypeChecker::check_field(FieldDecl * const ptr)
{
    const FieldSymbol * f_sym = 
        static_cast<FieldSymbol*>(s_table->symbols.at(*ptr->symbol_idx).get());

    // If this field has an init expression.
    if(ptr->init_expr != nullptr)
    {
        if(s_table->scopes.at(*f_sym->scope_idx).owning_symbol_type != 
            SymbolType::MODULE)
        {
            print_error_location(ptr->line, ptr->col);
            std::cerr << ": This field declaration cannot have an "
                "initialization expression.\n";
            exit(1);
        }

        // This is a global variable with an init expression.
        check_expression(ptr->init_expr.get());
    }

    
}

void TypeChecker::check_param(Parameter * const ptr)
{
    std::cerr << "Check param\n";
}

void TypeChecker::check_var_decl(VarDeclStmt * const ptr)
{   
    std::cerr << "Checking Variable declaration\n";
    const TypeDecl * const variable_type = 
        ptr->type_decl.get();

    if(ptr->init_expr != nullptr)
    {
        std::cerr << "Checking declaration init expression\n";

        const TypeDecl * const init_expr_type = 
            check_expression(ptr->init_expr.get());

        cmp_type_decls(variable_type, init_expr_type);
    }
}

void TypeChecker::check_type_decl(TypeDecl * const ptr)
{
    ;
}

void TypeChecker::check_declaration(Declaration * const ptr,
    std::optional<ComponentDecl*> enclosing_component)
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

            check_function(static_cast<FunctionDecl*>(ptr), 
                enclosing_component);
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

void TypeChecker::check_statement(Statement * const ptr,
    std::optional<const TypeDecl*> expected_ret_type)
{
    std::cerr << "Checking statement of type: ";

    switch(ptr->stmt_type)
    {
        case StatementType::BLOCK:

            std::cerr << "Block\n";
            check_block(&static_cast<BlockStmt*>(ptr)->block_decl, 
                expected_ret_type);
            break;

        case StatementType::COMPONENT_DECL:

            std::cerr << "Component\n";
            check_component(static_cast<ComponentDeclStmt*>(ptr)->decl.get());
            break;

        case StatementType::EXPR:

            std::cerr << "Expression\n";
            check_expression(static_cast<ExprStmt*>(ptr)->expr.get());
            break;

        case StatementType::FOR:
        {
            std::cerr << "For\n";
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
            std::cerr << "If\n";
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
        {
            std::cerr << "Return\n";
            // We are not expecting a return statement.
            if(!expected_ret_type.has_value())
            {
                print_error_location(ptr->line, ptr->col);
                std::cerr << ": Unexpected return statement.\n";
                exit(1);
            }

            const TypeDecl * got_ret_type = 
                check_expression(static_cast<RetStmt*>(ptr)->ret_expr.get());

            cmp_type_decls(got_ret_type, *expected_ret_type);
            break;
        }

        case StatementType::STRUCT_DECL:

            std::cerr << "Struct\n";
            check_struct(static_cast<StructDeclStmt*>(ptr)->decl.get());
            break;

        case StatementType::VAR_DECL:
        
            std::cerr << "Var declaration\n";
            check_var_decl(static_cast<VarDeclStmt*>(ptr));
            break;

        case StatementType::WHILE:
        {
            std::cerr << "While\n";
            WhileStmt * const reint_ptr =   
                static_cast<WhileStmt*>(ptr);

            check_expression(reint_ptr->condition_expr.get());
            check_block(&reint_ptr->body);
            break;
        }
            
    default:

        print_error_location(ptr->line, ptr->col);
        std::cerr << ": Invalid StatementType found\n";
        exit(1);
    }
}

void TypeChecker::print_type_decl(const TypeDecl* decl)
{
    switch(decl->kind)
    {
        case TypeKind::ARRAY:
        {
            const ArrTypeDecl *reint_ptr = 
                static_cast<const ArrTypeDecl*>(decl); 

            print_type_decl(reint_ptr->element_type.get());

            for(int i = 0; i < reint_ptr->depth; ++i)
            {
                std::cerr << "[]";
            }

            break;
        }

        case TypeKind::FUNC_PTR:
        {

            break;
        }

        case TypeKind::NAMED:
        {
            const NamedTypeDecl *reint_ptr = 
                static_cast<const NamedTypeDecl*>(decl); 

            break;
        }

        case TypeKind::PTR:
        {

            break;
        }

        case TypeKind::REF:
        {

            break;
        }

        default:

            
            std::cerr << "Invalid type found for TypeDecl:";
            break;
    }
}

void TypeChecker::cmp_type_decls(const TypeDecl *first, const TypeDecl *second)
{
    std::cerr << "Comparing type decls: ";
    print_type_decl(first);
    std::cerr << " and ";
    print_type_decl(second);
    std::cerr << '\n';

    if(first->kind != second->kind)
    {
        print_error_location(second->line, second->col);
        std::cerr << ": Got type: \"";
        print_type_decl(first);
        std::cerr << "\" expected type: \"";
        print_type_decl(second);
        std::cerr << "\"\n";
        exit(1);
    }

    switch(first->kind)
    {
        case TypeKind::ARRAY:
        {
            const ArrTypeDecl * reint_first = 
                static_cast<const ArrTypeDecl*>(first);
            const ArrTypeDecl * reint_second = 
                static_cast<const ArrTypeDecl*>(second);

            if(reint_first->depth != reint_second->depth)
            {
                print_error_location(reint_second->line, reint_second->line);
                std::cerr << "Expected array type of dimension: \"" << 
                    reint_first->depth << "\" but got an array type of "
                    "dimension: \"" << reint_second->depth << "\"\n";
                exit(1);
            }
            
            

            break;
        }

        case TypeKind::FUNC_PTR:
        {


            break;
        }

        case TypeKind::INVALID:
        {
            break;
        }

        case TypeKind::NAMED:
        {
            break;
        }

        case TypeKind::PTR:

            break;

        case TypeKind::REF:

            break;

        default:

            print_error_location(first->line, first->col);
            std::cerr << ": Invalid TypeKind found.\n";
            exit(1);
    }
}

const TypeDecl* TypeChecker::check_expression(const Expression * const ptr)
{
    std::cerr << "Checking expression of type: ";

    switch(ptr->exp_type)
    {
        case ExpressionType::ASSIGN:
        {
            std::cerr << "Assignment\n";
            const AssignExpr * const reint_ptr = 
                static_cast<const AssignExpr*>(ptr);

            const TypeDecl * lhs_type = 
                check_expression(reint_ptr->lhs.get());
            const TypeDecl * rhs_type = 
                check_expression(reint_ptr->rhs.get());

            // Make sure the rhs matches the type of the lhs
            cmp_type_decls(lhs_type, rhs_type);

            break;
        }

        case ExpressionType::BIN_LITERAL:
        {
            
            
            break;
        }

        case ExpressionType::BINARY:
        {
            break;
        }

        case ExpressionType::BOOL_LITERAL:
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

        case ExpressionType::CHAR_LITERAL:
        {
            break;
        }

        case ExpressionType::FLOAT_LITERAL:
        {
            break;
        }

        case ExpressionType::HEX_LITERAL:
        {
            break;
        }

        case ExpressionType::IDENTIFIER:
        {
            break;
        }

        case ExpressionType::INT_LITERAL:
        {
            break;
        }

        case ExpressionType::MEMBER_ACCESS:
        {
            break;
        }

        case ExpressionType::NULLPTR_LITERAL:
        {
            break;
        }

        case ExpressionType::STR_LITERAL:
        {
            break;
        }

        case ExpressionType::STRUCT_INIT:
        {
            break;
        }

        case ExpressionType::SUBSCRIPT:
        {
            break;
        }

        case ExpressionType::TERNARY:
        {
            break;
        }

        case ExpressionType::UNARY:
        {
            break;
        }

        default: 
        
            print_error_location(ptr->line, ptr->col);
            std::cerr << ": Invalid type found for expression\n";
            exit(1);
    }

    return nullptr;
}

// TypeChecker.cpp

#include "TypeChecker.hpp"

#include <iostream>
#include <charconv>


// Ctors / Dtor

TypeChecker::TypeChecker() {}

TypeChecker::~TypeChecker() 
{
    for(TypeDecl *decl : decls_to_delete)
    {
        delete decl;
    }
}


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
    std::cout << program->source_file_name << ':' << line << ':' << col;
}

void TypeChecker::print_type(const TypeDecl *ptr) const
{
    switch(ptr->kind)
    {
        case TypeKind::ARRAY:
        {
            const ArrTypeDecl *reint_ptr = static_cast<const ArrTypeDecl*>(ptr);

            print_type(reint_ptr->element_type.get());

            for(uint8_t i = 0; i < reint_ptr->depth; ++i)
            {
                std::cout << "[]";
            }

            break;
        }

        // I don't want to deal with this right now.
        // case TypeKind::FUNC_PTR:
        // {
        //     const FuncPtrDecl *reint_ptr = static_cast<const FuncPtrDecl*>(ptr);

        //     break;
        // }

        case TypeKind::NAMED:
        {
            const NamedTypeDecl *reint_ptr = 
                static_cast<const NamedTypeDecl*>(ptr);

            for(size_t i = 0; i < reint_ptr->ident_path.size(); ++i)
            {
                std::cout << reint_ptr->ident_path[i];

                if(i < reint_ptr->ident_path.size() - 1)
                    std::cout << "::";
            }

            break;
        }

        case TypeKind::PTR:
        {
            const PtrTypeDecl *reint_ptr = 
                static_cast<const PtrTypeDecl*>(ptr);

            if(reint_ptr->builtin_type.has_value())
            {
                switch(*reint_ptr->builtin_type)
                {
                    case BuiltinPtrType::CSTR_PTR:

                        std::cout << "CStr_ptr";
                        return;

                    case BuiltinPtrType::OPAQUE_PTR:

                        std::cout << "Opaque_ptr";
                        return;

                    case BuiltinPtrType::NULL_PTR:

                        std::cout << "Null_ptr";
                        return;
                }
            }

            std::cout << '*';
            if(reint_ptr->points_to_mutable) std::cout << "mut";
            std::cout << ' ';
            print_type(reint_ptr->pointee.get());
            break;
        }

        case TypeKind::REF:
        {
            const RefTypeDecl *reint_ptr = 
                static_cast<const RefTypeDecl*>(ptr);

            std::cout << "ref ";
            if(reint_ptr->ref_to_mutable) std::cout << "mut ";
            print_type(reint_ptr->referred.get());
            break;
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid TypeKind found\n";
            exit(1);
    }
}

void TypeChecker::print_type_mismatch(const TypeDecl *expected, 
    const TypeDecl *received, uint32_t expr_line, uint32_t expr_col) const
{
    print_error_location(expr_line, expr_col);
    std::cout << " -> Expected type: \"";
    print_type(expected);
    std::cout << "\" but got type: \"";
    print_type(received);
    std::cout << "\"\n";
}

void TypeChecker::print_invalid_init_expr(uint32_t line, uint32_t col,
    const TypeDecl *type) const
{
    print_error_location(line, col);
    std::cout << " -> Initialization expression is not valid for type: "
        "\"";
    print_type(type);
    std::cout << "\"\n";
}

uint64_t TypeChecker::resolve_type_decl(const TypeDecl *ptr) const
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
            std::cout << " -> Invalid kind found for TypeDecl\n";
            exit(1);
    }   
}

bool TypeChecker::is_type_uint_literal(const TypeDecl *ptr) const
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    if(!reint_ptr->builtin_type.has_value()) return false;

    switch(*reint_ptr->builtin_type)
    {
        case BuiltinType::U8:
            return true;

        case BuiltinType::U16:
            return true;

        case BuiltinType::U32:
            return true;

        case BuiltinType::U64:
            return true;

        default: return false;
    }
}

void TypeChecker::check_private_access(const uint64_t targ_scope_id, 
    const uint64_t accessing_scope_id, uint32_t expr_line, uint32_t expr_col,
    const std::vector<std::string> &ident_path) const
{
    // The only way a private access is allowed is if we find the symbol as
    // we iterate upward through the scope chain from the accessing symbol.

    uint64_t parsed_scope_id = accessing_scope_id;

    while(true)
    {
        // If we've found the scope iterating upwards, private access is fine.
        if(parsed_scope_id == targ_scope_id) return;

        const Scope *parsed_scope = &s_table->scopes.at(parsed_scope_id);

        // If we're at the global scope.
        if(!parsed_scope->parent_scope_idx.has_value()) break;

        parsed_scope_id = *parsed_scope->parent_scope_idx;
    }
    
    print_error_location(expr_line, expr_col);
    std::cout << " -> \"";
    print_ident_path(ident_path);
    std::cout << "\" is private and not accessible from this context.\n";
    exit(1);
}

void TypeChecker::cmp_ptr_types(const PtrTypeDecl *first, 
    const PtrTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const
{
    if(second->builtin_type.has_value())
    {
        std::cout << "rhs is a nullptr\n";

        // Nullptr can be assigned to any pointer.
        return;
    }

    if(first->points_to_mutable && !second->points_to_mutable)
    {
        print_error_location(second->line, second->col);
        std::cout << " -> Cannot assign pointer to non mutable: \"";
        print_type(second);
        std::cout << "\" to pointer to mutable: \"";
        print_type(first);
        std::cout << "\"\n";
        exit(1);
    }

    cmp_types(first->pointee.get(), second->pointee.get(), expr_line, 
        expr_col);
}

void TypeChecker::cmp_named_types(const NamedTypeDecl *first, 
    const NamedTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const
{
    if(second->builtin_type.has_value())
    {
        std::vector<BuiltinType> allowed_types;

        const uint64_t first_sym_idx = 
            first->resolved_symbol_idx.value();

        if(first_sym_idx == s_table->builtin_to_id.at("i8"))
        {
            std::cout << "lhs is i8\n";
            allowed_types.push_back(BuiltinType::I8);
        }
        
        else if(first_sym_idx == s_table->builtin_to_id.at("u8"))
        {
            std::cout << "lhs is u8\n";
            allowed_types.push_back(BuiltinType::U8);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i16"))
        {
            std::cout << "lhs is i16\n";
            allowed_types.push_back(BuiltinType::I8);
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::I16);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u16"))
        {
            std::cout << "lhs is u16\n";
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::U16);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i32") || 
            first_sym_idx == s_table->builtin_to_id.at("int"))
        {
            std::cout << "lhs is i32\n";
            allowed_types.push_back(BuiltinType::I8);
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::I16);
            allowed_types.push_back(BuiltinType::U16);
            allowed_types.push_back(BuiltinType::I32);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u32"))
        {
            std::cout << "lhs is u32\n";
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::U16);
            allowed_types.push_back(BuiltinType::U32);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i64"))
        {
            std::cout << "lhs is i64\n";
            allowed_types.push_back(BuiltinType::I8);
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::I16);
            allowed_types.push_back(BuiltinType::U16);
            allowed_types.push_back(BuiltinType::I32);
            allowed_types.push_back(BuiltinType::U32);
            allowed_types.push_back(BuiltinType::I64);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u64"))
        {
            std::cout << "lhs is u64\n";
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::U16);
            allowed_types.push_back(BuiltinType::U32);
            allowed_types.push_back(BuiltinType::U64);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("bool"))
        {
            std::cout << "lhs is bool\n";
            allowed_types.push_back(BuiltinType::BOOL);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("char"))
        {
            std::cout << "lhs is char\n";
            allowed_types.push_back(BuiltinType::U8);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("float"))
        {
            std::cout << "lhs is float\n";
            allowed_types.push_back(BuiltinType::FLOAT);
        }

        else
        {
            std::cout << "lhs could not be determined\n";
        }

        std::cout << "rhs is a literal: " << second->ident_path.at(0) << '\n';

        bool allowed = false;

        for(BuiltinType type : allowed_types)
        {
            if(type == *second->builtin_type) 
            {
                allowed = true;
                break;
            }
        }

        if(!allowed)
        {
            print_error_location(second->line, second->col);
            std::cout << " -> Cannot assign literal of type: \"" << 
                *second->builtin_type << "\" to a type of: \"";
            print_type(first);
            std::cout << "\"\n";
            exit(1);
        }

        std::cout << "Literal is allowed\n";

        return;
    }

    if(first->resolved_symbol_idx.value() != 
        second->resolved_symbol_idx.value())
    {
        print_type_mismatch(first, second, expr_line, expr_col);
        exit(1);
    }
}

void TypeChecker::cmp_types(const TypeDecl *first, const TypeDecl *second,
    uint32_t expr_line, uint32_t expr_col)
    const
{
    std::cout << "Checking types: ";
    print_type(first);
    std::cout << " and ";
    print_type(second);
    std::cout << '\n';

    if(first->kind != second->kind)
    {
        print_type_mismatch(first, second, expr_line, expr_col);
        exit(1); 
    }
 
    switch(first->kind)
    {
        case TypeKind::NAMED:
        {
            cmp_named_types(static_cast<const NamedTypeDecl*>(first),
                static_cast<const NamedTypeDecl*>(second), expr_line,
                expr_col);
            return;
        }

        case TypeKind::PTR:
        {
            cmp_ptr_types(static_cast<const PtrTypeDecl*>(first),
                static_cast<const PtrTypeDecl*>(second), expr_line, expr_col);
            return;
        }

        // case TypeKind::REF:
        // {

        //     break;
        // }

        // case TypeKind::ARRAY:
        // {

        //     break;
        // }

        // case TypeKind::FUNC_PTR:
        // {

        //     break;
        // }

        default:

            print_error_location(first->line, first->col);
            std::cout << " -> Invalid TypeKind found\n";
            exit(1);
    }
}

void TypeChecker::check_type(const TypeDecl *ptr, 
    const uint64_t type_scope_id)
{
    switch(ptr->kind)
    {
        case TypeKind::NAMED:
        {
            const NamedTypeDecl *reint_ptr = 
                static_cast<const NamedTypeDecl*>(ptr);

            if(reint_ptr->builtin_type.has_value()) return;

            if(s_table->type_symbol_ids.find(
                reint_ptr->resolved_symbol_idx.value()) == 
                s_table->type_symbol_ids.end())
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> \"";
                print_ident_path(reint_ptr->ident_path, std::cout);
                std::cout << "\" does not name a type\n";
                exit(1);
            }

            break;
        }

        case TypeKind::PTR:
        {
            const PtrTypeDecl *reint_ptr = 
                static_cast<const PtrTypeDecl*>(ptr);

            if(reint_ptr->builtin_type.has_value()) return;
            
            check_type(reint_ptr->pointee.get(), type_scope_id);
            break;
        }

        case TypeKind::REF:
        {
            const RefTypeDecl *reint_ptr = 
                static_cast<const RefTypeDecl*>(ptr);

            check_type(reint_ptr->referred.get(), type_scope_id);
            break;
        }

        case TypeKind::ARRAY:
        {
            const ArrTypeDecl *reint_ptr = 
                static_cast<const ArrTypeDecl*>(ptr);

            check_type(reint_ptr->element_type.get(), type_scope_id);

            for(uint8_t i = 0; i < reint_ptr->depth; ++i)
            {
                CheckExprResult depth_expr_result = 
                    check_expression(reint_ptr->size_exprs.at(i).get(),
                    type_scope_id);

                if(!is_type_uint_literal(depth_expr_result.type_decl))
                {
                    print_error_location(reint_ptr->size_exprs[i]->line,
                        reint_ptr->size_exprs[i]->col);
                    std::cout << " -> Array size expression must be an unsigned"
                        " int literal type.\n";
                    exit(1);
                }
            }

            break;
        }

        // case TypeKind::FUNC_PTR:
        // {

        //     break;
        // }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid TypeKind found\n";
            exit(1);
    }
}

void TypeChecker::check_module(const ModuleDecl * ptr)
{   
    std::cout << "Checking Module: " << ptr->ident << '\n';

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
        check_decl(decl.get());
}

void TypeChecker::check_component(const ComponentDecl * ptr)
{
    std::cout << "Checking Component: " << ptr->name << '\n';

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
        check_decl(decl.get(), ptr->symbol_idx.value());
}

void TypeChecker::check_struct(const StructDecl * ptr)
{
    std::cout << "Checking Struct: " << ptr->name << '\n';

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
        check_decl(decl.get());
}

void TypeChecker::check_function(const FunctionDecl * ptr,
    std::optional<uint64_t> owning_component_sym_idx)
{
    std::cout << "Checking Function: " << ptr->name << '\n';

    // This function has a receiver component
    if(ptr->receiver_data.has_value())
    {
        // This function is not defined inside a Component.
        if(!owning_component_sym_idx.has_value())
        {
            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Function with receiver component declared outside"
                " of component body.\n";
            exit(1);
        }

        if(ptr->receiver_data->type_decl->kind != TypeKind::REF)
        {
            print_error_location(ptr->receiver_data->type_decl->line,
                ptr->receiver_data->type_decl->col);
            std::cout << " -> Receiver components must be reference types.\n";
            exit(1);
        }

        const RefTypeDecl * receiver_type_decl = static_cast<const RefTypeDecl*>(
                ptr->receiver_data->type_decl.get());

        if(receiver_type_decl->referred->kind != TypeKind::NAMED)
        {
            print_error_location(receiver_type_decl->line, 
                receiver_type_decl->col);
            std::cout << " -> Receiver component must be a reference to a named"
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
            std::cout << " -> Receiver Component type does not match the owning"
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
    std::cout << "Checking Scope Body\n";

    for(const std::unique_ptr<Statement> &stmt : ptr->statements)
        check_statement(stmt.get(), ptr->scope_idx, expected_return_type);
}

void TypeChecker::check_var_decl_stmt(const VarDeclStmt *ptr)
{
    const Symbol *var_sym = 
        s_table->symbols.at(ptr->symbol_idx).get();

    check_type(ptr->type_decl.get(), var_sym->scope_idx.value());

    if(ptr->init_expr == nullptr) return;

    // We have an init expression.

    if(ptr->init_expr->exp_type == ExpressionType::STRUCT_INIT)
    {
        const StructInitExpr *init_expr = 
            static_cast<const StructInitExpr*>(ptr->init_expr.get());

        check_struct_init_expr(
            init_expr, ptr->type_decl.get(), var_sym->scope_idx.value());
        return;
    }

    if(ptr->init_expr->exp_type == ExpressionType::ARR_INIT)
    {
        const ArrInitExpr *init_expr = 
            static_cast<const ArrInitExpr*>(ptr->init_expr.get());

        check_arr_init_expr(
            init_expr, ptr->type_decl.get(), var_sym->scope_idx.value());
        return;
    }

    const CheckExprResult init_expr_result = 
        check_expression(ptr->init_expr.get(), *var_sym->scope_idx);

    cmp_types(ptr->type_decl.get(),
        init_expr_result.type_decl, ptr->init_expr->line,
        ptr->init_expr->col);
}

void TypeChecker::check_statement(const Statement * ptr, 
    const uint64_t scope_id, 
    std::optional<const TypeDecl*> expected_return_type)
{
    std::cout << "\nChecking Statement of type: " << ptr->stmt_type << '\n';

    switch(ptr->stmt_type)
    {
        case StatementType::EXPR:
        {
            check_expression(static_cast<const ExprStmt*>(ptr)->expr.get(), 
                scope_id);
            break;
        }

        case StatementType::VAR_DECL:
        {
            check_var_decl_stmt(static_cast<const VarDeclStmt*>(ptr));
            break;
        }

        // case StatementType::STRUCT_DECL:
        // {
            // break;
        // }

        // case StatementType::COMPONENT_DECL:
        // {
        //     break;
        // }

        case StatementType::RETURN:
        {
            const RetStmt *reint_ptr = static_cast<const RetStmt*>(ptr); 

            // If we are not expecting a return expression but we have one.
            if(!expected_return_type.has_value() && 
                reint_ptr->ret_expr != nullptr)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> Unexpected return expression for function "
                    "with void return type.";
            }

            // If we have no return expression.
            if(reint_ptr->ret_expr == nullptr)
            {
                // Make sure that the return type is void.

                const uint64_t void_symbol_idx = 
                    s_table->builtin_to_id.at("void");

                // If the expected return type is not void 
                if
                (
                    (*expected_return_type)->kind != TypeKind::NAMED || 
                    static_cast<const NamedTypeDecl*>(*expected_return_type)->
                        resolved_symbol_idx != void_symbol_idx
                )
                {
                    print_error_location(ptr->line, ptr->col);
                    std::cout << " -> Expected a return expression for "
                        "function with non void return type.\n";
                    exit(1);
                }
            }

            // We have a return type.
            else
            {
                const CheckExprResult ret_expr_result = 
                check_expression(reint_ptr->ret_expr.get(), scope_id);

                cmp_types(*expected_return_type, ret_expr_result.type_decl, 
                    reint_ptr->ret_expr->line, reint_ptr->ret_expr->col);
            }

            break;
        }

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
            std::cout << " -> Invalid StatementType found.\n";
            exit(1);
    }
}

void TypeChecker::check_param(const Parameter * ptr)
{
    std::cout << "Checking Parameter: " << ptr->name << '\n';
    
    std::cout << "Checking Parameter not implemented\n";
    exit(1);
}

void TypeChecker::check_decl(const Declaration * ptr, 
    std::optional<uint64_t> owning_component_sym_idx)
{
    std::cout << "\nChecking Declaration of type: " << ptr->kind << '\n';

    switch(ptr->kind)
    {
        case DeclKind::COMPONENT:
            check_component(static_cast<const ComponentDecl*>(ptr));
            break;

        case DeclKind::FIELD:
        {
            const FieldDecl *reint_ptr = 
                static_cast<const FieldDecl*>(ptr);
            
            const Symbol *field_sym =  
                s_table->symbols.at(reint_ptr->symbol_idx.value()).get();

            std::cout << "Checking Field: " << reint_ptr->name << '\n';

            if(reint_ptr->init_expr != nullptr)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> Field variables cannot have an initialization"
                    " expression.\n";
                exit(1);
            }

            check_type(reint_ptr->type_decl.get(), 
                field_sym->scope_idx.value());
            break;
        }

        case DeclKind::FUNCTION:
            check_function(static_cast<const FunctionDecl*>(ptr),
                owning_component_sym_idx);
            break;
        
        case DeclKind::MODULE:
            check_module(static_cast<const ModuleDecl*>(ptr));
            break;

        case DeclKind::STRUCT:
            check_struct(static_cast<const StructDecl*>(ptr));
            break;

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid DeclKind found\n";
            exit(1);
    }
}

template<typename T>
bool fits_in(const std::string &s, FitsInOption option) 
{
    std::from_chars_result result;
    T sink;

    switch(option)
    {
        case FitsInOption::NUMBER:

            result = std::from_chars(s.data(), s.data() + s.size(), sink);
            return result.ec == std::errc() && 
                result.ptr == s.data() + s.size();

        case FitsInOption::HEX:
        {
            // Trim the "0x" off the string
            std::string_view v = std::string_view{s}.substr(2);

            result = std::from_chars(v.data(), v.data() + v.size(), 
                sink, 16);

            return result.ec == std::errc() && 
                result.ptr == v.data() + v.size();
        }

        case FitsInOption::BINARY:
        {
            // Trim the "0b" off the string
            std::string_view v = std::string_view{s}.substr(2);

            result = std::from_chars(v.data(), v.data() + v.size(), 
                sink, 2);

            return result.ec == std::errc() && 
                result.ptr == v.data() + v.size();
        }
    }

    return false;
}

void TypeChecker::init_literal_typedecl(NamedTypeDecl *new_decl, 
    const std::string &s, FitsInOption option) const 
{
    if(fits_in<uint8_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U8);
        new_decl->ident_path.push_back("u8");
        new_decl->resolved_symbol_idx = s_table->builtin_to_id.at("u8");
    }

    else if(fits_in<int8_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I8);
        new_decl->ident_path.push_back("i8");
        new_decl->resolved_symbol_idx = s_table->builtin_to_id.at("i8");
    }

    else if(fits_in<uint16_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U16);
        new_decl->ident_path.push_back("u16");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("u16");
    }

    else if(fits_in<int16_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I16);
        new_decl->ident_path.push_back("i16");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("i16");
    }

    else if(fits_in<uint32_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U32);
        new_decl->ident_path.push_back("u32");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("u32");
    }

    else if(fits_in<int32_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I32);
        new_decl->ident_path.push_back("i32");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("i32");
    }

    else if(fits_in<uint64_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U64);
        new_decl->ident_path.push_back("u64");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("u64");
    }


    else if(fits_in<int64_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I64);
        new_decl->ident_path.push_back("i64");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("i64");
    }

    else
    {
        print_error_location(new_decl->line, new_decl->col);
        std::cout << " -> Unable to convert Binary number: " << s << 
            '\n';
        exit(1);
    }
}

const StructDecl* TypeChecker::get_struct_decl_from_type(
    const TypeDecl *type, uint32_t expr_line, uint32_t expr_col) const
{
    // First, make sure that the type of the variable is a struct.
    if(type->kind != TypeKind::NAMED)
    {
        print_invalid_init_expr(expr_line, expr_col, type);
        exit(1);
    }

    const StructDecl *struct_decl = nullptr;
    {
        const Symbol *symbol_of_type = 
            s_table->symbols.at(static_cast<const NamedTypeDecl*>(type)->
            resolved_symbol_idx.value()).get();

        if(symbol_of_type->sym_type != SymbolType::STRUCT)
        {
            print_invalid_init_expr(expr_line, expr_col, type);
            exit(1);
        }

        struct_decl = 
            static_cast<const StructSymbol*>(symbol_of_type)->ast_node_ptr;
    }

    return struct_decl;
}

TypeChecker::CheckExprResult TypeChecker::check_struct_init_expr(
    const StructInitExpr *expr, const TypeDecl *var_type,
    const uint64_t var_scope_id)
{
    std::cout << "Checking expression of type: " << 
        ExpressionType::STRUCT_INIT << '\n';

    CheckExprResult expr_result;

    const StructDecl *struct_decl = get_struct_decl_from_type(
        var_type, expr->line, expr->col);

    // // First, make sure that the type of the variable is a struct.
    // if(var_type->kind != TypeKind::NAMED)
    // {
    //     print_invalid_init_expr(expr->line, expr->col, var_type);
    //     exit(1);
    // }

    // const StructDecl *struct_decl = nullptr;
    // {
    //     const Symbol *symbol_of_type = 
    //         s_table->symbols.at(static_cast<const NamedTypeDecl*>(var_type)->
    //         resolved_symbol_idx.value()).get();

    //     if(symbol_of_type->sym_type != SymbolType::STRUCT)
    //     {
    //         print_invalid_init_expr(expr->line, expr->col, var_type);
    //         exit(1);
    //     }

    //     struct_decl = 
    //         static_cast<const StructSymbol*>(symbol_of_type)->ast_node_ptr;
    // }

    // The variable type is a valid struct type.

    if(expr->init_args.size() != struct_decl->decls.size())
    {
        print_error_location(expr->line, expr->col);
        std::cout << " -> Incorrect number of init args for struct type: \"";
        print_type(var_type);
        std::cout << "\".\n";
        exit(1);
    }

    for(size_t i = 0; i < struct_decl->decls.size(); ++i)
    {
        // Structs can only contain field declarations.
        const FieldDecl *field_decl = 
            static_cast<const FieldDecl*>(struct_decl->decls[i].get());

        // Check that the name of the init arg matches the name of the same idx
        // decl in the struct.

        const std::string &init_name = expr->init_args[i].first;
        const std::string &decl_name = field_decl->name;

        if(init_name != decl_name)
        {
            print_error_location(expr->line, expr->col);
            std::cout << " -> Name mismatch in struct init list: \"" <<
                init_name << "\" != \"" << decl_name << "\".\n";
            exit(1);
        }

        // Check the types of the init arg and the field decl.

        const Expression *targ_init_expr = 
            expr->init_args[i].second.get();

        CheckExprResult init_expr_result = 
            check_expression(targ_init_expr, var_scope_id);

        cmp_types(field_decl->type_decl.get(), init_expr_result.type_decl, 
            targ_init_expr->line, 
            targ_init_expr->col);
    }

    expr_result.is_lvalue = false;
    expr_result.is_var_and_mutable = false;
    expr_result.type_decl = var_type;

    return expr_result;
}

TypeChecker::CheckExprResult TypeChecker::check_arr_init_expr(
    const ArrInitExpr *expr, const TypeDecl *var_type, 
    const uint64_t var_scope_id)
{
    std::cout << "Checking expression of type: " <<
        ExpressionType::ARR_INIT << '\n';

    for(const std::unique_ptr<Expression> &init_expr: expr->init_args)
    {
    }    
        
    CheckExprResult expr_result;

    expr_result.is_lvalue = false;
    expr_result.is_var_and_mutable = false;
    expr_result.type_decl = var_type;

    std::cout << "Array init expr checking not implemented\n";
    exit(1);
}

TypeChecker::CheckExprResult TypeChecker::check_struct_create_expr(
    const StructCreateExpr *expr, const uint64_t scope_id)
{
    CheckExprResult lhs_expr_result = check_expression(expr->lhs.get(), 
        scope_id);

    if(expr->create_expr->exp_type != ExpressionType::STRUCT_INIT)
    {
        print_invalid_init_expr(expr->lhs->line, expr->lhs->col, 
            lhs_expr_result.type_decl);
        exit(1);
    }

    return check_struct_init_expr(static_cast<const StructInitExpr*>(
        expr->create_expr.get()), lhs_expr_result.type_decl, scope_id);  
}

TypeChecker::CheckExprResult TypeChecker::check_ident_expr(
    const IdentExpr *ptr, const uint64_t scope_id)
{
    CheckExprResult expr_result;

    const Symbol *ident_symbol = s_table->symbols.at(
        ptr->resolved_symbol_idx.value()).get();

    switch(ident_symbol->sym_type)
    {
        case SymbolType::STRUCT:
        {   
            const StructSymbol *reint_symbol = 
                static_cast<const StructSymbol*>(ident_symbol);

            const StructDecl *struct_decl = 
                static_cast<const StructDecl*>(reint_symbol->ast_node_ptr);

            expr_result.is_lvalue = false;
            expr_result.is_var_and_mutable = false;

            NamedTypeDecl * new_decl = new NamedTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;
            new_decl->ident_path = ptr->ident_path;
            new_decl->resolved_symbol_idx = ptr->resolved_symbol_idx;

            expr_result.type_decl = new_decl;

            // If this Struct is marked private, we need to make sure we have 
            // the rights to access it from this scope.
            if(!struct_decl->is_pub)
                check_private_access(reint_symbol->scope_idx.value(), 
                    scope_id, ptr->line, ptr->col, ptr->ident_path);
            break;
        }

        case SymbolType::FIELD:
        {
            const FieldSymbol *reint_symbol = 
                static_cast<const FieldSymbol*>(ident_symbol);

            const FieldDecl *field = reint_symbol->ast_node_ptr;

            expr_result.is_lvalue = true;
            expr_result.is_var_and_mutable = field->is_binding_mutable;
            expr_result.type_decl = field->type_decl.get();

            // If this field is marked private, we need to make sure we have the 
            // rights to access it from this scope.
            if(!field->is_pub)
                check_private_access(reint_symbol->scope_idx.value(), scope_id, 
                ptr->line, ptr->col, ptr->ident_path);
            break;
        }

        case SymbolType::PARAM:
        {
            const ParamSymbol *reint_symbol = 
                static_cast<const ParamSymbol*>(ident_symbol);

            const Parameter *param = reint_symbol->ast_node_ptr;

            expr_result.is_lvalue = true;
            expr_result.is_var_and_mutable = param->is_binding_mutable;
            expr_result.type_decl = param->type_decl.get();
            break;
        }

        case SymbolType::VAR:
        {
            const VarSymbol *reint_symbol = 
                static_cast<const VarSymbol*>(ident_symbol);

            const VarDeclStmt * var_stmt = reint_symbol->ast_node_ptr;
        
            expr_result.is_lvalue = true;
            expr_result.is_var_and_mutable = var_stmt->is_binding_mutable;
            expr_result.type_decl = var_stmt->type_decl.get();
            break;
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Unexpected identifier: \"";
            print_ident_path(ptr->ident_path, std::cout);
            std::cout << "\".\n";
            exit(1);

    }

    return expr_result;
}

TypeChecker::CheckExprResult TypeChecker::check_expression
    (const Expression *ptr, const uint64_t scope_id)
{
    std::cout << "Checking expression of type: " << ptr->exp_type << '\n';

    CheckExprResult expr_result;

    switch(ptr->exp_type)
    {       
        case ExpressionType::BIN_LITERAL:
        {
            const BinaryLitExpr *reint_ptr = 
                static_cast<const BinaryLitExpr*>(ptr);

            NamedTypeDecl *new_decl = new NamedTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;
        
            init_literal_typedecl(new_decl, reint_ptr->value, 
                FitsInOption::BINARY);            
            
            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;
        }

        case ExpressionType::HEX_LITERAL:
        {
            const HexLitExpr *reint_ptr = 
                static_cast<const HexLitExpr*>(ptr);

            NamedTypeDecl *new_decl = new NamedTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;

            init_literal_typedecl(new_decl, reint_ptr->value, 
                FitsInOption::HEX);

            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;
        }

        case ExpressionType::INT_LITERAL:
        {
            const IntLitExpr *reint_ptr = 
                static_cast<const IntLitExpr*>(ptr);

            NamedTypeDecl *new_decl = new NamedTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;

            init_literal_typedecl(new_decl, reint_ptr->value, 
                FitsInOption::NUMBER);

            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;;
        }

        case ExpressionType::FLOAT_LITERAL:
        {
            NamedTypeDecl *new_decl = new NamedTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;
            
            new_decl->builtin_type.emplace(BuiltinType::FLOAT);
            new_decl->ident_path.push_back("float");
            new_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("float"));

            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;
        }

        case ExpressionType::STR_LITERAL:
        {
            PtrTypeDecl *new_decl = new PtrTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;

            new_decl->builtin_type = BuiltinPtrType::CSTR_PTR;
            new_decl->points_to_mutable = false;

            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;
        }

        case ExpressionType::CHAR_LITERAL:
        {
            NamedTypeDecl *new_decl = new NamedTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;
            
            new_decl->builtin_type.emplace(BuiltinType::U8);
            new_decl->ident_path.push_back("u8");
            new_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("u8"));

            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;
        }

        case ExpressionType::BOOL_LITERAL:
        {
            NamedTypeDecl *new_decl = new NamedTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;
            
            new_decl->builtin_type.emplace(BuiltinType::BOOL);
            new_decl->ident_path.push_back("bool");
            new_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("bool"));

            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;
        }

        case ExpressionType::NULLPTR_LITERAL:
        {
            PtrTypeDecl *new_decl = new PtrTypeDecl();
            decls_to_delete.push_back(new_decl);

            new_decl->line = ptr->line;
            new_decl->col = ptr->col;

            new_decl->builtin_type = BuiltinPtrType::NULL_PTR;

            expr_result.is_var_and_mutable = false;
            expr_result.type_decl = new_decl;

            break;
        }

        case ExpressionType::IDENTIFIER:
        {
            expr_result = check_ident_expr(static_cast<const IdentExpr*>(ptr),
                scope_id);
            break;
        }

        case ExpressionType::STRUCT_INIT:
        {
            std::cout << "Struct init expression should not be handled by the "
                "main check_expression function\n";
            exit(1);
        }

        case ExpressionType::STRUCT_CREATE:

            expr_result = check_struct_create_expr(
                static_cast<const StructCreateExpr*>(ptr), scope_id);
            break;

        case ExpressionType::ARR_INIT:
        {
            std::cout << "Array init expression should not be handled by the "
                "main check_expression function\n";
            exit(1);
        }

        // case ExpressionType::UNARY:
        // {
        //     break;
        // }

        // case ExpressionType::BINARY:
        // {
        //     break;
        // }

        // case ExpressionType::TERNARY:
        // {
        //     break;
        // }

        // case ExpressionType::ASSIGN:
        // {
        //     break;
        // }

        // case ExpressionType::CALL:
        // {
        //     break;
        // }

        // case ExpressionType::CAST:
        // {
        //     break;
        // }

        // case ExpressionType::SUBSCRIPT:
        // {
        //     break;
        // }

        // case ExpressionType::MEMBER_ACCESS:
        // {
        //     break;
        // }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid ExpressionType found\n";
            exit(1);
    }

    return expr_result;
}

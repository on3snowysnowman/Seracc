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

                        std::cout << "opaque_ptr";
                        return;

                    case BuiltinPtrType::NULL_PTR:

                        std::cout << "null_ptr";
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

void TypeChecker::print_invalid_type_assignment(const TypeDecl *first,
    const TypeDecl *second, uint32_t expr_line, uint32_t expr_col) const
{
    print_error_location(expr_line, expr_col);
    std::cout << " -> Cannot assign type: \"";
    print_type(second);
    std::cout << "\" to type: \"";
    print_type(first);
    std::cout << "\".\n";
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

bool TypeChecker::is_type_numeric(const TypeDecl *ptr) const
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    if(reint_ptr->builtin_data.has_value())
    {
        return is_type_numeric_literal(ptr);
    }

    const uint64_t type_id = 
        reint_ptr->resolved_symbol_idx.value();

    return 
        type_id == s_table->builtin_to_id.at("i8") || 
        type_id == s_table->builtin_to_id.at("u8") || 
        type_id == s_table->builtin_to_id.at("i16") || 
        type_id == s_table->builtin_to_id.at("u16") || 
        type_id == s_table->builtin_to_id.at("i32") || 
        type_id == s_table->builtin_to_id.at("int") || 
        type_id == s_table->builtin_to_id.at("u32") || 
        type_id == s_table->builtin_to_id.at("i64") || 
        type_id == s_table->builtin_to_id.at("u64") || 
        type_id == s_table->builtin_to_id.at("float") ||
        type_id == s_table->builtin_to_id.at("double");
}

bool TypeChecker::is_type_integral(const TypeDecl *ptr) const
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    if(reint_ptr->builtin_data.has_value())
    {
        return is_type_int_literal(ptr);
    }

    const uint64_t type_id = 
        reint_ptr->resolved_symbol_idx.value();

    return 
        type_id == s_table->builtin_to_id.at("i8") || 
        type_id == s_table->builtin_to_id.at("u8") || 
        type_id == s_table->builtin_to_id.at("i16") || 
        type_id == s_table->builtin_to_id.at("u16") || 
        type_id == s_table->builtin_to_id.at("i32") || 
        type_id == s_table->builtin_to_id.at("int") || 
        type_id == s_table->builtin_to_id.at("u32") || 
        type_id == s_table->builtin_to_id.at("i64") || 
        type_id == s_table->builtin_to_id.at("u64");
}

bool TypeChecker::is_type_uintegral(const TypeDecl *ptr) const
{
    // Need to check if this is an uint literal for uintegral types, since 
    // a positive i8 literal will flag a false negative here.
    // if(is_type_uint_literal(ptr)) return true;

    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    // If we are dealing with a builtin type.
    if(reint_ptr->builtin_data.has_value())
    {
        return is_type_uint_literal(ptr);
    }

    const uint64_t type_id = 
        reint_ptr->resolved_symbol_idx.value();
    
    return 
        type_id == s_table->builtin_to_id.at("u8") ||  
        type_id == s_table->builtin_to_id.at("u16") ||   
        type_id == s_table->builtin_to_id.at("u32") ||  
        type_id == s_table->builtin_to_id.at("u64");
}

bool is_type_literal_helper(const TypeDecl *ptr, 
    const std::unordered_set<uint64_t> acceptable_ids)
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    if(!reint_ptr->builtin_data.has_value()) return false;

    const BuiltinData &bt_data = *reint_ptr->builtin_data;

    for(uint64_t bt_id : bt_data.acceptable_builtin_ids)
    {
        if(acceptable_ids.find(bt_id) !=  acceptable_ids.end()) return true;
    }

    return false;
}

bool TypeChecker::is_type_numeric_literal(const TypeDecl *ptr) const
{
    std::unordered_set<uint64_t> acceptable_ids;
    acceptable_ids.emplace(s_table->builtin_to_id.at("i8"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u8"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("i16"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u16"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("i32"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u32"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("i64"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u64"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("float"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("double"));

    return is_type_literal_helper(ptr, acceptable_ids);

    // if(ptr->kind != TypeKind::NAMED) return false;

    // const NamedTypeDecl *reint_ptr = 
    //     static_cast<const NamedTypeDecl*>(ptr);

    // if(!reint_ptr->builtin_data.has_value()) return false;

    // const BuiltinData &bt_data = *reint_ptr->builtin_data;

    // std::cout << "is_numeric_literal not implemented\n";
    // exit(1);

    // switch(bt_data.builtin_type)
    // {
    //     case BuiltinType::I8:
    //         return true;

    //     case BuiltinType::U8:
    //         return true;

    //     case BuiltinType::I16:
    //         return true;

    //     case BuiltinType::U16:
    //         return true;

    //     case BuiltinType::I32:
    //         return true;

    //     case BuiltinType::U32:
    //         return true;

    //     case BuiltinType::I64:
    //         return true;

    //     case BuiltinType::U64:
    //         return true;

    //     case BuiltinType::FLOAT:
    //         return true;

    //     case BuiltinType::DOUBLE:
    //         return true;

    //     default: return false;
    // }
}

bool TypeChecker::is_type_int_literal(const TypeDecl *ptr) const
{
    std::unordered_set<uint64_t> acceptable_ids;
    acceptable_ids.emplace(s_table->builtin_to_id.at("i8"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u8"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("i16"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u16"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("i32"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u32"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("i64"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u64"));

    return is_type_literal_helper(ptr, acceptable_ids);

    // if(ptr->kind != TypeKind::NAMED) return false;

    // const NamedTypeDecl *reint_ptr = 
    //     static_cast<const NamedTypeDecl*>(ptr);

    // if(!reint_ptr->builtin_data.has_value()) return false;

    // const BuiltinData &bt_data = *reint_ptr->builtin_data;

    // std::cout << "is_int_literal not implemented\n";
    // exit(1);

    // switch(bt_data.builtin_type)
    // {
    //     case BuiltinType::I8:
    //         return true;

    //     case BuiltinType::U8:
    //         return true;

    //     case BuiltinType::I16:
    //         return true;

    //     case BuiltinType::U16:
    //         return true;

    //     case BuiltinType::I32:
    //         return true;

    //     case BuiltinType::U32:
    //         return true;

    //     case BuiltinType::I64:
    //         return true;

    //     case BuiltinType::U64:
    //         return true;

    //     default: return false;
    // }
}

bool TypeChecker::is_type_uint_literal(const TypeDecl *ptr) const
{
    // if(ptr->kind != TypeKind::NAMED) return false;

    // const NamedTypeDecl *reint_ptr = 
    //     static_cast<const NamedTypeDecl*>(ptr);

    // if(!reint_ptr->builtin_data.has_value()) return false;

    // const BuiltinData &bt_data = *reint_ptr->builtin_data;

    std::unordered_set<uint64_t> acceptable_ids;
    acceptable_ids.emplace(s_table->builtin_to_id.at("u8"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u16"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u32"));
    acceptable_ids.emplace(s_table->builtin_to_id.at("u64"));

    return is_type_literal_helper(ptr, acceptable_ids);

    // for(uint64_t bt_id : bt_data.acceptable_builtin_ids)
    // {
    //     if(acceptable_ids.find(bt_id) !=  acceptable_ids.end()) return true;
    // }

    // return false;

    // switch(bt_data.builtin_type)
    // {
    //     case BuiltinType::I8:
    //         return !bt_data.is_integral_and_negative;

    //     case BuiltinType::U8:
    //         return true;

    //     case BuiltinType::I16:
    //         return !bt_data.is_integral_and_negative;

    //     case BuiltinType::U16:
    //         return true;

    //     case BuiltinType::I32:
    //         return !bt_data.is_integral_and_negative;

    //     case BuiltinType::U32:
    //         return true;

    //     case BuiltinType::I64:
    //         return !bt_data.is_integral_and_negative;

    //     case BuiltinType::U64:
    //         return true;

    //     default: return false;
    // }
}

bool TypeChecker::is_type_bool(const TypeDecl *ptr) const
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = static_cast<const NamedTypeDecl*>(ptr);

    // Builtins live at the top scope.
    if(reint_ptr->ident_path.size() != 1) return false;
    
    return reint_ptr->ident_path.front() == "bool";
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

bool TypeChecker::cmp_ptr_types(const PtrTypeDecl *first, 
    const PtrTypeDecl *second) const
{
    if(second->builtin_type.has_value() && 
        *second->builtin_type == BuiltinPtrType::NULL_PTR)
    {
        std::cout << "rhs is a null_ptr\n";
        // Nullptr can be assigned to any pointer.
        return true;
    }

    if(first->builtin_type.has_value() && 
        *first->builtin_type == BuiltinPtrType::OPAQUE_PTR)
    {
        if(!second->builtin_type.has_value() || 
            *second->builtin_type != BuiltinPtrType::OPAQUE_PTR)
        {
            // print_type_mismatch(first, second, expr_line, expr_col);
            // exit(1);
            return false;
        }

        return true;
    }

    if(first->points_to_mutable && !second->points_to_mutable)
    {
        // print_error_location(expr_line, expr_col);
        // std::cout << " -> Cannot assign pointer to non mutable: \"";
        // print_type(second);
        // std::cout << "\" to pointer to mutable: \"";
        // print_type(first);
        // std::cout << "\"\n";
        // exit(1);
        return false;
    }

    return cmp_types(first->pointee.get(), second->pointee.get());
}

bool TypeChecker::cmp_builtin_type(const NamedTypeDecl *builtin_type,
    const NamedTypeDecl *other_type) const
{
    // If we are dealing with two builtin types.
    if(other_type->builtin_data.has_value()) return false;

    // -- We are dealing with only a single builtin 

    std::cout << "Dealing with single builtin type not implemented\n";
    exit(1);

    return true;
}

bool TypeChecker::cmp_named_types(const NamedTypeDecl *first, 
    const NamedTypeDecl *second) const
{
    if(first->builtin_data.has_value())
        return cmp_builtin_type(first, second);

    if(second->builtin_data.has_value())
        return cmp_builtin_type(second, first);

    return first->resolved_symbol_idx.value() ==
           second->resolved_symbol_idx.value();

    return true;
}

bool TypeChecker::cmp_types(const TypeDecl *first, const TypeDecl *second)
    const
{
    std::cout << "Checking types: ";
    print_type(first);
    std::cout << " and ";
    print_type(second);
    std::cout << '\n';

    if(first->kind != second->kind)
    {
        // print_type_mismatch(first, second, expr_line, expr_col);
        // exit(1); 
        return false;
    }
 
    switch(first->kind)
    {
        case TypeKind::NAMED:
        {
            return cmp_named_types(static_cast<const NamedTypeDecl*>(first),
                static_cast<const NamedTypeDecl*>(second));
        }

        case TypeKind::PTR:
        {
            return cmp_ptr_types(static_cast<const PtrTypeDecl*>(first),
                static_cast<const PtrTypeDecl*>(second));
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

            if(reint_ptr->builtin_data.has_value()) return;

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
        if(ptr->type_decl->kind != TypeKind::ARRAY)
        {
            print_error_location(ptr->init_expr->line, ptr->init_expr->col);
            std::cout << " -> Cannot initialize non Array type with an Array "
                "initialization expression\n";
            exit(1);
        }

        const ArrTypeDecl *arr_type = 
            static_cast<const ArrTypeDecl*>(ptr->type_decl.get());

        const ArrInitExpr *init_expr = 
            static_cast<const ArrInitExpr*>(ptr->init_expr.get());

        check_arr_init_expr(
            init_expr, arr_type, var_sym->scope_idx.value());
        return;
    }

    // -- Non Struct or Array type -- 

    const CheckExprResult init_expr_result = 
        check_expression(ptr->init_expr.get(), *var_sym->scope_idx);

    if(!cmp_types(ptr->type_decl.get(),
        init_expr_result.type_decl))
    {
        print_invalid_type_assignment(ptr->type_decl.get(), 
            init_expr_result.type_decl, ptr->init_expr->line, 
            ptr->init_expr->col);
        exit(1);
    }
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

                if(!cmp_types(*expected_return_type, ret_expr_result.type_decl))
                {
                    print_invalid_type_assignment(*expected_return_type, 
                        ret_expr_result.type_decl, reint_ptr->ret_expr->line,
                        reint_ptr->ret_expr->col);
                    exit(1);
                }
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

template<typename T>
void handle_fits_in(NamedTypeDecl *new_decl, const std::string &s, 
    FitsInOption option, const uint64_t builtin_id, 
    std::string &&builtin_readable)
{
    if(fits_in<T>(s, option))
    {
        new_decl->builtin_data->acceptable_builtin_ids.push_back(builtin_id);
        new_decl->ident_path.push_back(std::move(builtin_readable));
    }
}

void TypeChecker::init_literal_typedecl(NamedTypeDecl *new_decl, 
    const std::string &s, FitsInOption option) const 
{
    new_decl->builtin_data.emplace(BuiltinData{});
    
    handle_fits_in<int8_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("i8"), "i8");
    handle_fits_in<uint8_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("u8"), "u8");
    handle_fits_in<int16_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("i16"), "i16");
    handle_fits_in<uint16_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("u16"), "u16");
    handle_fits_in<int32_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("i32"), "i32");
    handle_fits_in<uint32_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("u32"), "u32");
    handle_fits_in<int64_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("i64"), "i64");
    handle_fits_in<uint64_t>(new_decl, s, option, 
        s_table->builtin_to_id.at("u64"), "u64");

    if(new_decl->builtin_data->acceptable_builtin_ids.size() == 0)
    {
        print_error_location(new_decl->line, new_decl->col);
        std::cout << " -> Unable to convert number: " << s << 
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

    // The variable type is a valid struct type.

    if(expr->init_args.size() != struct_decl->decls.size())
    {
        print_error_location(expr->line, expr->col);
        std::cout << " -> Incorrect number of initialization args for struct "
        "type: \"";
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

        if(!cmp_types(field_decl->type_decl.get(), init_expr_result.type_decl))
        {
            print_error_location(targ_init_expr->line, targ_init_expr->col);
            std::cout << " -> Type mismatch between init arg of type: \"";
            print_type(init_expr_result.type_decl);
            std::cout << "\" and Struct member type: \"";
            print_type(field_decl->type_decl.get());
            std::cout << "\".\n";
            exit(1);
        }
    }

    expr_result.is_lvalue = false;
    expr_result.is_var_and_mutable = false;
    expr_result.type_decl = var_type;

    return expr_result;
}

void TypeChecker::recurse_check_arr_init(const ArrInitExpr *init_expr, 
    const uint8_t depth, const std::vector<size_t> &size_values,
    const TypeDecl *element_type, const uint64_t scope_id)
{
    if(size_values.at(depth) != init_expr->init_args.size())
    {
        print_error_location(init_expr->line, init_expr->col);
        std::cout << " -> Number of initialization arguments does not match "
            " Array type dimensions.\n";
        exit(1);
    }

    // We are not at the final depth of the array yet so we don't need to 
    // compare expression types yet.
    if(depth < size_values.size() - 1)
    {
        for(const std::unique_ptr<Expression> & init_arg : 
            init_expr->init_args)
        {
            if(init_arg->exp_type != ExpressionType::ARR_INIT)
            {
                print_error_location(init_arg->line, init_arg->col);
                std::cout << " -> Expected Array initialization expression\n";
                exit(1);
            }

            const ArrInitExpr *reint_init_arg = 
                static_cast<const ArrInitExpr*>(init_arg.get());

            recurse_check_arr_init(reint_init_arg, depth + 1, size_values,
                element_type, scope_id);
        }
    
        return;
    }

    // We are at the final depth. Parse each init expression 
    for(const std::unique_ptr<Expression> &init_arg : init_expr->init_args)
    {
        CheckExprResult init_arg_res = 
            check_expression(init_arg.get(), scope_id);

        if(!cmp_types(element_type, init_arg_res.type_decl))
        {
            print_error_location(init_arg->line, init_arg->col);
            std::cout << " -> Type mismatch between init arg of type: \"";
            print_type(init_arg_res.type_decl);
            std::cout << "\" and Array type: \"";
            print_type(element_type);
            std::cout << "\".\n";
            exit(1);
        }
    }    
}

TypeChecker::CheckExprResult TypeChecker::check_arr_init_expr(
    const ArrInitExpr *init_expr, const ArrTypeDecl *arr_type, 
    const uint64_t var_scope_id)
{
    std::cout << "Checking expression of type: " <<
        ExpressionType::ARR_INIT << '\n';

    // First convert all of our size expressions to actual values.

    std::vector<size_t> size_values;

    for(const std::unique_ptr<Expression> &size_expr : arr_type->size_exprs)
    {
        const IntLitExpr *depth_expr = 
        static_cast<IntLitExpr*>(size_expr.get());

        size_t size_at_depth = 0;
        
        std::from_chars_result from_ch_res = std::from_chars(
            depth_expr->value.data(), 
            depth_expr->value.data() + depth_expr->value.size(), size_at_depth);

        // Check if conversion was successful
        if(from_ch_res.ec != std::errc() || 
            from_ch_res.ptr != 
            depth_expr->value.data() + depth_expr->value.size())
        {
            print_error_location(depth_expr->line, depth_expr->col);
            std::cout << " -> Failed to convert string number to an integer.\n";
            exit(1);
        }

        std::cout << "Got array size: " << size_at_depth << '\n';

        size_values.push_back(size_at_depth);
    }

    recurse_check_arr_init(init_expr, 0, size_values, 
        arr_type->element_type.get(), var_scope_id);

    CheckExprResult expr_result;

    expr_result.is_lvalue = false;
    expr_result.is_var_and_mutable = false;
    expr_result.type_decl = arr_type;

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

TypeChecker::CheckExprResult TypeChecker::check_prepost_incdec(
    const UnaryExpr * ptr, const uint64_t scope_id)
{
    CheckExprResult operand_res = 
        check_expression(ptr->operand.get(), scope_id);

    if(!operand_res.is_lvalue)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Expression result must be an lvalue.\n";
        exit(1);
    }

    if(!operand_res.is_var_and_mutable)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Variable is not mutable.\n";
        exit(1);
    }

    if(operand_res.type_decl->kind == TypeKind::PTR) return operand_res;

    if(!is_type_numeric(operand_res.type_decl))
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Expression result must be an integral type\n";
        exit(1);
    }

    return operand_res;
}

TypeChecker::CheckExprResult TypeChecker::check_expr_ensure_integral(
    const Expression *ptr, const uint64_t scope_id)
{
    CheckExprResult expr_result = check_expression(ptr, scope_id);
                
    if(!is_type_numeric(expr_result.type_decl))
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Expression result must be an integral type\n";
        exit(1);
    }

    return expr_result;
}

TypeChecker::CheckExprResult TypeChecker::check_addr_of_expr(
    const UnaryExpr *ptr, const uint64_t scope_id)
{
    CheckExprResult expr_result = 
        check_expression(ptr->operand.get(), scope_id);

    if(!expr_result.is_lvalue)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Can't take address of a non lvalue\n";
        exit(1);
    }

    PtrTypeDecl * new_decl = new PtrTypeDecl();
    decls_to_delete.push_back(new_decl);

    new_decl->line = ptr->line;
    new_decl->col = ptr->col;
    new_decl->points_to_mutable = expr_result.is_var_and_mutable;

    // Our new PtrTypeDecl that we've had to create needs a unique ptr to the 
    // TypeDecl it's pointing to. Since it's a unique ptr and we only have a raw
    // pointer, we need to clone it and keep our own copy of the data.
    new_decl->pointee = expr_result.type_decl->clone();

    CheckExprResult final_result;

    final_result.type_decl = new_decl;
    // Temporary ptr to lvalue is not an lvalue.
    final_result.is_lvalue = false; 
    final_result.is_var_and_mutable = false;

    return final_result;
}

TypeChecker::CheckExprResult TypeChecker::check_deref_expr(
    const UnaryExpr *ptr, const uint64_t scope_id)
{
    CheckExprResult operand_result = 
        check_expression(ptr->operand.get(), scope_id);

    if(operand_result.type_decl->kind != TypeKind::PTR)
    {
        print_error_location(ptr->line, ptr->col);
        std::cerr << " -> Can't dereference a non pointer type\n";
        exit(1);
    }

    const PtrTypeDecl *operand_reint_ptr = 
        static_cast<const PtrTypeDecl*>(operand_result.type_decl);

    if(operand_reint_ptr->builtin_type.has_value())
    {
        if(*operand_reint_ptr->builtin_type == BuiltinPtrType::NULL_PTR)
        {
            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Can't dereference a null_ptr\n";
            exit(1);
        }

        else if(*operand_reint_ptr->builtin_type == BuiltinPtrType::OPAQUE_PTR)
        {
            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Unexpected identifier: \"opaque_ptr\"\n";
            exit(1);
        }
    }

    CheckExprResult final_result;

    final_result.type_decl = operand_reint_ptr->pointee.get();
    final_result.is_lvalue = operand_result.is_lvalue;
    final_result.is_var_and_mutable = final_result.is_lvalue && 
        operand_reint_ptr->points_to_mutable;

    return final_result;
}

TypeChecker::CheckExprResult TypeChecker::check_unary_expr(
    const UnaryExpr *ptr, const uint64_t scope_id)
{
    std::cout << "Checking Unary Expression of type: " <<
        ptr->op_type << '\n';

    CheckExprResult expr_result;

    switch(ptr->op_type)
    {
        case UnaryOp::PRE_INC: 
        
            return check_prepost_incdec(ptr, scope_id);

        case UnaryOp::PRE_DEC: 
        
            return check_prepost_incdec(ptr, scope_id);

        case UnaryOp::POST_INC: 
        
            return check_prepost_incdec(ptr, scope_id);

        case UnaryOp::POST_DEC: 
        
            return check_prepost_incdec(ptr, scope_id);

        case UnaryOp::NEGATE:
        {
            expr_result = check_expr_ensure_integral(ptr->operand.get(),
                scope_id);
            break;
        }

        case UnaryOp::BIT_NOT:
        {
            expr_result = check_expr_ensure_integral(ptr->operand.get(),
                scope_id);
            break;
        }

        case UnaryOp::LOG_NOT:
        {
            expr_result = check_expression(ptr->operand.get(), scope_id);

            if(!is_type_bool(expr_result.type_decl))
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " Expression result must be a boolean type\n";
                exit(1);
            }

            break;
        }

        case UnaryOp::ADDRESS_OF:
        {
            expr_result = check_addr_of_expr(ptr, scope_id);
            break;
        }

        case UnaryOp::DEREF:
        {
            expr_result = check_deref_expr(ptr, scope_id);
            break;
        }

        default:
            
            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid OpType found\n";
            exit(1);
    }

    return expr_result;
}

TypeChecker::CheckExprResult TypeChecker::check_arith_expr(
    const BinaryExpr *ptr, const uint64_t scope_id, bool is_addsub_expr)
{
    CheckExprResult lhs_result = 
        check_expression(ptr->lhs.get(), scope_id);
    CheckExprResult rhs_result = 
        check_expression(ptr->rhs.get(), scope_id);

    bool lhs_is_numerical = is_type_numeric(lhs_result.type_decl);
    bool rhs_is_numerical = is_type_numeric(rhs_result.type_decl);

    if(is_addsub_expr)
    {
        if(lhs_result.type_decl->kind == TypeKind::PTR)
        {
            if(!rhs_is_numerical)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> Pointers only support arithmetic with "
                    "integral types\n";
                exit(1);
            }

            return lhs_result;
        }

        if(rhs_result.type_decl->kind == TypeKind::PTR)
        {
            if(!lhs_is_numerical)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> Pointers only support arithmetic with "
                    "integral types\n";
                exit(1);
            }

            return rhs_result;
        }
    }

    if(!lhs_is_numerical)
    {
        print_error_location(ptr->lhs->line, ptr->lhs->col);
        std::cout << " -> Invalid type for arithmetic.\n";
        exit(1);
    }

    if(!rhs_is_numerical)
    {
        print_error_location(ptr->rhs->line, ptr->rhs->col);
        std::cout << " -> Invalid type for arithmetic.\n";
        exit(1);
    }

    // If lhs is a literal
    // if(is_type_numeric_literal(lhs_result.type_decl))
    // {
    //     if(!cmp_types(rhs_result.type_decl, lhs_result.type_decl))
    //     {
    //         print_error_location(ptr->line, ptr->col);
    //         std::cout << " -> Unable to perform arithmetic between non matching"
    //             " types: \"";
    //         print_type(lhs_result.type_decl);
    //         std::cout << "\" and type: \"";
    //         print_type(rhs_result.type_decl);
    //         std::cout << "\".\n";
    //         exit(1);
    //     }

    //     return rhs_result;
    // }

    // // If rhs is a literal
    // if(is_type_numeric_literal(rhs_result.type_decl))
    // {
    //     if(!cmp_types(lhs_result.type_decl, rhs_result.type_decl))
    //     {
    //         print_error_location(ptr->line, ptr->col);
    //         std::cout << " -> Unable to perform arithmetic between non matching"
    //             " types: \"";
    //         print_type(lhs_result.type_decl);
    //         std::cout << "\" and type: \"";
    //         print_type(rhs_result.type_decl);
    //         std::cout << "\".\n";
    //         exit(1);
    //     }

    //     return lhs_result;
    // }
    
    if(cmp_types(lhs_result.type_decl, rhs_result.type_decl)) 
        return lhs_result;

    print_error_location(ptr->line, ptr->col);
    std::cout << " -> Unable to perform arithmetic between non matching types:"
        " \"";
    print_type(lhs_result.type_decl);
    std::cout << "\" and type: \"";
    print_type(rhs_result.type_decl);
    std::cout << "\".\n";
    exit(1);
}

TypeChecker::CheckExprResult TypeChecker::check_shift_expr(
    const BinaryExpr *ptr, const uint64_t scope_id)
{
    CheckExprResult lhs_result = 
        check_expression(ptr->lhs.get(), scope_id);
    CheckExprResult rhs_result = 
        check_expression(ptr->rhs.get(), scope_id);

    if(!is_type_integral(lhs_result.type_decl))
    {
        print_error_location(ptr->lhs->line, ptr->lhs->col);
        std::cout << " -> Expression result must have integral type.\n";
        exit(1);
    }

    if(!is_type_uintegral(rhs_result.type_decl))
    {
        print_error_location(ptr->rhs->line, ptr->rhs->col);
        std::cout << " -> Expression result must have an unsigned integral "
            "type.\n";
        exit(1);
    }

    return lhs_result;
}

TypeChecker::CheckExprResult TypeChecker::check_cmp_expr(const BinaryExpr *ptr,
    const uint64_t scope_id, bool is_equals_expr)
{
    CheckExprResult lhs_result = 
        check_expression(ptr->lhs.get(), scope_id);
    CheckExprResult rhs_result = 
        check_expression(ptr->rhs.get(), scope_id);

   if(!is_type_numeric(lhs_result.type_decl) && 
        !(is_equals_expr && is_type_bool(lhs_result.type_decl)))
    {
        print_error_location(ptr->lhs->line, ptr->lhs->col);
        std::cout << " -> Invalid type for comparison.\n";
        exit(1);
    }

    if(!cmp_types(lhs_result.type_decl, rhs_result.type_decl))
    {
        print_error_location(ptr->lhs->line, ptr->lhs->col);
        std::cout << " -> Type mismatch in comparing type: \"";
        print_type(lhs_result.type_decl);
        std::cout << "\" with type: \"";
        print_type(rhs_result.type_decl);
        std::cout << "\".\n";
        exit(1);
    }

    NamedTypeDecl *new_decl = new NamedTypeDecl();
    decls_to_delete.push_back(new_decl);

    new_decl->line = ptr->lhs->line;
    new_decl->col = ptr->lhs->col;

    new_decl->builtin_data.emplace(BuiltinData{
        {s_table->builtin_to_id.at("bool")}});
    new_decl->ident_path.push_back("bool");
    new_decl->resolved_symbol_idx.emplace(s_table->builtin_to_id.at("bool"));
    
    CheckExprResult final_result;
    
    final_result.is_lvalue = false;
    final_result.is_var_and_mutable = false;
    final_result.type_decl = new_decl;
    
    return final_result;
}

TypeChecker::CheckExprResult TypeChecker::check_binary_expr(
    const BinaryExpr *ptr, const uint64_t scope_id)
{
    switch(ptr->op_type)
    {
        case BinaryOp::ADD:
            return check_arith_expr(ptr, scope_id, true);

        case BinaryOp::SUB:
            return check_arith_expr(ptr, scope_id, true);

        case BinaryOp::MUL:
            return check_arith_expr(ptr, scope_id, false);

        case BinaryOp::DIV:
            return check_arith_expr(ptr, scope_id, false);

        case BinaryOp::MOD:
            return check_arith_expr(ptr, scope_id, false);

        case BinaryOp::LSHIFT:
            return check_shift_expr(ptr, scope_id);

        case BinaryOp::RSHIFT:
            return check_shift_expr(ptr, scope_id);

        case BinaryOp::LT:
            return check_cmp_expr(ptr, scope_id, false);

        case BinaryOp::GT:
            return check_cmp_expr(ptr, scope_id, false);

        case BinaryOp::LE:
            return check_cmp_expr(ptr, scope_id, false);

        case BinaryOp::GE:
            return check_cmp_expr(ptr, scope_id, false);

        case BinaryOp::EQ:
            return check_cmp_expr(ptr, scope_id, true);

        case BinaryOp::NE:
            return check_cmp_expr(ptr, scope_id, true);

        case BinaryOp::BIT_AND:
            return check_arith_expr(ptr, scope_id, false);

        case BinaryOp::BIT_OR:
            return check_arith_expr(ptr, scope_id, false);

        case BinaryOp::BIT_XOR:
            return check_arith_expr(ptr, scope_id, false);

        // case BinaryOp::LOG_AND:
        //     break;

        // case BinaryOp::LOG_OR:
        //     break;

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid BinaryOp type found\n";
            exit(1);
    }
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
            
            const uint64_t builtin_float_id = 
                s_table->builtin_to_id.at("float");

            // new_decl->builtin_data.emplace(
            //     BuiltinData{{BuiltinType::FLOAT, BuiltinType::DOUBLE}});
            new_decl->builtin_data.emplace(
                BuiltinData{
                    {builtin_float_id,
                    s_table->builtin_to_id.at("double")}});
            new_decl->ident_path.push_back("float");
            new_decl->resolved_symbol_idx.emplace(builtin_float_id);

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
            
            const uint64_t builtin_char_id = 
                s_table->builtin_to_id.at("char");

            // new_decl->builtin_data.emplace(BuiltinData{{BuiltinType::CHAR}});
            new_decl->builtin_data.emplace(
                BuiltinData{{builtin_char_id}});
            new_decl->ident_path.push_back("char");
            new_decl->resolved_symbol_idx.emplace(builtin_char_id);

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
            
            const uint64_t builtin_bool_id = 
                s_table->builtin_to_id.at("bool");

            // new_decl->builtin_data.emplace(
            //     BuiltinData{{BuiltinType::BOOL}});
            new_decl->builtin_data.emplace(
                BuiltinData{{builtin_bool_id}});
            new_decl->ident_path.push_back("bool");
            new_decl->resolved_symbol_idx.emplace(builtin_bool_id);

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

        case ExpressionType::UNARY:
            return check_unary_expr(static_cast<const UnaryExpr*>(ptr), 
                scope_id);

        case ExpressionType::BINARY:
            return check_binary_expr(static_cast<const BinaryExpr*>(ptr),
                scope_id);

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

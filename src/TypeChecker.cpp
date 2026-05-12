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
    std::cerr << program->source_file_name << ':' << line << ':' << col;
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
                std::cerr << "[]";
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
                std::cerr << reint_ptr->ident_path[i];

                if(i < reint_ptr->ident_path.size() - 1)
                    std::cerr << "::";
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

                        std::cerr << "CStr_ptr";
                        return;

                    case BuiltinPtrType::OPAQUE_PTR:

                        std::cerr << "Opaque_ptr";
                        return;

                    case BuiltinPtrType::NULL_PTR:

                        std::cerr << "Null_ptr";
                        return;
                }
            }

            std::cerr << '*';
            if(reint_ptr->points_to_mutable) std::cerr << "mut";
            std::cerr << ' ';
            print_type(reint_ptr->pointee.get());
            break;
        }

        case TypeKind::REF:
        {
            const RefTypeDecl *reint_ptr = 
                static_cast<const RefTypeDecl*>(ptr);

            std::cerr << "ref ";
            if(reint_ptr->ref_to_mutable) std::cerr << "mut ";
            print_type(reint_ptr->referred.get());
            break;
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid TypeKind found\n";
            exit(1);
    }
}

void TypeChecker::print_type_mismatch(const TypeDecl *expected, 
    const TypeDecl *received, uint32_t expr_line, uint32_t expr_col) const
{
    print_error_location(expr_line, expr_col);
    std::cerr << " -> Expected type: \"";
    print_type(expected);
    std::cerr << "\" but got type: \"";
    print_type(received);
    std::cerr << "\"\n";
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

void TypeChecker::cmp_ptr_types(const PtrTypeDecl *first, 
    const PtrTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const
{
    if(second->builtin_type.has_value())
    {
        std::cerr << "rhs is a nullptr\n";

        // Nullptr can be assigned to any pointer.
        return;
    }

    if(first->points_to_mutable && !second->points_to_mutable)
    {
        print_error_location(second->line, second->col);
        std::cerr << " -> Cannot assign pointer to non mutable: \"";
        print_type(second);
        std::cerr << "\" to pointer to mutable: \"";
        print_type(first);
        std::cerr << "\"\n";
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
            std::cerr << "lhs is i8\n";
            allowed_types.push_back(BuiltinType::I8);
        }
        
        else if(first_sym_idx == s_table->builtin_to_id.at("u8"))
        {
            std::cerr << "lhs is u8\n";
            allowed_types.push_back(BuiltinType::U8);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i16"))
        {
            std::cerr << "lhs is i16\n";
            allowed_types.push_back(BuiltinType::I8);
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::I16);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u16"))
        {
            std::cerr << "lhs is u16\n";
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::U16);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i32") || 
            first_sym_idx == s_table->builtin_to_id.at("int"))
        {
            std::cerr << "lhs is i32\n";
            allowed_types.push_back(BuiltinType::I8);
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::I16);
            allowed_types.push_back(BuiltinType::U16);
            allowed_types.push_back(BuiltinType::I32);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u32"))
        {
            std::cerr << "lhs is u32\n";
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::U16);
            allowed_types.push_back(BuiltinType::U32);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i64"))
        {
            std::cerr << "lhs is i64\n";
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
            std::cerr << "lhs is u64\n";
            allowed_types.push_back(BuiltinType::U8);
            allowed_types.push_back(BuiltinType::U16);
            allowed_types.push_back(BuiltinType::U32);
            allowed_types.push_back(BuiltinType::U64);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("bool"))
        {
            std::cerr << "lhs is char\n";
            allowed_types.push_back(BuiltinType::BOOL);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("char"))
        {
            std::cerr << "lhs is char\n";
            allowed_types.push_back(BuiltinType::U8);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("float"))
        {
            std::cerr << "lhs is float\n";
            allowed_types.push_back(BuiltinType::FLOAT);
        }

        else
        {
            std::cerr << "lhs could not be determined\n";
        }

        std::cerr << "rhs is a literal: " << second->ident_path.at(0) << '\n';

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
            std::cerr << " -> Cannot assign literal of type: \"" << 
                *second->builtin_type << "\" to a type of: \"";
            print_type(first);
            std::cerr << "\"\n";
            exit(1);
        }

        std::cerr << "Literal is allowed\n";

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
            std::cerr << " -> Invalid TypeKind found\n";
            exit(1);
    }
}

void TypeChecker::check_type(const TypeDecl *ptr) const
{
    switch(ptr->kind)
    {
        case TypeKind::NAMED:
        {
            const NamedTypeDecl * reint_ptr = 
                static_cast<const NamedTypeDecl*>(ptr);

            if(s_table->type_symbol_ids.find(
                reint_ptr->resolved_symbol_idx.value()) == 
                s_table->type_symbol_ids.end())
            {
                print_error_location(ptr->line, ptr->col);
                std::cerr << " -> \"";
                print_ident_path(reint_ptr->ident_path, std::cerr);
                std::cerr << "\" does not name a type\n";
                exit(1);
            }

            break;
        }

        case TypeKind::PTR:
        {
            
            break;
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

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid TypeKind found\n";
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
    std::cerr << "\nChecking Statement of type: " << ptr->stmt_type << '\n';

    switch(ptr->stmt_type)
    {
        // case StatementType::EXPR:
        // {
        //     break;
        // }

        case StatementType::VAR_DECL:
        {
            const VarDeclStmt *reint_ptr = static_cast<const VarDeclStmt*>(ptr); 

            check_type(reint_ptr->type_decl.get());

            if(reint_ptr->init_expr != nullptr)
            {
                const CheckExprResult init_expr_result = 
                    check_expression(reint_ptr->init_expr.get());

                cmp_types(reint_ptr->type_decl.get(),
                    init_expr_result.type_decl, reint_ptr->init_expr->line,
                    reint_ptr->init_expr->col);
            }

            break;
        }

        // case StatementType::STRUCT_DECL:
        // {
        //     break;
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
                std::cerr << " -> Unexpected return expression for function "
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
                    std::cerr << " -> Expected a return expression for "
                        "function with non void return type.\n";
                }
            }

            const CheckExprResult ret_expr_result = 
                check_expression(reint_ptr->ret_expr.get());

            cmp_types(*expected_return_type, ret_expr_result.type_decl, 
                reint_ptr->ret_expr->line, reint_ptr->ret_expr->col);

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
            std::cerr << " -> Invalid StatementType found.\n";
            exit(1);
    }
}

void TypeChecker::check_param(const Parameter * ptr)
{
    std::cerr << "Checking Parameter: " << ptr->name << '\n';
    
    std::cerr << "Checking Parameter not implemented\n";
    exit(1);
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
    if(fits_in<int8_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I8);
        new_decl->ident_path.push_back("i8");
        new_decl->resolved_symbol_idx = s_table->builtin_to_id.at("i8");
    }

    else if(fits_in<uint8_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U8);
        new_decl->ident_path.push_back("u8");
        new_decl->resolved_symbol_idx = s_table->builtin_to_id.at("u8");
    }

    else if(fits_in<int16_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I16);
        new_decl->ident_path.push_back("i16");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("i16");
    }

    else if(fits_in<uint16_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U16);
        new_decl->ident_path.push_back("u16");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("u16");
    }

    else if(fits_in<int32_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I32);
        new_decl->ident_path.push_back("i32");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("i32");
    }

    else if(fits_in<uint32_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U32);
        new_decl->ident_path.push_back("u32");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("u32");
    }

    else if(fits_in<int64_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::I64);
        new_decl->ident_path.push_back("i64");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("i64");
    }

    else if(fits_in<uint64_t>(s, option))
    {
        new_decl->builtin_type.emplace(BuiltinType::U64);
        new_decl->ident_path.push_back("u64");
        new_decl->resolved_symbol_idx = 
            s_table->builtin_to_id.at("u64");
    }

    else
    {
        print_error_location(new_decl->line, new_decl->col);
        std::cerr << " -> Unable to convert Binary number: " << s << 
            '\n';
        exit(1);
    }
}

TypeChecker::CheckExprResult TypeChecker::check_expression
    (const Expression *ptr)
{
    std::cerr << "Checking expression of type: " << ptr->exp_type << '\n';

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
            
            new_decl->builtin_type.emplace(BuiltinType::U8);
            new_decl->ident_path.push_back("u8");
            new_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("u8"));

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
            const IdentExpr *reint_ptr = static_cast<const IdentExpr*>(ptr);

            const Symbol *ident_symbol = s_table->symbols.at(
                reint_ptr->resolved_symbol_idx.value()).get();

            if(ident_symbol->sym_type == SymbolType::VAR)
            {
                const VarSymbol *reint_symbol = 
                    static_cast<const VarSymbol*>(ident_symbol);

                const VarDeclStmt * var_stmt = reint_symbol->ast_node_ptr;
            
                expr_result.is_var_and_mutable = var_stmt->is_binding_mutable;
                expr_result.type_decl = var_stmt->type_decl.get();
            }
            
            else if(ident_symbol->sym_type == SymbolType::PARAM)
            {
                const ParamSymbol *reint_symbol = 
                    static_cast<const ParamSymbol*>(ident_symbol);

                const Parameter *param = reint_symbol->ast_node_ptr;

                expr_result.is_var_and_mutable = param->is_binding_mutable;
                expr_result.type_decl = param->type_decl.get();
            }

            else
            {
                print_error_location(ptr->line, ptr->col);
                std::cerr << " -> \"";
                print_ident_path(reint_ptr->ident_path, std::cerr);
                std::cerr << "\" does not name a variable.\n";
                exit(1);
            }

            break;
        }

        // case ExpressionType::STRUCT_INIT:
        // {
        //     break;
        // }

        // case ExpressionType::ARR_INIT:
        // {
        //     break;
        // }

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
            std::cerr << " -> Invalid ExpressionType found\n";
            exit(1);
    }

    return expr_result;
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

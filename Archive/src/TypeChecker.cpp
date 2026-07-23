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

template<typename T>
static bool fits_in(const std::string &s, FitsInOption option, 
    T *output = nullptr) 
{
    std::from_chars_result result;
    T sink;

    switch(option)
    {
        case FitsInOption::NUMBER:
        {
            result = std::from_chars(s.data(), s.data() + s.size(), sink);

            bool success = result.ec == std::errc() && 
                result.ptr == s.data() + s.size();

            if(output != nullptr)
            {
                *output = sink;
            }

            return success;
        }

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

            for(uint8_t i = 0; i < reint_ptr->size_exprs.size(); ++i)
            {
                std::cout << '[' << reint_ptr->size_expr_as_num.at(i) << ']';
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

            if(reint_ptr->literal_data.has_value())
            {
                if(reint_ptr->literal_data->acceptable_builtin_ids.size() == 0)
                    break;

                const std::vector<BuiltinType> &bt_types = 
                    reint_ptr->literal_data->acceptable_builtin_ids;


                std::cout << bt_types.front();

                for(size_t i = 1; i < bt_types.size(); ++i)
                {
                    std::cout << '|' << bt_types[i];
                }
                break;
            }


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

void TypeChecker::print_invalid_integral_type(const TypeDecl *type,
    uint32_t line, uint32_t col) const
{
    print_error_location(line, col);
    std::cout << " -> Expected integral type but got type: \"";
    print_type(type);
    std::cout << "\"\n";
}

void TypeChecker::print_invalid_bool_type(const TypeDecl *type,
    uint32_t line, uint32_t col) const
{
    print_error_location(line, col);
    std::cout << " -> Expected bool type but got type: \"";
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

std::optional<uint64_t> TypeChecker::expr_to_uint64(const Expression *ptr) const
{
    if(ptr->exp_type != ExpressionType::INT_LITERAL) return {};

    const IntLitExpr * reint_ptr = static_cast<const IntLitExpr*>(ptr);

    uint64_t val;
    
    if(!fits_in<uint64_t>(reint_ptr->value, FitsInOption::NUMBER,
        &val)) return {};

    return {val};
}

bool TypeChecker::is_type_literal(const TypeDecl *ptr) const
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    return reint_ptr->literal_data.has_value();
}

bool TypeChecker::is_type_numeric(const TypeDecl *ptr) const
{
    // if(ptr->kind != TypeKind::NAMED || ptr->kind != TypeKind::REF) return false;

    // if(ptr->kind == TypeKind::REF)
    // {
    //     return is_type_numeric(
    //         static_cast<const RefTypeDecl*>(ptr)->referred.get());
    // }
    
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    if(reint_ptr->literal_data.has_value())
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

    if(reint_ptr->literal_data.has_value())
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

    const NamedTypeDecl *reint_ptr = static_cast<const NamedTypeDecl*>(ptr);

    // If we are dealing with a builtin type.
    if(reint_ptr->literal_data.has_value())
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
    const std::unordered_set<BuiltinType> acceptable_ids)
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = 
        static_cast<const NamedTypeDecl*>(ptr);

    if(!reint_ptr->literal_data.has_value()) return false;

    const LiteralData &lt_data = *reint_ptr->literal_data;

    for(BuiltinType bt_id : lt_data.acceptable_builtin_ids)
    {
        if(acceptable_ids.find(bt_id) !=  acceptable_ids.end()) return true;
    }

    return false;
}

bool TypeChecker::is_type_numeric_literal(const TypeDecl *ptr) const
{
    std::unordered_set<BuiltinType> acceptable_ids;
    acceptable_ids.emplace(BuiltinType::I8);
    acceptable_ids.emplace(BuiltinType::U8);
    acceptable_ids.emplace(BuiltinType::I16);
    acceptable_ids.emplace(BuiltinType::U16);
    acceptable_ids.emplace(BuiltinType::I32);
    acceptable_ids.emplace(BuiltinType::U32);
    acceptable_ids.emplace(BuiltinType::I64);
    acceptable_ids.emplace(BuiltinType::U64);
    acceptable_ids.emplace(BuiltinType::FLOAT);
    acceptable_ids.emplace(BuiltinType::DOUBLE);
    return is_type_literal_helper(ptr, acceptable_ids);
}

bool TypeChecker::is_type_int_literal(const TypeDecl *ptr) const
{
    std::unordered_set<BuiltinType> acceptable_ids;
    acceptable_ids.emplace(BuiltinType::I8);
    acceptable_ids.emplace(BuiltinType::U8);
    acceptable_ids.emplace(BuiltinType::I16);
    acceptable_ids.emplace(BuiltinType::U16);
    acceptable_ids.emplace(BuiltinType::I32);
    acceptable_ids.emplace(BuiltinType::U32);
    acceptable_ids.emplace(BuiltinType::I64);
    acceptable_ids.emplace(BuiltinType::U64);

    return is_type_literal_helper(ptr, acceptable_ids);
}

bool TypeChecker::is_type_uint_literal(const TypeDecl *ptr) const
{
    std::unordered_set<BuiltinType> acceptable_ids;
    acceptable_ids.emplace(BuiltinType::U8);
    acceptable_ids.emplace(BuiltinType::U16);
    acceptable_ids.emplace(BuiltinType::U32);
    acceptable_ids.emplace(BuiltinType::U64);

    return is_type_literal_helper(ptr, acceptable_ids);
}

bool TypeChecker::is_type_bool(const TypeDecl *ptr) const
{
    if(ptr->kind != TypeKind::NAMED) return false;

    const NamedTypeDecl *reint_ptr = static_cast<const NamedTypeDecl*>(ptr);

    if(reint_ptr->literal_data.has_value())
    {
        return reint_ptr->literal_data->acceptable_builtin_ids.size() > 0 &&
            reint_ptr->literal_data->acceptable_builtin_ids.at(0) == 
                BuiltinType::BOOL;
    }

    // Builtins live at the top scope.
    if(reint_ptr->ident_path.size() != 1) return false;
    
    return reint_ptr->ident_path.front() == "bool";
}

std::optional<const StructDecl*> TypeChecker::is_namedtype_struct(
    const NamedTypeDecl *ptr) const 
{
    const Symbol *sym = 
        s_table->symbols.at(ptr->resolved_symbol_idx.value()).get();

    if(sym->sym_type != SymbolType::STRUCT) return {};

    return static_cast<const StructSymbol*>(sym)->ast_node_ptr;
}

std::optional<const ComponentDecl*> TypeChecker::is_namedtype_component(
    const NamedTypeDecl *ptr) const 
{
    const Symbol *sym = 
        s_table->symbols.at(ptr->resolved_symbol_idx.value()).get();

    if(sym->sym_type != SymbolType::COMPONENT) return {};

    return static_cast<const ComponentSymbol*>(sym)->ast_node_ptr;
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

    // If the first pointer is an opaque_ptr
    if(first->builtin_type.has_value() && 
        *first->builtin_type == BuiltinPtrType::OPAQUE_PTR)
    {
        // If the second pointer is not an opaque_ptr
        if(!second->builtin_type.has_value() || 
            *second->builtin_type != BuiltinPtrType::OPAQUE_PTR)
        {
            return false;
        }

        return true;
    }

    // If the second pointer is an opaque_ptr
    if(second->builtin_type.has_value() && 
        *second->builtin_type == BuiltinPtrType::OPAQUE_PTR)
    {
        // If the first pointer is not an opaque_ptr
        if(!first->builtin_type.has_value() ||
            *first->builtin_type != BuiltinPtrType::OPAQUE_PTR)
        {
            return false;
        }

        return true;
    }

    if(first->points_to_mutable && !second->points_to_mutable) return false;

    return cmp_types(first->pointee.get(), second->pointee.get());
}

bool TypeChecker::cmp_arr_types(const ArrTypeDecl *first, 
    const ArrTypeDecl *second) const
{
    if(!cmp_types(first->element_type.get(), second->element_type.get()))
        return false;

    if(first->size_exprs.size() != second->size_exprs.size()) return false;

    if(first->size_exprs.size() != first->size_expr_as_num.size())
    {
        std::cout << __FILE__ << ":"<<  __LINE__ << 
            ": Something aint lining up here\n";
        exit(1);
    }

    if(second->size_exprs.size() != second->size_expr_as_num.size())
    {
        std::cout << __FILE__ << ":"<<  __LINE__ << 
            ": Something aint lining up here\n";
        exit(1);
    }


    for(uint64_t i = 0; i < first->size_expr_as_num.size(); ++i)
    {
        if(first->size_expr_as_num.at(i) != second->size_expr_as_num.at(i))
            return false;
    }

    return true;
}

bool TypeChecker::cmp_ref_types(const RefTypeDecl *first,
    const RefTypeDecl *second) const
{
    if(first->ref_to_mutable && !second->ref_to_mutable) return false;

    return cmp_types(first->referred.get(), second->referred.get());
}

bool TypeChecker::cmp_two_builtin_type(const NamedTypeDecl *first_builtin, 
    const NamedTypeDecl *second_builtin) const
{
    for(BuiltinType first_bt_type : 
        first_builtin->literal_data->acceptable_builtin_ids)
    {
        for(BuiltinType second_bt_type : 
            second_builtin->literal_data->acceptable_builtin_ids)
        {
            if(first_bt_type == second_bt_type) return true;
        }
    }

    return false;
}

bool TypeChecker::cmp_builtin_type(const NamedTypeDecl *builtin_type,
    const NamedTypeDecl *other_type) const
{
    const uint64_t other_sym_id = other_type->resolved_symbol_idx.value();

    for(BuiltinType bt_id : builtin_type->literal_data->acceptable_builtin_ids)
    {
        const uint64_t bt_sym_id = s_table->builtin_to_id.at(
            readable_to_builtin.at(static_cast<int>(bt_id)).first);

        if(bt_sym_id == other_sym_id) return true;
    }

    return false;
}

bool TypeChecker::cmp_named_types(const NamedTypeDecl *first, 
    const NamedTypeDecl *second) const
{
    if(first->literal_data.has_value() && second->literal_data.has_value())
    {
        return cmp_two_builtin_type(first, second);
    }

    if(first->literal_data.has_value())
        return cmp_builtin_type(first, second);

    if(second->literal_data.has_value())
        return cmp_builtin_type(second, first);

    return first->resolved_symbol_idx.value() ==
           second->resolved_symbol_idx.value();

    return true;
}

bool TypeChecker::cmp_types(const TypeDecl *first, const TypeDecl *second)
    const
{
    if(first->kind != second->kind)
    {
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

        case TypeKind::REF:
        {
            return cmp_ref_types(static_cast<const RefTypeDecl*>(first),
                static_cast<const RefTypeDecl*>(second));
        }

        case TypeKind::ARRAY:
        {
            return cmp_arr_types(static_cast<const ArrTypeDecl*>(first),
                static_cast<const ArrTypeDecl*>(second));
        }

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

void TypeChecker::check_type(TypeDecl *ptr, const uint64_t type_scope_id,
    TypeKind last_type)
{
    switch(ptr->kind)
    {
        case TypeKind::NAMED:
        {
            const NamedTypeDecl *reint_ptr = 
                static_cast<const NamedTypeDecl*>(ptr);

            if(reint_ptr->literal_data.has_value()) return;

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
            
            check_type(reint_ptr->pointee.get(), type_scope_id, TypeKind::PTR);
            break;
        }

        case TypeKind::REF:
        {
            if(last_type == TypeKind::PTR)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> Cannot have a pointer to a reference.\n";
                exit(1);
            }

            else if(last_type == TypeKind::REF)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> Cannot have a reference to a reference.\n";
                exit(1);
            }

            const RefTypeDecl *reint_ptr = 
                static_cast<const RefTypeDecl*>(ptr);

            check_type(reint_ptr->referred.get(), type_scope_id, 
                TypeKind::REF);
            break;
        }

        case TypeKind::ARRAY:
        {
            // std::cout << "Checking arr type\n";
            ArrTypeDecl *reint_ptr = 
                static_cast<ArrTypeDecl*>(ptr);

            check_type(reint_ptr->element_type.get(), type_scope_id);

            // for(uint8_t i = 0; i < reint_ptr->size_exprs.size(); ++i)
            // {
            //     std::optional<uint64_t> size_val = 
            //         expr_to_uint64(reint_ptr->size_exprs.at(i).get());

            //     if(!size_val.has_value())
            //     {
            //         print_error_location(reint_ptr->size_exprs[i]->line,
            //             reint_ptr->size_exprs[i]->col);
            //         std::cout << " -> Array size expression must be an unsigned"
            //             " int literal type.\n";
            //         exit(1);
            //     }

            //     reint_ptr->size_expr_as_num.push_back(*size_val);
            // }

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

void TypeChecker::check_named_cast(const NamedTypeDecl *casting_type,
    const NamedTypeDecl *being_casted_type, uint32_t expr_line, 
        uint32_t expr_col)
{
    const uint64_t casting_type_sym_idx = 
        casting_type->resolved_symbol_idx.value();

    if(s_table->builtin_symbol_ids.find(casting_type_sym_idx) ==
        s_table->builtin_symbol_ids.end() || 
        casting_type_sym_idx == s_table->builtin_to_id.at("void"))
    {
        print_error_location(expr_line, expr_col);
        std::cout << " -> Cannot cast to this type.\n";
        exit(1);
    }

    // What's being casted is a literal. Any literal is ok?
    if(being_casted_type->literal_data.has_value()) return;

    const uint64_t being_casted_sym_idx = 
        being_casted_type->resolved_symbol_idx.value();

        if(s_table->builtin_symbol_ids.find(being_casted_sym_idx) == 
            s_table->builtin_symbol_ids.end())
        {
            print_error_location(expr_line, expr_col);
            std::cout << " -> Cannot cast this expression's type: \"";
            print_type(being_casted_type);
            std::cout << "\".\n";
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

    const bool var_is_ref_type = ptr->type_decl->kind == TypeKind::REF;

    if(ptr->is_binding_mutable && var_is_ref_type)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Variables with a reference type cannot be marked "
            "mutable.\n";
        exit(1);
    }

    if(ptr->init_expr == nullptr)
    {
        if(var_is_ref_type)
        {
            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Variables with a reference type must have an "
                "initialization expression\n";
            exit(1);
        }

        return;
    }

    // We have an init expression.

    if(ptr->init_expr->exp_type == ExpressionType::STRUCT_INIT)
    {
        const StructInitExpr *init_expr = 
            static_cast<const StructInitExpr*>(ptr->init_expr.get());

        check_struct_init_expr(
            init_expr, ptr->type_decl.get(), var_sym->scope_idx.value());
        return;
    }

    if(ptr->type_decl->kind == TypeKind::ARRAY)
    {
        if(ptr->init_expr->exp_type != ExpressionType::ARR_INIT)
        {
            print_error_location(ptr->init_expr->line, ptr->init_expr->col);
            std::cout << " -> Array variables can only be initialized with "
                "Array initializers.\n";
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

    // We don't have an Array type.

    if(ptr->init_expr->exp_type == ExpressionType::ARR_INIT)
    {
        print_error_location(ptr->init_expr->line, ptr->init_expr->col);
        std::cout << " -> Cannot initialize non Array type with an Array "
            "initialization expression\n";
        exit(1);
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

    const Symbol *symbol = s_table->symbols.at(ptr->symbol_idx.value()).get();

    if(symbol->sym_type != SymbolType::PARAM)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Parameter found with non Parameter symbol type.\n";
        exit(1);
    }

    const ParamSymbol *reint_sym = static_cast<const ParamSymbol*>(symbol);

    check_type(ptr->type_decl.get(), reint_sym->scope_idx.value());

    if(ptr->type_decl->kind == TypeKind::ARRAY)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Parameters cannot have direct Array types.\n";
        exit(1);
    }

    if(!ptr->passed_by_copy && ptr->type_decl->kind != TypeKind::REF)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Parameter pass in behavior not marked \"val\" or "
            "\"ref\".\n";
        exit(1);
    }

    if(ptr->type_decl->kind == TypeKind::REF && ptr->is_binding_mutable)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Parameters with a reference type cannot be "
            "marked mutable.\n";
        exit(1);
    }   
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
void handle_fits_in(NamedTypeDecl *new_decl, const std::string &s, 
    FitsInOption option, const BuiltinType builtin_type)
{
    if(fits_in<T>(s, option))
    {
        new_decl->literal_data->acceptable_builtin_ids.push_back(builtin_type);
    }
}

void TypeChecker::init_literal_typedecl(NamedTypeDecl *new_decl, 
    const std::string &s, FitsInOption option) const 
{
    new_decl->literal_data.emplace(LiteralData{});
    
    handle_fits_in<int8_t>(new_decl, s, option, 
        BuiltinType::I8);
    handle_fits_in<uint8_t>(new_decl, s, option, 
        BuiltinType::U8);
    handle_fits_in<int16_t>(new_decl, s, option, 
        BuiltinType::I16);
    handle_fits_in<uint16_t>(new_decl, s, option, 
        BuiltinType::U16);
    handle_fits_in<int32_t>(new_decl, s, option, 
        BuiltinType::I32);
    handle_fits_in<uint32_t>(new_decl, s, option, 
        BuiltinType::U32);
    handle_fits_in<int64_t>(new_decl, s, option, 
        BuiltinType::I64);
    handle_fits_in<uint64_t>(new_decl, s, option, 
        BuiltinType::U64);

    if(new_decl->literal_data->acceptable_builtin_ids.size() == 0)
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

const FieldDecl* TypeChecker::get_struct_member(const StructDecl *decl, 
    const std::string &member_name, uint32_t expr_line, uint32_t expr_col)
{
    for(const std::unique_ptr<Declaration> &d : decl->decls)
    {
        if(d->kind != DeclKind::FIELD) continue;

        const FieldDecl *reint_d = static_cast<const FieldDecl*>(d.get());

        if(reint_d->name == member_name) return reint_d;
    }

    print_error_location(expr_line, expr_col);
    std::cout << " -> Struct member: \"" << member_name << "\" does not "
        "exist.\n";
    exit(1);
}

const FieldDecl* TypeChecker::get_comp_member(const ComponentDecl *decl,
    const std::string &member_name, uint32_t expr_line, uint32_t expr_col)
{
    for(const std::unique_ptr<Declaration> &d : decl->decls)
    {
        if(d->kind != DeclKind::FIELD) continue;

        const FieldDecl *reint_d = static_cast<const FieldDecl*>(d.get());

        if(reint_d->name == member_name) return reint_d;
    }

    print_error_location(expr_line, expr_col);
    std::cout << " -> Component member: \"" << member_name << "\" does not "
        "exist.\n";
    exit(1);
}

const FunctionDecl* TypeChecker::get_func_decl(const Expression *ptr)
{
    if(ptr->exp_type != ExpressionType::IDENTIFIER || 
        ptr->exp_type != ExpressionType::MEMBER_ACCESS)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Type must be a function to call it.\n";
        exit(1);
    }

    // If we are handling a function called by accessing a component object.
    if(ptr->exp_type == ExpressionType::MEMBER_ACCESS)
    {
        const MemberAccExpr *reint_ptr = static_cast<const MemberAccExpr*>(ptr);

        

    }


    const IdentExpr *reint_ptr = static_cast<const IdentExpr*>(ptr);

    const Symbol *symbol = s_table->symbols.at(
        reint_ptr->resolved_symbol_idx.value()).get();

    if(symbol->sym_type != SymbolType::FN)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Type must be a function to call it.\n";
        exit(1);
    }

    const FunctionSymbol *reint_symbol = 
        static_cast<const FunctionSymbol*>(symbol);

    return reint_symbol->ast_node_ptr;
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
            "Array type dimensions.\n";
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
                std::cout << " -> Expected Array initialization expression.\n";
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

    recurse_check_arr_init(init_expr, 0, arr_type->size_expr_as_num,
        arr_type->element_type.get(), var_scope_id);

    CheckExprResult expr_result;

    expr_result.is_lvalue = false;
    expr_result.is_var_and_mutable = false;
    expr_result.type_decl = arr_type;

    return expr_result;
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

            // // If the type of this variable is a reference, then whether the
            // // variable is mutable depends on if the referred to variable is 
            // // mutable.
            // if(var_stmt->type_decl->kind == TypeKind::REF)
            // {
            //     const RefTypeDecl *reint_type = 
            //         static_cast<const RefTypeDecl*>(var_stmt->type_decl.get());

            //     expr_result.is_var_and_mutable = reint_type->ref_to_mutable;
            //     expr_result.type_decl = reint_type->referred.get();
            // }
            // else 
            // {
            //     expr_result.is_var_and_mutable = var_stmt->is_binding_mutable;
            //     expr_result.type_decl = var_stmt->type_decl.get();
            // }


            expr_result.is_var_and_mutable = var_stmt->is_binding_mutable;
            expr_result.type_decl = var_stmt->type_decl.get();
            break;
        }

        case SymbolType::FN:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Unexpected function symbol.\n";
            exit(1);

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Unexpected symbol type\n";
            exit(1);

    }

    return expr_result;
}

TypeChecker::CheckExprResult TypeChecker::check_prepost_incdec(
    const UnaryExpr * ptr, const CheckExprResult operand_res)
{
    // CheckExprResult operand_res = 
    //     check_expression(ptr->operand.get(), scope_id);

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
    const UnaryExpr *ptr, const CheckExprResult operand_res)
{
    // CheckExprResult expr_result = 
    //     check_expression(ptr->operand.get(), scope_id);

    if(!operand_res.is_lvalue)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Can't take address of a non lvalue.\n";
        exit(1);
    }

    // if(operand_res.type_decl->kind == TypeKind::REF)
    // {
    //     // Strip away reference. 
    //     const RefTypeDecl *reint_type = 
    //         static_cast<const RefTypeDecl*>(operand_res.type_decl);

    //     operand_res.type_decl = reint_type->referred.get();
    //     operand_res.is_var_and_mutable = reint_type->ref_to_mutable;
    // }

    PtrTypeDecl * new_decl = new PtrTypeDecl();
    decls_to_delete.push_back(new_decl);

    new_decl->line = ptr->line;
    new_decl->col = ptr->col;
    new_decl->points_to_mutable = operand_res.is_var_and_mutable;

    // Our new PtrTypeDecl that we've had to create needs a unique ptr to the 
    // TypeDecl it's pointing to. Since it's a unique ptr and we only have a raw
    // pointer, we need to clone it and keep our own copy of the data.
    new_decl->pointee = operand_res.type_decl->clone();

    CheckExprResult final_result;

    final_result.type_decl = new_decl;
    // Temporary ptr to lvalue is not an lvalue.
    final_result.is_lvalue = false; 
    final_result.is_var_and_mutable = false;

    return final_result;
}

TypeChecker::CheckExprResult TypeChecker::check_ref_of_expr(
    const UnaryExpr *ptr, const CheckExprResult operand_res)
{
    // CheckExprResult expr_result = 
    //     check_expression(ptr->operand.get(), scope_id);

    if(!operand_res.is_lvalue)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Can't take reference of a non lvalue.\n";
        exit(1);
    }

    RefTypeDecl * new_decl = new RefTypeDecl();
    decls_to_delete.push_back(new_decl);

    new_decl->line = ptr->line;
    new_decl->col = ptr->col;
    new_decl->ref_to_mutable = operand_res.is_var_and_mutable;

    new_decl->referred = operand_res.type_decl->clone();

    CheckExprResult final_result;

    final_result.is_lvalue = false;
    final_result.is_var_and_mutable = false;
    final_result.type_decl = new_decl;

    return final_result;
}

TypeChecker::CheckExprResult TypeChecker::check_deref_expr(
    const UnaryExpr *ptr, const CheckExprResult operand_res)
{
    // CheckExprResult operand_result = 
    //     check_expression(ptr->operand.get(), scope_id);

    if(operand_res.type_decl->kind != TypeKind::PTR)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Can't dereference a non pointer type\n";
        exit(1);
    }

    const PtrTypeDecl *operand_reint_ptr = 
        static_cast<const PtrTypeDecl*>(operand_res.type_decl);

    if(operand_reint_ptr->builtin_type.has_value())
    {
        if(*operand_reint_ptr->builtin_type == BuiltinPtrType::NULL_PTR)
        {
            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Can't dereference a null_ptr\n";
            exit(1);
        }

        else if(*operand_reint_ptr->builtin_type == BuiltinPtrType::OPAQUE_PTR)
        {
            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Can't dereference an opaque_ptr.\n";
            exit(1);
        }
    }

    CheckExprResult final_result;

    final_result.type_decl = operand_reint_ptr->pointee.get();
    final_result.is_lvalue = operand_res.is_lvalue;
    final_result.is_var_and_mutable = final_result.is_lvalue && 
        operand_reint_ptr->points_to_mutable;

    return final_result;
}

TypeChecker::CheckExprResult TypeChecker::check_unary_expr(
    const UnaryExpr *ptr, const uint64_t scope_id)
{
    std::cout << "Checking Unary Expression of type: " <<
        ptr->op_type << '\n';
    
    CheckExprResult expr_result = 
        check_expression(ptr->operand.get(), scope_id);

    CheckExprResult ref_stripped_res = expr_result;
        
    if(ref_stripped_res.type_decl->kind == TypeKind::REF)
    {
        // Strip the reference off temporarily.
        const RefTypeDecl *reint_type =    
            static_cast<const RefTypeDecl*>(ref_stripped_res.type_decl);

        ref_stripped_res.type_decl = reint_type->referred.get();
        ref_stripped_res.is_var_and_mutable = reint_type->ref_to_mutable;
    }

    switch(ptr->op_type)
    {
        case UnaryOp::PRE_INC: 
        
            check_prepost_incdec(ptr, ref_stripped_res);
            break;

        case UnaryOp::PRE_DEC: 
        
            check_prepost_incdec(ptr, ref_stripped_res);
            break;

        case UnaryOp::POST_INC: 
        
            check_prepost_incdec(ptr, ref_stripped_res);
            break;

        case UnaryOp::POST_DEC: 
        
            check_prepost_incdec(ptr, ref_stripped_res);
            break;

        case UnaryOp::NEGATE:
        {
            if(!is_type_numeric(ref_stripped_res.type_decl))
            {
                print_invalid_integral_type(ref_stripped_res.type_decl, 
                    ptr->operand->line, ptr->operand->col);
                exit(1);
            }
            break;
        }

        case UnaryOp::BIT_NOT:
        {
            if(!is_type_numeric(ref_stripped_res.type_decl))
            {
                print_invalid_integral_type(ref_stripped_res.type_decl, 
                    ptr->operand->line, ptr->operand->col);
                exit(1);
            }
            break;
        }

        case UnaryOp::LOG_NOT:
        {
            // expr_result = check_expression(ptr->operand.get(), scope_id);

            if(!is_type_bool(ref_stripped_res.type_decl))
            {
                print_invalid_bool_type(ref_stripped_res.type_decl,
                    ptr->operand->line, ptr->operand->col);
                exit(1);
            }

            break;
        }

        case UnaryOp::ADDRESS_OF:
        {
            expr_result = check_addr_of_expr(ptr, ref_stripped_res);
            break;
        }

        case UnaryOp::REF_OF:
        {
            // Use the original expr_result rather than the potentially ref 
            // stripped operand_res, since we don't want to allow reference to 
            // a reference.
            expr_result = check_ref_of_expr(ptr, expr_result);
            break;
        }

        case UnaryOp::DEREF:
        {
            expr_result = check_deref_expr(ptr, ref_stripped_res);
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

    const TypeDecl *lhs_type = nullptr;
    const TypeDecl *rhs_type = nullptr;

    // We track if the rhs is a ptr or ref type. If it is, then we will return 
    // the rhs's type, since in scenarios such as "1 + ptr" we want to return
    // the ptr type.
    bool rhs_priority = false;

    if(lhs_result.type_decl->kind == TypeKind::REF)
    {
        // Need to strip the ref off this to get to the actual type.
        lhs_type = static_cast<const RefTypeDecl*>(
            lhs_result.type_decl)->referred.get();
    }
    else lhs_type = lhs_result.type_decl;

    if(rhs_result.type_decl->kind == TypeKind::REF)
    {
        rhs_priority = true;
        // Need to strip the ref off this to get to the actual type.
        rhs_type = static_cast<const RefTypeDecl*>(
            rhs_result.type_decl)->referred.get();
    }
    else rhs_type = rhs_result.type_decl;

    bool lhs_is_numerical = is_type_numeric(lhs_type);
    bool rhs_is_numerical = is_type_numeric(rhs_type);

    if(is_addsub_expr)
    {
        if(lhs_type->kind == TypeKind::PTR)
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

        if(rhs_type->kind == TypeKind::PTR)
        {
            rhs_priority = true;
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
        std::cout << " -> Invalid type for arithmetic: \"";
        print_type(lhs_type);
        std::cout << "\".\n";
        exit(1);
    }

    if(!rhs_is_numerical)
    {
        print_error_location(ptr->rhs->line, ptr->rhs->col);
        std::cout << " -> Invalid type for arithmetic: \"";
        print_type(rhs_type);
        std::cout << "\".\n";
        exit(1);
    }

    const bool lhs_literal = is_type_literal(lhs_type);
    const bool rhs_literal = is_type_literal(rhs_type);

    // We are dealing with two literal types. This means we need to construct 
    // a new type decl that contains the intersection of the literal values 
    // each type can take on. ie 255 + -1 leaves only i16, i32 and i64 types.
    if(lhs_literal && rhs_literal)
    {
        const NamedTypeDecl *lhs_reint = 
            static_cast<const NamedTypeDecl*>(lhs_type);
        const NamedTypeDecl *rhs_reint = 
            static_cast<const NamedTypeDecl*>(rhs_type);

        NamedTypeDecl *new_decl = new NamedTypeDecl();
        decls_to_delete.push_back(new_decl);
        
        new_decl->line = ptr->lhs->line;
        new_decl->col = ptr->lhs->col;

        new_decl->literal_data.emplace(LiteralData{});

        new_decl->ident_path.push_back("");

        for(BuiltinType lhs_bt_type : 
            lhs_reint->literal_data->acceptable_builtin_ids)
        {
            for(BuiltinType rhs_bt_type : 
                rhs_reint->literal_data->acceptable_builtin_ids)
            {
                if(lhs_bt_type == rhs_bt_type)
                {
                    new_decl->literal_data->acceptable_builtin_ids.push_back(
                        lhs_bt_type);
                }
            }

        }

        CheckExprResult final_result;

        final_result.is_lvalue = false;
        final_result.is_var_and_mutable = false;
        final_result.type_decl = new_decl;

        return final_result;
    }    

    // If just the lhs is a literal, it will assume the type of the rhs, so rhs
    // takes priority.
    else if(lhs_literal) rhs_priority = true;

    if(cmp_types(lhs_type, rhs_type))
    {
        CheckExprResult final_res;

        final_res.is_lvalue = false;
        final_res.is_var_and_mutable = false;
        final_res.type_decl = 
            rhs_priority ? rhs_result.type_decl : lhs_result.type_decl;

        return final_res;
    }

    print_error_location(ptr->line, ptr->col);
    std::cout << " -> Unable to perform arithmetic between non matching types:"
        " \"";
    print_type(lhs_type);
    std::cout << "\" and type: \"";
    print_type(rhs_type);
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

    if(!is_type_numeric(rhs_result.type_decl) && 
        !(is_equals_expr && is_type_bool(rhs_result.type_decl)))
    {
        print_error_location(ptr->rhs->line, ptr->rhs->col);
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

    new_decl->literal_data.emplace(LiteralData{{BuiltinType::BOOL}});
    // new_decl->resolved_symbol_idx.emplace(s_table->builtin_to_id.at("bool"));
    
    CheckExprResult final_result;
    
    final_result.is_lvalue = false;
    final_result.is_var_and_mutable = false;
    final_result.type_decl = new_decl;
    
    return final_result;
}

TypeChecker::CheckExprResult TypeChecker::check_log_expr(const BinaryExpr *ptr,
    const uint64_t scope_id)
{
    CheckExprResult lhs_result = 
        check_expression(ptr->lhs.get(), scope_id);
    CheckExprResult rhs_result = 
        check_expression(ptr->rhs.get(), scope_id);

    if(!is_type_bool(lhs_result.type_decl))
    {
        print_error_location(ptr->lhs->line, ptr->lhs->col);
        std::cout << " Invalid type for logic comparison: \"";
        print_type(lhs_result.type_decl);
        std::cout << "\".\n";
        exit(1);
    }

    if(!is_type_bool(rhs_result.type_decl))
    {
        print_error_location(ptr->rhs->line, ptr->rhs->col);
        std::cout << " Invalid type for logic comparison: \"";
        print_type(rhs_result.type_decl);
        std::cout << "\".\n";
        exit(1);
    }

    return lhs_result;
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

        case BinaryOp::LOG_AND:
            return check_log_expr(ptr, scope_id);

        case BinaryOp::LOG_OR:
            return check_log_expr(ptr, scope_id);

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid BinaryOp type found\n";
            exit(1);
    }
}

TypeChecker::CheckExprResult TypeChecker::check_cast_expr(
    const CastExpr *ptr, const uint64_t scope_id)
{
    TypeDecl *casting_type = ptr->to_cast_type.get();

    check_type(casting_type, scope_id);

    CheckExprResult being_casted_res = 
        check_expression(ptr->expr_to_cast.get(), scope_id);

    const TypeDecl *being_casted_type = being_casted_res.type_decl;

    if(casting_type->kind != TypeKind::NAMED &&
        casting_type->kind != TypeKind::PTR)
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Cannot cast to this type.\n";
        exit(1);
    }

    if(being_casted_type->kind != TypeKind::NAMED &&
        being_casted_type->kind != TypeKind::PTR)
    {
        print_error_location(ptr->expr_to_cast->line, ptr->expr_to_cast->col);
        std::cout << " -> This type cannot be casted.\n";
        exit(1);
    }

    if(casting_type->kind != being_casted_type->kind)
    {        
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Cannot cast between named types and pointer types.\n";
        exit(1);
    }

    if(casting_type->kind == TypeKind::NAMED)
        check_named_cast(static_cast<const NamedTypeDecl*>(casting_type),
            static_cast<const NamedTypeDecl*>(being_casted_type), 
        ptr->line, ptr->col);

    // Casting type is a pointer.

    being_casted_res.type_decl = casting_type;
    return being_casted_res;
}

TypeChecker::CheckExprResult TypeChecker::check_subscript_expr(
    const SubscriptExpr *ptr, const uint64_t scope_id)
{
    // Validate each index expression.
    for(const std::unique_ptr<Expression> &index_ptr : ptr->index_exprs)
    {
        CheckExprResult index_res = check_expression(index_ptr.get(), scope_id);

        if(!is_type_integral(index_res.type_decl))
        {
            print_error_location(index_ptr->line, index_ptr->col);
            std::cout << " -> Array index expression must be an integral "
                "type.\n";
            exit(1);
        }
    }

    CheckExprResult base_expr_res = 
        check_expression(ptr->base_expr.get(), scope_id);

    if(base_expr_res.type_decl->kind != TypeKind::ARRAY)
    {
        print_error_location(ptr->base_expr->line, ptr->base_expr->col);
        std::cout << " -> Can only subscript Array types.\n";
        exit(1);
    }

    const ArrTypeDecl * const arr_type = 
        static_cast<const ArrTypeDecl*>(base_expr_res.type_decl);

    if(ptr->index_exprs.size() > arr_type->size_exprs.size())
    {
        const std::unique_ptr<Expression> &problem_index = 
            ptr->index_exprs.at(arr_type->size_exprs.size());

        print_error_location(problem_index->line, problem_index->col);
        std::cout << " -> Index depth exceeds Array size depth.\n";
        exit(1);
    }

    // Simple case, the subscript indexes through ever depth of the array, so 
    // we should simply return the type the Array stores.
    if(ptr->index_exprs.size() == arr_type->size_exprs.size())
    {
        CheckExprResult ret_res;

        ret_res.is_lvalue = true; // Array elements are always lvalues
        ret_res.is_var_and_mutable = base_expr_res.is_var_and_mutable;
        ret_res.type_decl = arr_type->element_type.get();

        return ret_res;
    }

    // There are less index expressions than the depth of the Array, which means
    // we are subscripting to get an Array of some depth rather than an element.

    // We need to make a new ArrTypeDecl to represent the array being 
    // subscripted.

    ArrTypeDecl * new_decl = new ArrTypeDecl;
    decls_to_delete.push_back(new_decl);

    new_decl->line = ptr->line;
    new_decl->col = ptr->col;

    new_decl->element_type = arr_type->element_type->clone();

    for(uint64_t i = ptr->index_exprs.size(); i < arr_type->size_exprs.size(); 
        ++i)
    {
        new_decl->size_exprs.push_back(arr_type->size_exprs.at(i)->clone());
        new_decl->size_expr_as_num.push_back(arr_type->size_expr_as_num.at(i));
    }

    CheckExprResult expr_res;

    expr_res.is_lvalue = true;
    expr_res.is_var_and_mutable = false;
    expr_res.type_decl = new_decl;

    return expr_res;
}

TypeChecker::CheckExprResult TypeChecker::check_memaccess_expr(
    const MemberAccExpr *ptr, const uint64_t scope_id)
{
    CheckExprResult base_res = check_expression(ptr->base_expr.get(), scope_id);

    const NamedTypeDecl * typedecl_being_accessed = nullptr;

    if(ptr->via_pointer)
    {
        if(base_res.type_decl->kind != TypeKind::PTR)
        {
            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Member being accessed must be a pointer to a " 
                "container.\n";
            exit(1);
        }

        const PtrTypeDecl * base_ptr = 
            static_cast<const PtrTypeDecl*>(base_res.type_decl);

        if(base_ptr->builtin_type.has_value() || 
            base_ptr->pointee->kind != TypeKind::NAMED)
        {
            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Member being accessed must be a pointer to a " 
                "container.\n";
            exit(1);
        }

        typedecl_being_accessed = 
            static_cast<const NamedTypeDecl*>(base_ptr->pointee.get());
    }
    else
    {
        if(base_res.type_decl->kind != TypeKind::NAMED)
        {
            if(base_res.type_decl->kind == TypeKind::PTR)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << " -> Use \"->\" when accessing a pointer to a "
                    "container.\n";
                exit(1);
            }

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Member being accessed must be a container.\n";
            exit(1);
        }

        typedecl_being_accessed = 
            static_cast<const NamedTypeDecl*>(base_res.type_decl);
    }

    std::optional<const StructDecl*> decl_as_struct = 
        is_namedtype_struct(typedecl_being_accessed);

    // If the decl being accessed is a struct.
    if(decl_as_struct.has_value())
    {
        const StructDecl *struct_decl = *decl_as_struct;
    
        const FieldDecl *member = get_struct_member(struct_decl,
            ptr->member_name, ptr->line, ptr->col);

        CheckExprResult final_res;
        final_res.is_lvalue = true;
        final_res.is_var_and_mutable = member->is_binding_mutable;
        final_res.type_decl = member->type_decl.get();
        return final_res;
    }

    std::optional<const ComponentDecl*> decl_as_comp = 
        is_namedtype_component(typedecl_being_accessed);

    // If the decl being access is not a component (and it's not a struct)
    // it's not a container.
    if(!decl_as_comp.has_value())
    {
        print_error_location(ptr->line, ptr->col);
        std::cout << " -> Member being accessed must be a container.\n";
        exit(1);
    }

    // Member being accessed is a component.

    const ComponentDecl *comp_decl = *decl_as_comp;

    const FieldDecl *member = get_comp_member(comp_decl,
        ptr->member_name, ptr->line, ptr->col);

    if(!member->is_pub)
    {
        const FieldSymbol *sym = static_cast<const FieldSymbol*>(
            s_table->symbols.at(member->symbol_idx.value()).get());

        check_private_access(sym->scope_idx.value(), scope_id, ptr->line, 
            ptr->col, {ptr->member_name});
    }

    CheckExprResult final_res;

    final_res.is_lvalue = true;
    final_res.is_var_and_mutable = member->is_binding_mutable;
    final_res.type_decl = member->type_decl.get();

    return final_res;
}

TypeChecker::CheckExprResult TypeChecker::check_call_expr(const CallExpr *ptr,
    const uint64_t scope_id)
{
    const FunctionDecl *fn_decl = get_func_decl(ptr->callee_expr.get());

    

    std::cout << "Not implemented\n";
    exit(1);
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
            
            expr_result.is_lvalue = false;
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

            expr_result.is_lvalue = false;
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

            expr_result.is_lvalue = false;
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
            
            // const uint64_t builtin_float_id = 
            //     s_table->builtin_to_id.at("float");

            // new_decl->literal_data.emplace(
            //     LiteralData{{BuiltinType::FLOAT, BuiltinType::DOUBLE}});
            new_decl->literal_data.emplace(
                LiteralData{
                    {BuiltinType::FLOAT, BuiltinType::DOUBLE}});
            // new_decl->ident_path.push_back("float");
            // new_decl->resolved_symbol_idx.emplace(builtin_float_id);

            expr_result.is_lvalue = false;
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

            expr_result.is_lvalue = false;
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
            
            // const uint64_t builtin_char_id = 
            //     s_table->builtin_to_id.at("char");

            // new_decl->literal_data.emplace(LiteralData{{BuiltinType::CHAR}});
            new_decl->literal_data.emplace(
                LiteralData{{BuiltinType::CHAR}});
            // new_decl->ident_path.push_back("char");
            // new_decl->resolved_symbol_idx.emplace(builtin_char_id);

            expr_result.is_lvalue = false;
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
            
            new_decl->literal_data.emplace(
                LiteralData{{BuiltinType::BOOL}});
        
            expr_result.is_lvalue = false;
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

            expr_result.is_lvalue = false;
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

        case ExpressionType::TERNARY:
        {
            const TernaryExpr * const reint_expr = 
                static_cast<const TernaryExpr*>(ptr);
                
            CheckExprResult res = check_expression(reint_expr->condition.get(), 
                scope_id);

            if(res.type_decl->kind != TypeKind::NAMED)
            {
                print_error_location(ptr->line, ptr->col);
                std::cout << ": PRE Ternary condition must result in boolean "
                    "value.\n";
                exit(1);
            }

            const NamedTypeDecl *reint_decl = 
                static_cast<const NamedTypeDecl*>(res.type_decl);

            // If we are dealing with a literal.
            if(reint_decl->literal_data.has_value())
            {
                const LiteralData &data = *reint_decl->literal_data;

                bool found_bool = false;

                for(BuiltinType t : data.acceptable_builtin_ids)
                {
                    if(t == BuiltinType::BOOL)
                    {
                        found_bool = true;
                        break;
                    }
                }

                if(!found_bool)
                {
                    print_error_location(ptr->line, ptr->col);
                    std::cout << ": PRE Ternary condition must result in boolean "
                        "value.\n";
                    exit(1);
                }

                // The literal we are dealing with is a bool.
            }

            // We are not dealing with a literal.
            else
            {
                if(reint_decl->resolved_symbol_idx != 
                    s_table->builtin_to_id.at("bool"))
                {
                    print_error_location(ptr->line, ptr->col);
                    std::cout << ": Ternary condition must result in boolean "
                        "value.\n";
                    exit(1);
                }

                // The type is a bool, we're good.
            }

            CheckExprResult on_true_res = 
                check_expression(reint_expr->on_true_expr.get(), scope_id);
            CheckExprResult on_false_res = 
                check_expression(reint_expr->on_false_expr.get(), scope_id);

            if(!cmp_types(on_true_res.type_decl, on_false_res.type_decl))
            {
                print_error_location(reint_expr->on_true_expr->line, 
                    reint_expr->on_true_expr->col);
                std::cout << ": Condition result types do not match in ternary "
                    "statement\n";
                exit(1);
            }

            expr_result.is_lvalue = 
                on_true_res.is_lvalue && on_false_res.is_lvalue;
            expr_result.is_var_and_mutable = 
                on_true_res.is_var_and_mutable && 
                on_false_res.is_var_and_mutable;
            expr_result.type_decl = on_true_res.type_decl;

            break;
        }

        case ExpressionType::ASSIGN:
        {
            const AssignExpr * const reint_ptr = 
                static_cast<const AssignExpr*>(ptr);

            CheckExprResult lhs_res = check_expression(reint_ptr->lhs.get(), 
                scope_id);

            if(!lhs_res.is_lvalue || !lhs_res.is_var_and_mutable)
            {
                print_error_location(reint_ptr->lhs->line, reint_ptr->lhs->col);
                std::cout << ": Assignment requires a mutable lvalue.\n";
                exit(1);
            }

            CheckExprResult rhs_res = check_expression(reint_ptr->rhs.get(),
                scope_id);

            if(!cmp_types(lhs_res.type_decl, rhs_res.type_decl))
            {
                print_invalid_type_assignment(lhs_res.type_decl, 
                    rhs_res.type_decl, reint_ptr->rhs->line, 
                    reint_ptr->rhs->col);
                exit(1);
            }

            expr_result = lhs_res;
            break;
        }

        case ExpressionType::CALL:
        {
            expr_result = check_call_expr(static_cast<const CallExpr*>(ptr),
                scope_id);
            break;
        }

        case ExpressionType::CAST:

            expr_result = check_cast_expr(static_cast<const CastExpr*>(ptr),
                scope_id);
            break;

        case ExpressionType::SUBSCRIPT:
        {
            expr_result = check_subscript_expr(
                static_cast<const SubscriptExpr*>(ptr), scope_id);

            break;
        }

        case ExpressionType::MEMBER_ACCESS:
        {
            expr_result = check_memaccess_expr(
                static_cast<const MemberAccExpr*>(ptr), scope_id);

            break;
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid ExpressionType found\n";
            exit(1);
    }

    return expr_result;
}

// TypeChecker.cpp

#include "TypeChecker.hpp"

#include <iostream>
#include <charconv>


// Ctors / Dtor

TypeChecker::TypeChecker() {}

TypeChecker::~TypeChecker()
{
    for(TypeDecl *ptr : created_decls)
    {
        delete ptr;
    }
}


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
    std::cerr << "Serac ERROR: " << parsed_file << ":" << line << ':' << col;
}

void TypeChecker::print_type_mismatch(const TypeDecl *first, 
    const TypeDecl *second, uint32_t line, uint32_t col) const
{
    print_error_location(line, col);
    std::cerr << " -> Got type: \"";
    print_type_decl(second);
    std::cerr << "\" expected type: \"";
    print_type_decl(first);
    std::cerr << "\"\n";
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
                std::cerr << " -> Attempted to get resolved index of named type "
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
            std::cerr << " -> Invalid TypeKind for TypeDecl\n";
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
            std::cerr << " -> Receiver function declared outside of Component "
                "scope\n";
            exit(1);
        }

        if(get_resolved_idx_from_type_decl(ptr->receiver_data->type_decl.get()) 
            != enclosing_component.value()->symbol_idx)
        {
            print_error_location(ptr->receiver_data->type_decl->line,
                ptr->receiver_data->type_decl->col);
            std::cerr << " -> Component receiver object type does not match the "
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
            std::cerr << " -> This field declaration cannot have an "
                "initialization expression.\n";
            exit(1);
        }

        // This is a global variable with an init expression.
        check_expression(ptr->init_expr.get());
    }
}

void TypeChecker::check_param(Parameter * const ptr)
{
    std::cerr << "Checking param NOT IMPLEMENTED\n";

    (void)ptr;
}

void TypeChecker::check_var_decl(VarDeclStmt * const ptr)
{   
    std::cerr << "Checking Variable declaration for: " << ptr->var_name << '\n';
    const TypeDecl * const variable_type = 
        ptr->type_decl.get();

    if(ptr->init_expr != nullptr)
    {
        std::cerr << "Checking declaration init expression\n";

        CheckExprResult res = check_expression(ptr->init_expr.get());

        const TypeDecl * const init_expr_type = res.type_ptr;

        cmp_type_decls(variable_type, init_expr_type, ptr->init_expr->line,
            ptr->init_expr->col);
    }
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
                " -> Invalid declaration kind.\n";
            exit(1);
    }
}

void TypeChecker::check_statement(Statement * const ptr,
    std::optional<const TypeDecl*> expected_ret_type)
{
    std::cerr << "\nChecking statement of type: ";

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
                std::cerr << " -> Unexpected return statement.\n";
                exit(1);
            }

            const RetStmt * ret_stmt = static_cast<const RetStmt*>(ptr);


            const TypeDecl * func_ret_type = expected_ret_type.value();

            const uint64_t void_symbol_id = 
                    s_table->builtin_to_id.at("void");
            
            const bool return_type_is_void = 
                static_cast<const NamedTypeDecl*>(func_ret_type)->
                        resolved_symbol_idx.value() == void_symbol_id;  

            // We have no return expr, the return type better be void.
            if(ret_stmt->ret_expr == nullptr)
            {   
                // const uint64_t void_symbol_id = 
                //     s_table->builtin_to_id.at("void");

                // const TypeDecl * func_ret_type = expected_ret_type.value();

                if(func_ret_type->kind != TypeKind::NAMED || 
                    !return_type_is_void)
                {
                    print_error_location(ret_stmt->line, ret_stmt->col);
                    std::cerr << " -> No expression found in return statement for"
                        " function with non void return type.\n";
                    exit(1);
                }

                // Type is void, we're good.
                return;
            }

            // We have a return statement, return type better not be void
            if(return_type_is_void)
            {
                print_error_location(ptr->line, ptr->col);
                std::cerr << " -> Unexpected expression in return statement for"
                    " function with void return type.\n";
                exit(1);
            }

            CheckExprResult res = check_expression(
            ret_stmt->ret_expr.get());

            const TypeDecl * got_ret_type = res.type_ptr;

            cmp_type_decls(*expected_ret_type, got_ret_type, 
                ret_stmt->line, ret_stmt->col);
            
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
            std::cerr << " -> Invalid StatementType found\n";
            exit(1);
    }
}

void TypeChecker::print_type_decl(const TypeDecl* decl) const
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

        // case TypeKind::FUNC_PTR:
        // {

        //     break;
        // }

        case TypeKind::NAMED:
        {
            const NamedTypeDecl *reint_ptr = 
                static_cast<const NamedTypeDecl*>(decl); 

            std::cerr << reint_ptr->ident_path.back();
            break;
        }

        case TypeKind::PTR:
        {
            const PtrTypeDecl *reint_ptr = 
                static_cast<const PtrTypeDecl*>(decl);

            if(reint_ptr->builtin_type.has_value())
            {
                switch(*reint_ptr->builtin_type)
                {
                    case BuiltinPtrType::CSTR_PTR:

                        std::cerr << "cstr_ptr";
                        break;

                    case BuiltinPtrType::NULL_PTR:

                        std::cerr << "nullptr";
                        break;

                    case BuiltinPtrType::OPAQUE_PTR:

                        std::cerr << "opaque_ptr";
                        break;
                }

                return;
            } 

            // else 

            std::cerr << "*";
            if(reint_ptr->points_to_mutable) std::cerr << "mut";
            std::cerr << ' ';
            print_type_decl(reint_ptr->pointee.get());
            break;
        }

        case TypeKind::REF:
        {
            const RefTypeDecl * reint_ptr = 
                static_cast<const RefTypeDecl*>(decl);

            std::cerr << "ref";
            if(reint_ptr->ref_to_mutable) std::cerr << " mut";
            std::cerr << ' ';
            print_type_decl(reint_ptr->referred.get());
            break;
        }

        default:

            std::cerr << "Invalid type found for TypeDecl";
            break;
    }
}

TypeChecker::CheckExprResult TypeChecker::check_cast(
    const CheckExprResult &first, const CheckExprResult &second, 
    uint32_t expr_line, uint32_t expr_col) const
{
    std::cerr << "Checking cast between: ";
    print_type_decl(first.type_ptr);
    std::cerr << " and ";
    print_type_decl(second.type_ptr);
    std::cerr << '\n';

    std::cerr << "First is " <<
        (first.is_var_mutable ? "mutable\n" : "not mutable\n");
    std::cerr << "Second is " <<
        (second.is_var_mutable ? "mutable\n" : "not mutable\n");
    
    // Check if we're casting to * opaque. If so, all correctness checks are
    // off as we're in unsafe land.
    if(first.builtin_ptr_type.has_value() && 
        *first.builtin_ptr_type == BuiltinPtrType::OPAQUE_PTR)
    {
        std::cerr << "Casting to * opaque\n";
        return first;
    }

    // Check if we're casting from * opaque. Same for this as above.
    if(second.builtin_ptr_type.has_value() && 
        *second.builtin_ptr_type == BuiltinPtrType::OPAQUE_PTR)
    {
        std::cerr << "Casting from * opaque\n";
        return first;
    }

    // Not * opaque, need to check the types.
    cmp_type_decls(first.type_ptr, second.type_ptr, expr_line, expr_col);

    return first;
}

static bool check_acceptable_lit(const BuiltinType targ, 
    const std::vector<BuiltinType> &vec)
{
    for(BuiltinType t : vec)
    {
        if(t == targ) return true;
    }

    return false;
}

void TypeChecker::cmp_named_decls(const NamedTypeDecl *first, 
    const NamedTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const
{
    // Symbol index that the first NamedTypeDecl points to.
    const uint64_t first_sym_idx = 
        first->resolved_symbol_idx.value();

    // Our rhs is a literal type. 
    if(second->builtin_type.has_value())
    {
        std::cerr << "rhs is a literal\n";

        // Build a vector of acceptable type literals that are accepted as 
        // implicit conversion from the rhs to lhs. For instance, if the lhs was
        // an int, then the accepted types could be i8, u8, i16, u16 or i32, but
        // not u32, i64 and u64 as these are unrepresentable. 
        std::vector<BuiltinType> acceptable_types;

        if(first_sym_idx == s_table->builtin_to_id.at("u8"))
        {
            std::cerr << "lhs is an u8\n";
            acceptable_types.push_back(BuiltinType::U8);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i8"))
        {
            std::cerr << "lhs is an i8\n";
            // acceptable_types.push_back("i8");
            acceptable_types.push_back(BuiltinType::I8);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u16"))
        {
            // acceptable_types.push_back("u8");
            // acceptable_types.push_back("u16");
            acceptable_types.push_back(BuiltinType::U8);
            acceptable_types.push_back(BuiltinType::U16);
            std::cerr << "lhs is an u16\n";
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i16"))
        {
            std::cerr << "lhs is an i16\n";
            // acceptable_types.push_back("u8");
            // acceptable_types.push_back("i8");
            // acceptable_types.push_back("i16");
            acceptable_types.push_back(BuiltinType::U8);
            acceptable_types.push_back(BuiltinType::I8);
            acceptable_types.push_back(BuiltinType::I16);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u32"))
        {
            std::cerr << "lhs is an u32\n";
            // acceptable_types.push_back("u8");
            // acceptable_types.push_back("u16");
            // acceptable_types.push_back("u32");
            acceptable_types.push_back(BuiltinType::U8);
            acceptable_types.push_back(BuiltinType::U16);
            acceptable_types.push_back(BuiltinType::U32);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i32") || 
            first_sym_idx == s_table->builtin_to_id.at("int"))
        {
            std::cerr << "lhs is an i32\n";
            // acceptable_types.push_back("u8");
            // acceptable_types.push_back("i8");
            // acceptable_types.push_back("u16");
            // acceptable_types.push_back("i16");
            // acceptable_types.push_back("i32");
            acceptable_types.push_back(BuiltinType::U8);
            acceptable_types.push_back(BuiltinType::I8);
            acceptable_types.push_back(BuiltinType::U16);
            acceptable_types.push_back(BuiltinType::I16);
            acceptable_types.push_back(BuiltinType::I32);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("u64"))
        {
            std::cerr << "lhs is an u64\n";
            // acceptable_types.push_back("u8");
            // acceptable_types.push_back("u16");
            // acceptable_types.push_back("u32");
            // acceptable_types.push_back("u64");
            acceptable_types.push_back(BuiltinType::U8);
            acceptable_types.push_back(BuiltinType::U16);
            acceptable_types.push_back(BuiltinType::U32);
            acceptable_types.push_back(BuiltinType::U64);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("i64"))
        {
            std::cerr << "lhs is an i64\n";
            // acceptable_types.push_back("u8");
            // acceptable_types.push_back("i8");
            // acceptable_types.push_back("u16");
            // acceptable_types.push_back("i16");
            // acceptable_types.push_back("u32");
            // acceptable_types.push_back("i32");
            // acceptable_types.push_back("i64");
            acceptable_types.push_back(BuiltinType::U8);
            acceptable_types.push_back(BuiltinType::I8);
            acceptable_types.push_back(BuiltinType::U16);
            acceptable_types.push_back(BuiltinType::I16);
            acceptable_types.push_back(BuiltinType::I32);
            acceptable_types.push_back(BuiltinType::U32);
            acceptable_types.push_back(BuiltinType::I64);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("bool"))
        {
            std::cerr << "lhs is a bool\n";

            acceptable_types.push_back(BuiltinType::BOOL);
        }

        else if(first_sym_idx == s_table->builtin_to_id.at("char"))
        {
            std::cerr << "lhs is a char\n";
            // acceptable_types.push_back("u8");
            acceptable_types.push_back(BuiltinType::U8);
        }

        else
        {
            std::cerr << "Could not determine lhs\n";
        }
        
        std::cerr << "rhs is a: " << second->ident_path.at(0) << '\n';

        // The type of the lhs does not support assignment to the type of the 
        // rhs
        if(!check_acceptable_lit(second->builtin_type.value(), 
            acceptable_types))
        {
            print_error_location(expr_line, expr_col);
            std::cerr << " -> Illegal assignment of type: \"";
            print_type_decl(second);
            std::cerr << "\" to type: \"";
            print_type_decl(first);
            std::cerr << "\".\n";
            exit(1);
        }

        return;
    }

    // rhs is not a literal type.

    std::cerr << "rhs is not a literal type\n";

    const uint64_t second_sym_idx = 
        second->resolved_symbol_idx.value();

    // Make sure the types match.
    if(first_sym_idx != second_sym_idx)
    {
        print_type_mismatch(first, second, expr_line, expr_col);
        exit(0);
    }
}

void TypeChecker::cmp_ptr_decls(const PtrTypeDecl *first, 
    const PtrTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const
{
    // Check if rhs has a builtin ptr type (nullptr, opaque ptr or char ptr)
    if(second->builtin_type.has_value())
    {
        // If rhs is a nullptr, it can be assigned to any pointer.
        if(*second->builtin_type == BuiltinPtrType::NULL_PTR) return;

        // Other builtin types?
        std::cerr << "Implement this?\n";
        exit(1);
    }

    // *mut = * is not valid
    if(first->points_to_mutable && !second->points_to_mutable)
    {
        print_error_location(expr_line, expr_col);
        std::cerr << " -> Cannot assign pointer to non mutable: \"";
        print_type_decl(second);
        std::cerr << "\" to pointer to mutable: \"";
        print_type_decl(first);
        std::cerr << "\"\n";
        exit(1);
    }

    cmp_type_decls(first->pointee.get(), second->pointee.get(), expr_line, 
        expr_col);
}

void TypeChecker::cmp_type_decls(const TypeDecl *first, const TypeDecl *second,
    uint32_t expr_line, uint32_t expr_col) const
{
    std::cerr << "Comparing type decls: ";
    print_type_decl(first);
    std::cerr << " and ";
    print_type_decl(second);
    std::cerr << '\n';

    if(first->kind != second->kind)
    {
        print_type_mismatch(first, second, expr_line, expr_col);
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

        case TypeKind::NAMED:
        
            cmp_named_decls(static_cast<const NamedTypeDecl*>(first), 
                static_cast<const NamedTypeDecl*>(second), expr_line, expr_col);
            break;
        

        case TypeKind::PTR:

            cmp_ptr_decls(static_cast<const PtrTypeDecl*>(first),
                static_cast<const PtrTypeDecl*>(second), expr_line, expr_col);
            break;

        default:

            print_error_location(first->line, first->col);
            std::cerr << " ->  Invalid TypeKind found.\n";
            exit(1);
    }
}

bool TypeChecker::is_symbol_mutable(const Symbol *ptr, uint32_t symbol_line, 
    uint32_t symbol_col) const
{
    switch(ptr->sym_type)
    {
        case SymbolType::FIELD:
        {
            const FieldSymbol *reint_ptr = 
                static_cast<const FieldSymbol*>(ptr);

            return reint_ptr->ast_node_ptr->is_binding_mutable;
        }

        case SymbolType::INVALID:

            std::cerr << "TypeChecker::is_symbol_mutable() -> Invalid type "
                "found for symbol.\n";
            exit(1);

        case SymbolType::PARAM:
        {
            const ParamSymbol *reint_ptr = 
                static_cast<const ParamSymbol*>(ptr);

            return reint_ptr->ast_node_ptr->is_binding_mutable;
        }

        case SymbolType::VAR:
        {
            const VarSymbol *reint_ptr = 
                static_cast<const VarSymbol*>(ptr);

            return reint_ptr->ast_node_ptr->is_binding_mutable;
        }

        default: 
            print_error_location(symbol_line, symbol_col);
            std::cerr << " -> requested to determine if symbol was mutable but "
                " symbol has non applicable mutability type: " <<
                ptr->sym_type << '\n';
            exit(1);
    }
}

bool TypeChecker::is_var_mutable(const NamedTypeDecl *ptr) const
{
    // if(ptr->kind != TypeKind::NAMED)
    // {
    //     print_error_location(ptr->line, ptr->col);
    //     std::cerr << " -> TypeChecker::is_var_mutable() -> Cannot check variable"
    //         " mutability of a non variable.\n";
    //     exit(1);
    // }

    // const NamedTypeDecl * reint_ptr = 
    //     static_cast<const NamedTypeDecl*>(ptr);

    return is_symbol_mutable(s_table->symbols.at(
        ptr->resolved_symbol_idx.value()).get(), ptr->line, 
        ptr->col);
}

bool TypeChecker::is_ptr_opaque_ptr(const TypeDecl *ptr) const
{
    switch(ptr->kind)
    {
        case TypeKind::PTR:
            return is_ptr_opaque_ptr(
                static_cast<const PtrTypeDecl*>(ptr)->pointee.get()); 
        
        case TypeKind::NAMED:
            return static_cast<const NamedTypeDecl*>(ptr)->ident_path.back()
                == "opaque";

        default: return false;
    }
}

// Returns true if the number in the string can fit in type 'T'
template <typename T>
bool fits_in(const std::string& s, T& out) 
{
    std::from_chars_result res = 
        std::from_chars(s.data(), s.data() + s.size(), out);

    // auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);

    // return ec == std::errc() && ptr == s.data() + s.size();

    return res.ec == std::errc() && res.ptr == s.data() + s.size();
}

TypeChecker::CheckExprResult TypeChecker::check_addr_of_unary_expr(
    const UnaryExpr * const ptr)
{
    CheckExprResult operand_res = check_expression(ptr->operand.get());
    
    if(!operand_res.is_lvalue)
    {
        print_error_location(ptr->line, ptr->col);
        std::cerr << " -> Cannot take address of a non lvalue\n";
        exit(1);
    }
    
    PtrTypeDecl * new_ptr = new PtrTypeDecl;
    created_decls.push_back(new_ptr);

    new_ptr->line = ptr->line;
    new_ptr->col = ptr->col;
    new_ptr->points_to_mutable = operand_res.is_var_mutable;

    // Our new PtrTypeDecl that we've had to create needs a unique ptr to the 
    // TypeDecl it's pointing to. Since it's a unique ptr and we only have a raw
    // pointer, we need to clone it and keep our own copy of the data.
    new_ptr->pointee = operand_res.type_ptr->clone();

    CheckExprResult final_res;
    final_res.type_ptr = new_ptr;
    final_res.is_var_mutable = operand_res.is_var_mutable;
    final_res.is_lvalue = false; // Temporary ptr to lvalue is not an lvalue.

    return final_res;
}

TypeChecker::CheckExprResult TypeChecker::check_unary_expr(
    const UnaryExpr * const ptr)
{
    std::cerr << "Checking Unary Expr for: " << parsed_file << 
        ":" << ptr->line << ":" << ptr->col << '\n';

    switch(ptr->op_type)
    {
        case UnaryOp::ADDRESS_OF: return check_addr_of_unary_expr(ptr);

        // case UnaryOp::BIT_NOT:

        //     break;

        // case UnaryOp::DEREF:

        //     break;

        // case UnaryOp::LOG_NOT:

        //     break;

        // case UnaryOp::NEGATE:

        //     break;

        // case UnaryOp::POST_DEC:

        //     break;

        // case UnaryOp::POST_INC:

        //     break;

        // case UnaryOp::PRE_DEC:

        //     break;

        // case UnaryOp::PRE_INC:

        //     break;

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid UnaryOp type found.\n";
            exit(1);
    }

    return CheckExprResult {};
}

TypeChecker::CheckExprResult TypeChecker::check_ident_expr(
    const IdentExpr * const ptr)
{
    CheckExprResult res;
    res.is_lvalue = true;

    std::cerr << "Checking Ident Expr for: ";

    for(size_t i = 0; i < ptr->ident_path.size(); ++i)
    {
        std::cerr << ptr->ident_path.at(i);
        if(i < ptr->ident_path.size() - 1) std::cerr << "::";
    }
    std::cerr << '\n';

    // Get the symbol that this IdentExpr points to.
    const Symbol * const sym_ptr = s_table->symbols.at(
        ptr->resolved_symbol_idx.value()).get();
        
    std::cerr << "Ident symbol type is: " << sym_ptr->sym_type << '\n';

    switch(sym_ptr->sym_type)
    {
        case SymbolType::VAR:
        {
            // Get the variable declaration statement of the symbol
            const VarDeclStmt * const var_stmt = 
                static_cast<const VarSymbol*>(sym_ptr)->ast_node_ptr;

            res.type_ptr = var_stmt->type_decl.get();
            res.is_var_mutable = var_stmt->is_binding_mutable;

            return res;
        }

        case SymbolType::PARAM:
        {
            // Get the Parameter object from the symbol.
            const Parameter * param = 
                static_cast<const ParamSymbol*>(sym_ptr)->ast_node_ptr;
            
            res.type_ptr = param->type_decl.get();
            res.is_var_mutable = param->is_binding_mutable;

            return res;
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> TypeChecker::check_ident_expr() -> Invalid type "
                "found for Symbol.\n";
            exit(1);
    }
}

TypeChecker::CheckExprResult TypeChecker::check_expression(
    const Expression * const ptr)
{
    std::cerr << "Checking expression of type: ";
    std::cerr << ptr->exp_type << '\n';

    switch(ptr->exp_type)
    {
        // case ExpressionType::ARR_INIT:

        //     break;

        case ExpressionType::ASSIGN:
        {
            const AssignExpr * const reint_ptr = 
                static_cast<const AssignExpr*>(ptr);

            const CheckExprResult lhs_res = 
                check_expression(reint_ptr->lhs.get());
                
            if(!lhs_res.is_var_mutable)
            {
                print_error_location(reint_ptr->line, reint_ptr->col);
                std::cerr << " -> Cannot assign an immutable variable.\n";
                exit(1);
            }

            const CheckExprResult rhs_res = 
                check_expression(reint_ptr->rhs.get());

            // Make sure the rhs matches the type of the lhs
            cmp_type_decls(lhs_res.type_ptr, rhs_res.type_ptr, 
                reint_ptr->rhs->line, reint_ptr->rhs->col);

            return lhs_res;
        }

        // case ExpressionType::BIN_LITERAL:
        // {
        //     break;
        // }

        // case ExpressionType::BINARY:
        // {
        //     break;
        // }

        case ExpressionType::BOOL_LITERAL:
        {
            // const BoolLitExpr *reint_ptr = 
            //     static_cast<const BoolLitExpr*>(ptr);

            NamedTypeDecl *type_decl = new NamedTypeDecl();

            CheckExprResult res;
            res.type_ptr = type_decl;
            res.is_var_mutable = false; 

            type_decl->builtin_type.emplace(BuiltinType::BOOL);
            type_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("bool"));
            type_decl->ident_path.push_back("bool");
            return res;
        }

        // case ExpressionType::CALL:
        // {
        //     break;
        // }

        case ExpressionType::CAST:
        {
            const CastExpr * reint_ptr = 
                static_cast<const CastExpr*>(ptr);
            
            CheckExprResult lhs_res;
            lhs_res.type_ptr = reint_ptr->to_cast_type.get();
            lhs_res.is_lvalue = false;
            lhs_res.is_var_mutable = false;

            if(is_ptr_opaque_ptr(lhs_res.type_ptr))
            {
                lhs_res.builtin_ptr_type.emplace(BuiltinPtrType::OPAQUE_PTR);
            }

            CheckExprResult rhs_res = 
                check_expression(reint_ptr->expr_to_cast.get());

            return check_cast(
                lhs_res, rhs_res, reint_ptr->line, reint_ptr->col);
        }

        case ExpressionType::CHAR_LITERAL:
        {
            // const CharLitExpr * const reint_ptr = 
            //     static_cast<const CharLitExpr*>(ptr);
            
            CheckExprResult res;

            NamedTypeDecl *type_decl = new NamedTypeDecl();
            // TODO: Implement some way of deleting this memory.

            res.type_ptr = type_decl;
            res.is_var_mutable = false;

            type_decl->builtin_type.emplace(BuiltinType::U8);
            type_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("u8"));
            type_decl->ident_path.push_back("u8");
            return res;
        }

        // case ExpressionType::FLOAT_LITERAL:
        // {
        //     break;
        // }

        // case ExpressionType::HEX_LITERAL:
        // {
        //     break;
        // }

        case ExpressionType::IDENTIFIER:
        
            return check_ident_expr(static_cast<const IdentExpr*>(ptr));
        

        case ExpressionType::INT_LITERAL:
        {
            const IntLitExpr * const reint_ptr = 
                static_cast<const IntLitExpr*>(ptr);

            NamedTypeDecl *type_decl = new NamedTypeDecl();
            // TODO: Implement some way of deleting this memory.

            CheckExprResult res;

            res.is_var_mutable = false;
            res.type_ptr = type_decl;

            // type_decl->is_literal = true;

            uint8_t valu8;
            if(fits_in(reint_ptr->value, valu8))
            {
                // std::cerr << "Num fits in u8\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u8"));
                type_decl->builtin_type.emplace(BuiltinType::U8);
                type_decl->ident_path.push_back("u8");
                return res;
            }

            int8_t vali8;
            if(fits_in(reint_ptr->value, vali8))
            {
                // std::cerr << "Num fits in i8\n";   
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i8"));
                type_decl->builtin_type.emplace(BuiltinType::I8);
                type_decl->ident_path.push_back("i8");
                return res;
            }

            uint16_t valu16;
            if(fits_in(reint_ptr->value, valu16))
            {
                // std::cerr << "Num fits in u16\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u16"));
                type_decl->builtin_type.emplace(BuiltinType::U16);
                type_decl->ident_path.push_back("u16");
                return res;
            }

            int16_t vali16;
            if(fits_in(reint_ptr->value, vali16))
            {
                // std::cerr << "Num fits in i16\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i16"));
                type_decl->builtin_type.emplace(BuiltinType::I16);
                type_decl->ident_path.push_back("i16");
                return res;
            }

            uint32_t valu32;
            if(fits_in(reint_ptr->value, valu32))
            {
                // std::cerr << "Num fits in u32\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u32"));
                type_decl->builtin_type.emplace(BuiltinType::U32);
                type_decl->ident_path.push_back("u32");
                return res;
            }

            int valint;
            if(fits_in(reint_ptr->value, valint))
            {
                // std::cerr << "Num fits in int\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i32"));
                type_decl->builtin_type.emplace(BuiltinType::I32);
                type_decl->ident_path.push_back("i32");
                return res;
            }

            uint64_t valu64;
            if(fits_in(reint_ptr->value, valu64))
            {
                // std::cerr << "Num fits in u64\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u64"));
                type_decl->builtin_type.emplace(BuiltinType::U64);
                type_decl->ident_path.push_back("u64");
                return res;
            }

            int64_t vali64;
            if(fits_in(reint_ptr->value, vali64))
            {
                // std::cerr << "Num fits in i64\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i64"));
                type_decl->builtin_type.emplace(BuiltinType::I64);
                type_decl->ident_path.push_back("i64");
                return res;
            }

            std::cerr << "Could not convert text to INT_LITERAL: "
                << reint_ptr->value << '\n';

            exit(1);
        }

        // case ExpressionType::MEMBER_ACCESS:
        // {
        //     break;
        // }

        case ExpressionType::NULLPTR_LITERAL:
        {
            PtrTypeDecl *type_decl = new PtrTypeDecl();
            // TODO: Implement some way of deleting this memory.

            type_decl->builtin_type.emplace(BuiltinPtrType::NULL_PTR);

            CheckExprResult res;
            res.type_ptr = type_decl;
            res.is_var_mutable = false;

            return res;
        }

        case ExpressionType::STR_LITERAL:
        {
            PtrTypeDecl *type_decl = new PtrTypeDecl();
            // TODO: Implement some way of deleting this memory.
            
            type_decl->builtin_type.emplace(BuiltinPtrType::CSTR_PTR);

            CheckExprResult res;
            res.type_ptr = type_decl;
            res.is_var_mutable = false;

            return res;
        }

        // case ExpressionType::STRUCT_INIT:
        // {
        //     break;
        // }

        // case ExpressionType::SUBSCRIPT:
        // {
        //     break;
        // }

        // case ExpressionType::TERNARY:
        // {
        //     break;
        // }

        case ExpressionType::UNARY:
        {
            const UnaryExpr * reint_ptr = 
                static_cast<const UnaryExpr*>(ptr);

            return check_unary_expr(reint_ptr);
        }

        default: 
        
            print_error_location(ptr->line, ptr->col);
            std::cerr << " -> Invalid type found for expression\n";
            exit(1);
    }

    return CheckExprResult {};
}

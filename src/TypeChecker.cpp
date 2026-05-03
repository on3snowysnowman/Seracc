// TypeChecker.cpp

#include "TypeChecker.hpp"

#include <iostream>
#include <charconv>


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

void TypeChecker::print_type_mismatch(const TypeDecl *first, 
    const TypeDecl *second, uint32_t line, uint32_t col) const
{
    print_error_location(line, col);
    std::cerr << ": Got type: \"";
    print_type_decl(first);
    std::cerr << "\" expected type: \"";
    print_type_decl(second);
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

        const TypeDecl * const init_expr_type = 
            check_expression(ptr->init_expr.get());

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
                ": Invalid declaration kind.\n";
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
                std::cerr << ": Unexpected return statement.\n";
                exit(1);
            }

            const RetStmt * ret_stmt_expr = static_cast<const RetStmt*>(ptr);

            const TypeDecl * got_ret_type = 
                check_expression(ret_stmt_expr->ret_expr.get());


            cmp_type_decls(*expected_ret_type, got_ret_type, 
                ret_stmt_expr->line, ret_stmt_expr->col);
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

        case TypeKind::FUNC_PTR:
        {

            break;
        }

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

            if(reint_ptr->pointee == nullptr)
            {
                switch(reint_ptr->builtin_type.value())
                {
                    case BuiltinPtrType::CSTR_PTR:

                        std::cerr << "cstr_ptr";
                        break;

                    case BuiltinPtrType::NULLPTR:

                        std::cerr << "nullptr";
                        break;
                }

                return;
            }

            std::cerr << "*";
            print_type_decl(reint_ptr->pointee.get());
            break;
        }

        case TypeKind::REF:
        {
            const RefTypeDecl * reint_ptr = 
                static_cast<const RefTypeDecl*>(decl);

            std::cerr << "ref ";
            print_type_decl(reint_ptr->referred.get());
            break;
        }

        default:

            std::cerr << "Invalid type found for TypeDecl";
            break;
    }
}

void TypeChecker::check_cast(const CheckExprResult &first, 
    const CheckExprResult &second, uint32_t expr_line, uint32_t expr_col) 
    const
{
    std::cerr << "Checking cast between: ";
    print_type_decl(first.type_ptr);
    std::cerr << " and ";
    print_type_decl(second.type_ptr);
    std::cerr << '\n';

    std::cerr << "First is " <<
        first.is_type_mutable ? "mutable\n" : "not mutable\n";
    std::cerr << "Second is " <<
        second.is_type_mutable ? "mutable\n" : "not mutable\n";

    
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
            std::cerr << ": Illegal assignment of type: \"";
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

}

void TypeChecker::cmp_type_decls(const TypeDecl *first, const TypeDecl *second,
    uint32_t expr_line, uint32_t expr_col)
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

        // case TypeKind::FUNC_PTR:
        // {


        //     break;
        // }

        case TypeKind::INVALID:
        {
            break;
        }

        case TypeKind::NAMED:
        {
            cmp_named_decls(static_cast<const NamedTypeDecl*>(first), 
                static_cast<const NamedTypeDecl*>(second), expr_line, expr_col);
            break;
        }

        // case TypeKind::PTR:

        //     break;

        // case TypeKind::REF:

        //     break;

        default:

            print_error_location(first->line, first->col);
            std::cerr << ": Invalid TypeKind found.\n";
            exit(1);
    }
}

// Returns true if the number in the string can fit in type 'T'
template <typename T>
bool fits_in(const std::string& s, T& out) 
{
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);

    return ec == std::errc() && ptr == s.data() + s.size();
}

const TypeDecl* TypeChecker::check_ident_expr(const IdentExpr * const ptr)
{
    std::cerr << "Checking Ident Expr\n";

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

            return var_stmt->type_decl.get();
        }

        case SymbolType::PARAM:
        {
            // Get the Parameter object from the symbol.
            const Parameter * param = 
                static_cast<const ParamSymbol*>(sym_ptr)->ast_node_ptr;
            
            return param->type_decl.get();
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cerr << ": TypeChecker::check_ident_expr() -> Invalid type "
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
            const CheckExprResult rhs_res = 
                check_expression(reint_ptr->rhs.get());

            // const TypeDecl * lhs_type = 
            //     check_expression(reint_ptr->lhs.get());
            // const TypeDecl * rhs_type = 
            //     check_expression(reint_ptr->rhs.get());

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
            const BoolLitExpr *reint_ptr = 
                static_cast<const BoolLitExpr*>(ptr);

            NamedTypeDecl *type_decl = new NamedTypeDecl();

            CheckExprResult res;
            res.type_ptr = type_decl;
            res.is_type_mutable = true; // Literals bools are mutable.

            type_decl->builtin_type.emplace(BuiltinType::BOOL);
            type_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("bool"));
            type_decl->ident_path.push_back("bool");
            return type_decl;
        }

        // case ExpressionType::CALL:
        // {
        //     break;
        // }

        case ExpressionType::CAST:
        {
            const CastExpr *reint_ptr = static_cast<const CastExpr*>(ptr);
            
            cmp_type_decls(reint_ptr->to_cast_type.get(), 
                check_expression(reint_ptr->expr_to_cast.get()), 
                reint_ptr->expr_to_cast->line, reint_ptr->expr_to_cast->col);
            return reint_ptr->to_cast_type.get();
        }

        case ExpressionType::CHAR_LITERAL:
        {
            const CharLitExpr * const reint_ptr = 
                static_cast<const CharLitExpr*>(ptr);

            NamedTypeDecl *type_decl = new NamedTypeDecl();
            // TODO: Implement some way of deleting this memory.

            type_decl->builtin_type.emplace(BuiltinType::U8);
            type_decl->resolved_symbol_idx.emplace(
                s_table->builtin_to_id.at("u8"));
            type_decl->ident_path.push_back("u8");
            return type_decl;
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
        {
            const IdentExpr * reint_ptr = 
                static_cast<const IdentExpr*>(ptr);

            return check_ident_expr(reint_ptr);
        }

        case ExpressionType::INT_LITERAL:
        {
            const IntLitExpr * const reint_ptr = 
                static_cast<const IntLitExpr*>(ptr);

            NamedTypeDecl *type_decl = new NamedTypeDecl();
            // TODO: Implement some way of deleting this memory.

            // type_decl->is_literal = true;

            uint8_t valu8;
            if(fits_in(reint_ptr->value, valu8))
            {
                // std::cerr << "Num fits in u8\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u8"));
                type_decl->builtin_type.emplace(BuiltinType::U8);
                type_decl->ident_path.push_back("u8");
                return type_decl;
            }

            int8_t vali8;
            if(fits_in(reint_ptr->value, vali8))
            {
                // std::cerr << "Num fits in i8\n";   
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i8"));
                type_decl->builtin_type.emplace(BuiltinType::I8);
                type_decl->ident_path.push_back("i8");
                return type_decl;
            }

            uint16_t valu16;
            if(fits_in(reint_ptr->value, valu16))
            {
                // std::cerr << "Num fits in u16\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u16"));
                type_decl->builtin_type.emplace(BuiltinType::U16);
                type_decl->ident_path.push_back("u16");
                return type_decl;
            }

            int16_t vali16;
            if(fits_in(reint_ptr->value, vali16))
            {
                // std::cerr << "Num fits in i16\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i16"));
                type_decl->builtin_type.emplace(BuiltinType::I16);
                type_decl->ident_path.push_back("i16");
                return type_decl;
            }

            uint32_t valu32;
            if(fits_in(reint_ptr->value, valu32))
            {
                // std::cerr << "Num fits in u32\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u32"));
                type_decl->builtin_type.emplace(BuiltinType::U32);
                type_decl->ident_path.push_back("u32");
                return type_decl;
            }

            int valint;
            if(fits_in(reint_ptr->value, valint))
            {
                // std::cerr << "Num fits in int\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i32"));
                type_decl->builtin_type.emplace(BuiltinType::I32);
                type_decl->ident_path.push_back("i32");
                return type_decl;
            }

            uint64_t valu64;
            if(fits_in(reint_ptr->value, valu64))
            {
                // std::cerr << "Num fits in u64\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("u64"));
                type_decl->builtin_type.emplace(BuiltinType::U64);
                type_decl->ident_path.push_back("u64");
                return type_decl;
            }

            int64_t vali64;
            if(fits_in(reint_ptr->value, vali64))
            {
                // std::cerr << "Num fits in i64\n";
                type_decl->resolved_symbol_idx.emplace(
                    s_table->builtin_to_id.at("i64"));
                type_decl->builtin_type.emplace(BuiltinType::I64);
                type_decl->ident_path.push_back("i64");
                return type_decl;
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

            type_decl->builtin_type.emplace(BuiltinPtrType::NULLPTR);

            return type_decl;
        }

        case ExpressionType::STR_LITERAL:
        {
            PtrTypeDecl *type_decl = new PtrTypeDecl();
            // TODO: Implement some way of deleting this memory.
            
            type_decl->builtin_type.emplace(BuiltinPtrType::CSTR_PTR);

            return type_decl;
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

        // case ExpressionType::UNARY:
        // {
        //     break;
        // }

        default: 
        
            print_error_location(ptr->line, ptr->col);
            std::cerr << ": Invalid type found for expression\n";
            exit(1);
    }

    return nullptr;
}

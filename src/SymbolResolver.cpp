// SymbolResolver.cpp

#include "SymbolResolver.hpp"

#include <iostream>


// Ctors / Dtors

SymbolResolver::SymbolResolver() {}


// Public

void SymbolResolver::resolve(Program &p, const SymbolTable &s_table)
{
    this->s_table = &s_table;
    parsed_file = p.source_file_name;
       
    resolve_module(p.ast.get());

    this->s_table = nullptr;
    parsed_file = nullptr;
}


// Private

void SymbolResolver::print_error_location(uint32_t line, uint32_t col) const
{
    std::cout << parsed_file << ':' << line << ':' << col;
}

void SymbolResolver::resolve_module(ModuleDecl * const ptr)
{
    uint64_t scope_idx = static_cast<ModuleSymbol*>(s_table->symbols.at(
        ptr->symbol_idx.value()).get())->scope_idx.value();

    for(const std::unique_ptr<Declaration> &ptr: ptr->decls)
    {
        switch(ptr->kind)
        {
            case DeclKind::FIELD:

                resolve_field(static_cast<FieldDecl*>(ptr.get()));
                break;

            case DeclKind::MODULE:

                resolve_module(static_cast<ModuleDecl*>(ptr.get()));
                break;
            
            case DeclKind::STRUCT:

                resolve_struct(static_cast<StructDecl*>(ptr.get()));
                break;

            case DeclKind::FUNCTION:

                resolve_function(static_cast<FunctionDecl*>(ptr.get()), 
                    scope_idx);
                break;

            case DeclKind::COMPONENT:

                resolve_component(static_cast<ComponentDecl*>(ptr.get()));
                break;

            default:

                std::cout << "Invalid declaration type found for declaration."
                    "\n";
                exit(1); 
        }
    }
}

void SymbolResolver::resolve_field(FieldDecl * const ptr)
{
    Symbol * const symbol_ptr = 
        s_table->symbols.at(ptr->symbol_idx.value()).get();

    resolve_type_decl(ptr->type_decl.get(), symbol_ptr->scope_idx.value());
}

void SymbolResolver::resolve_struct(StructDecl * const ptr)
{
    uint64_t scope_idx = static_cast<StructSymbol*>(s_table->symbols.at(
        ptr->symbol_idx.value()).get())->scope_idx.value();

    for(const auto &decl: ptr->decls)
        resolve_declaration(decl.get(), scope_idx);
}

void SymbolResolver::resolve_function(FunctionDecl * const ptr, 
    uint64_t scope_idx)
{
    for(Parameter &p: ptr->params)
        resolve_type_decl(p.type_decl.get(), scope_idx);

    if(ptr->receiver_data.has_value())
        resolve_type_decl(ptr->receiver_data->type_decl.get(), scope_idx);

    resolve_type_decl(ptr->ret_type.get(), scope_idx);

    resolve_block(&ptr->body);
}

void SymbolResolver::resolve_component(ComponentDecl * const ptr)
{
    uint64_t scope_idx = static_cast<ComponentSymbol*>(s_table->symbols.at(
        ptr->symbol_idx.value()).get())->scope_idx.value();

    for(const auto &decl: ptr->decls)
        resolve_declaration(decl.get(), scope_idx);
}

void SymbolResolver::resolve_type_decl(TypeDecl * const ptr, 
    uint64_t scope_idx)
{
    switch(ptr->kind)
    {
        case TypeKind::NAMED:
        {
            NamedTypeDecl * const reint_ptr = 
                static_cast<NamedTypeDecl*>(ptr);

            reint_ptr->resolved_symbol_idx = 
                find_symbol_idx(reint_ptr->ident_path, scope_idx, 
                    ptr->line, ptr->col);

            break;
        }

        case TypeKind::PTR:
        {
            PtrTypeDecl *reint_ptr = static_cast<PtrTypeDecl*>(ptr);

            // We do not need to resolve opaque_ptr types.
            if(reint_ptr->builtin_type.has_value() && 
                *reint_ptr->builtin_type == BuiltinPtrType::OPAQUE_PTR) break;

            resolve_type_decl(static_cast<PtrTypeDecl*>(ptr)->pointee.get(),
                scope_idx);
            break;
        }

        case TypeKind::REF:

            resolve_type_decl(static_cast<RefTypeDecl*>(ptr)->referred.get(),
                scope_idx);
            break;

        case TypeKind::ARRAY:

            resolve_type_decl(
                static_cast<ArrTypeDecl*>(ptr)->element_type.get(), scope_idx);            
            break;

        // case TypeKind::FUNC_PTR:

        //     break;

        default:

            std::cout << "SymbolResolver::resolve_type_decl() -> Invalid "
                "TypeKind for type declaration: "
                << parsed_file << ':' << ptr->line << ':' << ptr->col << "\n";
            exit(1);
            break;
    }
}

void SymbolResolver::resolve_declaration(Declaration * const ptr, 
    uint64_t scope_idx)
{
    switch(ptr->kind)
    {
        case DeclKind::COMPONENT:

            resolve_component(static_cast<ComponentDecl*>(ptr));
            break;

        case DeclKind::FIELD:

            resolve_field(static_cast<FieldDecl*>(ptr));
            break;

        case DeclKind::FUNCTION:

            resolve_function(static_cast<FunctionDecl*>(ptr), scope_idx);
            break;

        case DeclKind::MODULE:

            resolve_module(static_cast<ModuleDecl*>(ptr));
            break;

        case DeclKind::STRUCT:

            resolve_struct(static_cast<StructDecl*>(ptr));
            break;

        default:

            std::cout << parsed_file << ":" << ptr->line << ":" << ptr->col << 
                " -> Invalid type for declaration.\n";
            exit(1);
            break;
    }
}

void SymbolResolver::resolve_block(ScopeBody * const ptr)
{
    for(const auto &stmt: ptr->statements)
        resolve_statement(stmt.get(), ptr->scope_idx);
}

void SymbolResolver::resolve_statement(Statement * const ptr, 
    uint64_t scope_idx) 
{
    switch(ptr->stmt_type)
    {
        case StatementType::BLOCK:
        
            resolve_block(&static_cast<BlockStmt*>(ptr)->block_decl);
            break;

        case StatementType::COMPONENT_DECL:

            resolve_component(static_cast<ComponentDeclStmt*>(ptr)->decl.get());
            break;

        case StatementType::EXPR:

            resolve_expression(static_cast<ExprStmt*>(ptr)->expr.get(), 
                scope_idx);
            break;

        case StatementType::FOR:
        {
            ForStmt * const reint_ptr = 
                static_cast<ForStmt*>(ptr);

            resolve_statement(reint_ptr->init_stmt.get(), scope_idx);
            resolve_expression(reint_ptr->condition_expr.get(), scope_idx);
            resolve_expression(reint_ptr->increment_expr.get(), scope_idx);

            resolve_block(&reint_ptr->body);
            break;
        }

        case StatementType::IF:
        {
            IfStmt * const reint_ptr = 
                static_cast<IfStmt*>(ptr);

            resolve_expression(reint_ptr->condition_expr.get(), scope_idx);
            resolve_block(&reint_ptr->then_body);
            
            if(reint_ptr->else_branch != nullptr)
                resolve_statement(reint_ptr->else_branch.get(), scope_idx);
            break;
        }


        case StatementType::RETURN:
        {
            RetStmt * reint_ptr = static_cast<RetStmt*>(ptr);

            // No return expression.
            if(reint_ptr->ret_expr == nullptr) break; 

            resolve_expression(reint_ptr->ret_expr.get(), scope_idx);
            break;
        }

        case StatementType::STRUCT_DECL:

            resolve_struct(static_cast<StructDeclStmt*>(ptr)->decl.get());
            break;

        case StatementType::VAR_DECL:
        {
            VarDeclStmt * const reint_ptr = 
                static_cast<VarDeclStmt*>(ptr);

            resolve_type_decl(reint_ptr->type_decl.get(), scope_idx);

            if(reint_ptr->init_expr != nullptr)
                resolve_expression(reint_ptr->init_expr.get(), scope_idx);
                
            break;
        }

        case StatementType::WHILE:
        {
            WhileStmt * const reint_ptr =   
                static_cast<WhileStmt*>(ptr);

            resolve_expression(reint_ptr->condition_expr.get(), scope_idx);
            resolve_block(&reint_ptr->body);
            break;
        }

        default:

            std::cout << parsed_file << ":" << ptr->line << ":" << ptr->col <<
                " -> Invalid statement type.\n";
            exit(1);
            break;
    }
}

// void SymbolResolver::recurse_resolve_arr_init_expr(
//     ArrInitExpr * const ptr)
// {
//     for(const std::unique_ptr<Expression> &init_arg : ptr->init_args)
//     {
//         if(init_arg->exp_type == ExpressionType::ARR_INIT)
//         {
//             recurse_resolve_arr_init_expr(
//                 static_cast<ArrInitExpr*>(init_arg.get()));
//         }
//         else
//         {
//             resolve_expression()
//         }
//     }
// }

void SymbolResolver::resolve_expression(Expression * const ptr,
    uint64_t scope_idx) 
{
    switch(ptr->exp_type)
    {
        case ExpressionType::ARR_INIT:
        {
            ArrInitExpr * const reint_ptr = 
                static_cast<ArrInitExpr*>(ptr);
                
            for(const std::unique_ptr<Expression> &init_arg : 
                reint_ptr->init_args)
            {
                resolve_expression(init_arg.get(), scope_idx);
            }
            break;
        }

        case ExpressionType::ASSIGN:
        {
            AssignExpr * const reint_ptr = 
                static_cast<AssignExpr*>(ptr);

            resolve_expression(reint_ptr->lhs.get(), scope_idx);
            resolve_expression(reint_ptr->rhs.get(), scope_idx);
            break;
        }

        case ExpressionType::BINARY:
        {
            BinaryExpr * const reint_ptr = 
                static_cast<BinaryExpr*>(ptr);

            resolve_expression(reint_ptr->lhs.get(), scope_idx);
            resolve_expression(reint_ptr->rhs.get(), scope_idx);
            break;
        }

        case ExpressionType::CALL:
        {
            CallExpr * const reint_ptr = 
                static_cast<CallExpr*>(ptr);

            resolve_expression(reint_ptr->callee_expr.get(), scope_idx);

            for(const std::unique_ptr<Expression> &arg : reint_ptr->args)
            {
                resolve_expression(arg.get(), scope_idx);
            }

            break;
        }

        case ExpressionType::CAST:
        {
            CastExpr * const reint_ptr =
                static_cast<CastExpr*>(ptr);

            resolve_expression(reint_ptr->expr_to_cast.get(), scope_idx);
            resolve_type_decl(reint_ptr->to_cast_type.get(), scope_idx);
            break;
        }

        case ExpressionType::IDENTIFIER:
        {
            IdentExpr * const reint_ptr = 
                static_cast<IdentExpr*>(ptr);

            reint_ptr->resolved_symbol_idx = 
                find_symbol_idx(reint_ptr->ident_path, scope_idx, 
                    reint_ptr->line, reint_ptr->col);
            break;
        }

        case ExpressionType::MEMBER_ACCESS:
        {
            MemberAccExpr * const reint_ptr = 
                static_cast<MemberAccExpr*>(ptr);

            resolve_expression(reint_ptr->base_expr.get(), scope_idx);
            break;
        }     

        case ExpressionType::STRUCT_CREATE:
        {
            StructCreateExpr * const reint_ptr = 
                static_cast<StructCreateExpr*>(ptr);

            resolve_expression(reint_ptr->lhs.get(), scope_idx);
            resolve_expression(reint_ptr->create_expr.get(), scope_idx);
            break;
        }

        case ExpressionType::STRUCT_INIT:
        {
            StructInitExpr * const reint_ptr = 
                static_cast<StructInitExpr*>(ptr);

            for(auto &elem : reint_ptr->init_args)
            {
                resolve_expression(elem.second.get(), scope_idx);
            }
            break;
        }   

        case ExpressionType::SUBSCRIPT:
        {
            SubscriptExpr * const reint_ptr = 
                static_cast<SubscriptExpr*>(ptr);

            resolve_expression(reint_ptr->base_expr.get(), scope_idx);
            resolve_expression(reint_ptr->index_expr.get(), scope_idx);

            break;
        }

        case ExpressionType::TERNARY:
        {   
            TernaryExpr * const reint_ptr = 
                static_cast<TernaryExpr*>(ptr);

            resolve_expression(reint_ptr->condition.get(), scope_idx);
            resolve_expression(reint_ptr->on_true_expr.get(), scope_idx);
            resolve_expression(reint_ptr->on_false_expr.get(), scope_idx);

            break;
        }
       
        case ExpressionType::UNARY:
        {
            UnaryExpr * const reint_ptr = 
                static_cast<UnaryExpr*>(ptr);

            resolve_expression(reint_ptr->operand.get(), scope_idx);

            break;
        }        

        default: break;
    }
}

std::pair<uint32_t, uint32_t> SymbolResolver::get_symbol_pos(
    const Symbol *ptr) const
{
    switch(ptr->sym_type)
    {
        case SymbolType::BUILTIN:

            return {0, 0};

        case SymbolType::STRUCT:
        {
            const StructSymbol *reint_ptr = 
                static_cast<const StructSymbol*>(ptr);

            return 
            {
                reint_ptr->ast_node_ptr->line,
                reint_ptr->ast_node_ptr->col
            };
        }

        case SymbolType::COMPONENT:
        {
            const ComponentSymbol *reint_ptr = 
                static_cast<const ComponentSymbol*>(ptr);

            return
            {
                reint_ptr->ast_node_ptr->line,
                reint_ptr->ast_node_ptr->col
            };

        }

        case SymbolType::FN:
        {
            const FunctionSymbol *reint_ptr = 
                static_cast<const FunctionSymbol*>(ptr);

            return
            {
                reint_ptr->ast_node_ptr->line,
                reint_ptr->ast_node_ptr->col
            };
        }

        case SymbolType::VAR:
        {
            const VarSymbol *reint_ptr = 
                static_cast<const VarSymbol*>(ptr);

            return 
            {
                reint_ptr->ast_node_ptr->line,
                reint_ptr->ast_node_ptr->col
            };
        }

        case SymbolType::PARAM:
        {
            const ParamSymbol *reint_ptr = 
                static_cast<const ParamSymbol*>(ptr);

            return 
            {
                reint_ptr->ast_node_ptr->line,
                reint_ptr->ast_node_ptr->col
            };
        }

        case SymbolType::MODULE:
        {
            std::cout << "Can't find symbol position of a Module symbol\n";
            exit(1);

            // const ModuleSymbol *reint_ptr = 
            //     static_cast<const ModuleSymbol*>(ptr);

            // return
            // {
            //     reint_ptr->as_node
            // }
        }

        case SymbolType::FIELD:
        {
            const FieldSymbol *reint_ptr = 
                static_cast<const FieldSymbol*>(ptr);

            return 
            {
                reint_ptr->ast_node_ptr->line,
                reint_ptr->ast_node_ptr->col
            };
        }

        default:

            std::cout << "Attempted to find position of Symbol, but failed.\n";
            exit(1);
    }
}

uint64_t SymbolResolver::find_symbol_idx(
    const std::vector<std::string> &ident_path, uint64_t scope_idx, 
    uint32_t symbol_line, uint32_t symbol_col)
{
    if(ident_path.size() == 0)
    { 
        std::cout << "Passed an ident path of size 0 to find_symboL_idx.\n";
        exit(1);
    }

    // Only a single identifier is provided. Search the current scope upward 
    // until we've searched the global scope.
    if(ident_path.size() == 1)
    {   
        const Scope *parsed_scope = &s_table->scopes.at(scope_idx);

        // bool worry_about_use_before_decl = inside_body;
        
        // We want to worry about use before declaration if the symbol we're 
        // searching for starts inside a function body.
        bool catch_use_before_decl =
            parsed_scope->owning_symbol_type == SymbolType::FN || 
            parsed_scope->owning_symbol_type == SymbolType::SCOPE;

        while(true)
        {
            // We don't want to search Component fields, just skip it.
            if(parsed_scope->owning_symbol_type != SymbolType::COMPONENT)
            {
                for(const auto &elem: parsed_scope->sym_name_to_symbol_idx)
                {
                    // If identifier doesn't match.
                    if(elem.first != ident_path[0]) continue; 
                    
                    // Identifier matches. If we're inside a body, make sure 
                    // that the found identifier is placed ABOVE where we are
                    // attempting to find it, so things like 
                    // 
                    // t0 = 3
                    // let t0: int;
                    // 
                    // fail to compile. (Use before declaration)

                    // We are not worried about use before declaration.
                    if(!catch_use_before_decl) return elem.second;

                    // Get the symbol of the identifier match we've just found.
                    const Symbol *found_sym = 
                        s_table->symbols.at(elem.second).get();

                    std::pair<uint32_t, uint32_t> found_sym_pos = 
                        get_symbol_pos(found_sym);

                    if(found_sym_pos.first > symbol_line || 
                        (found_sym_pos.first == symbol_line && 
                        found_sym_pos.second > symbol_col))
                    {
                        print_error_location(symbol_line, symbol_col);
                        std::cout << " -> Symbol: \"";
                        print_ident_path(ident_path, std::cout);
                        std::cout << "\" used before declaration.\n";
                        exit(1);
                    }

                    return elem.second;
                }
            }

            // There are no more scopes above this, we failed to find the 
            // symbol.
            if(!parsed_scope->parent_scope_idx)
            {
                print_error_location(symbol_line, symbol_col);
                std::cout << " -> Undefined symbol: \"";
                print_ident_path(ident_path);
                std::cout << "\".\n";
                exit(1);
            }

            // Parse the scope above this one.
            parsed_scope = &s_table->scopes.at(
                *parsed_scope->parent_scope_idx);

            // If we are concerned about use before declaration, and we are 
            // leaving the function we were in, we don't need to worry about 
            // use before declaration anymore.
            if(catch_use_before_decl && 
                (parsed_scope->owning_symbol_type != SymbolType::FN && 
                 parsed_scope->owning_symbol_type != SymbolType::SCOPE))
            {
                catch_use_before_decl = false;
            }
        }
    }

    // We have a scope chain, start from the global namespace and search for
    // the target scope.

    const Scope *parsed_scope = &s_table->scopes.at(
        s_table->symbols.at(s_table->global_symbol_idx)->
        scope_idx.value());

    uint64_t targ_symbol_idx = 0;

    // Start by searching through the module subscripts
    for(size_t i = 0; i < ident_path.size() - 1; ++i)
    {
        const std::string &targ_module = ident_path[i];

        bool match = false;

        // Search for this Module name inside this scope.
        for(const auto &elem: parsed_scope->sym_name_to_symbol_idx)
        {
            if(elem.first != targ_module) continue;

            // Found a symbol that matches this Module name, we need to make 
            // sure it's a Module symbol.

            targ_symbol_idx = elem.second;
            match = true;
            break;
        }

        if(!match)
        {
            std::cout << parsed_file << ":" << symbol_line << ":" <<   
                symbol_col << " -> Could not find Module: \"" << targ_module << 
                "\"\n";
            exit(1);
        }

        // First, check that the symbol we've found that matched the ident path
        // module name is actually a symbol module
        if(s_table->symbols.at(targ_symbol_idx)->sym_type != 
            SymbolType::MODULE)
        {
            std::cout << parsed_file << ':' << symbol_line << ':' << symbol_col
                << " -> \"" << targ_module << "\" is not a Module and cannot be"
                " subscripted.\n";
            exit(1);
        }

        // Update the parsed scope to now point to this found module's scope.
        parsed_scope = &s_table->scopes.at(
            static_cast<const ModuleSymbol*>(
            s_table->symbols[targ_symbol_idx].get())->created_scope_idx);
    }

    // We've followed any Module chain to the depth where the target symbol 
    // lives, search for it.

    const std::string &targ_symbol_name = ident_path.back();

    for(const auto &elem : parsed_scope->sym_name_to_symbol_idx)
    {
        if(elem.first == targ_symbol_name) return elem.second;
    }

    print_error_location(symbol_line, symbol_col);
    std::cout << " -> Undefined symbol: \"";
    print_ident_path(ident_path);
    std::cout << "\".\n";
    exit(1);
} 

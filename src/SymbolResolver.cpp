// SymbolResolver.cpp

#include "SymbolResolver.hpp"

#include <iostream>


// Ctors / Dtors

SymbolResolver::SymbolResolver() {}


// Public

void SymbolResolver::resolve(Program &p, const SymbolTable &s_table)
{
    s_table_ptr = &s_table;
    parsed_file = p.source_file_name;
       
    resolve_module(p.ast.get());

    s_table_ptr = nullptr;
    parsed_file = nullptr;
}


// Private

void SymbolResolver::resolve_module(ModuleDecl * const ptr)
{
    uint64_t scope_idx = static_cast<ModuleSymbol*>(s_table_ptr->symbols.at(
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

                std::cerr << "Invalid declaration type found for declaration."
                    "\n";
                exit(1); 
        }
    }
}

void SymbolResolver::resolve_field(FieldDecl * const ptr)
{
    Symbol * const symbol_ptr = 
        s_table_ptr->symbols.at(ptr->symbol_idx.value()).get();

    resolve_type_decl(ptr->type_decl.get(), symbol_ptr->scope_idx.value());
}

void SymbolResolver::resolve_struct(StructDecl * const ptr)
{
    uint64_t scope_idx = static_cast<StructSymbol*>(s_table_ptr->symbols.at(
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

    resolve_block(&ptr->body);
}

void SymbolResolver::resolve_component(ComponentDecl * const ptr)
{
    uint64_t scope_idx = static_cast<ComponentSymbol*>(s_table_ptr->symbols.at(
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
                find_symbol_idx(reint_ptr->ident_path, scope_idx, ptr->line,
                ptr->col);

            break;
        }

        case TypeKind::PTR:

            resolve_type_decl(static_cast<PtrTypeDecl*>(ptr)->pointee.get(),
                scope_idx);
            break;

        case TypeKind::REF:

            resolve_type_decl(static_cast<RefTypeDecl*>(ptr)->referred.get(),
                scope_idx);
            break;

        case TypeKind::ARRAY:

            resolve_type_decl(
                static_cast<ArrTypeDecl*>(ptr)->element_type.get(), scope_idx);            
            break;

        case TypeKind::FUNC_PTR:

            break;

        default:

            std::cerr << "Invalid TypeKind for type declaration: "
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

            std::cerr << parsed_file << ":" << ptr->line << ":" << ptr->col << 
                ": Invalid type for declaration.\n";
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

            resolve_expression(static_cast<RetStmt*>(ptr)->ret_expr.get(), 
                scope_idx);
            break;

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

            std::cerr << parsed_file << ":" << ptr->line << ":" << ptr->col <<
                ": Invalid statement type.\n";
            exit(1);
            break;
    }
}

void SymbolResolver::resolve_expression(Expression * const ptr,
    uint64_t scope_idx) 
{
    switch(ptr->exp_type)
    {
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
                find_symbol_idx(reint_ptr->ident_path, scope_idx, reint_ptr->line,
                    reint_ptr->col);
            break;
        }

        case ExpressionType::MEMBER_ACCESS:
        {
            MemberAccExpr * const reint_ptr = 
                static_cast<MemberAccExpr*>(ptr);

            resolve_expression(reint_ptr->base_expr.get(), scope_idx);

            // reint_ptr->resolved_symbol_idx = 
            //     find_symbol_idx(reint_ptr->member_name, scope_idx,
            //     reint_ptr->line, reint_ptr->col);
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

            break;

       
        case ExpressionType::UNARY:
        {
            UnaryExpr * const reint_ptr = 
                static_cast<UnaryExpr*>(ptr);

            resolve_expression(reint_ptr->operand.get(), scope_idx);

            break;
        }        

        default:

            break;
    }
}

std::optional<uint64_t> SymbolResolver::symbol_idx_from_expr(
    const Expression * const ptr, uint64_t scope_idx)
{
    switch(ptr->exp_type)
    {
        case ExpressionType::BIN_LITERAL:
        {
            std::vector<std::string> temp_ident_path;
            temp_ident_path.push_back(BIN_LIT_IDENT);

            return find_symbol_idx(temp_ident_path, scope_idx, ptr->line, 
                ptr->col);
        }

        case ExpressionType::BINARY:
        {
            const BinaryExpr * const reint_ptr =  
                static_cast<const BinaryExpr*>(ptr);

            std::optional<uint64_t> lhs_symbol_idx = 
                symbol_idx_from_expr(reint_ptr->lhs.get(), scope_idx);

            std::optional<uint64_t> rhs_symbol_idx = 
                symbol_idx_from_expr(reint_ptr->rhs.get(), scope_idx);

            if(lhs_symbol_idx.has_value()) return lhs_symbol_idx.value();
            if(rhs_symbol_idx.has_value()) return rhs_symbol_idx.value();

            break;
        }

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

            // std::cerr << parsed_file << ':' << ptr->line << ':' << ptr->col << 
            //     ": Attempted to get index of symbol of expression result, but 
            //     this expression does not return a type.\n";
            return {};
    }

    return {};
}

uint64_t SymbolResolver::find_symbol_idx(
    const std::vector<std::string> &ident_path, uint64_t scope_idx, 
    uint32_t symbol_line, uint32_t symbol_col)
{ 
    if(ident_path.size() == 0)
    {
        std::cerr << "Passed an ident path of size 0 to find_symboL_idx.\n";
        exit(1);
    }

    // Only a single identifier is provided. Search the current scope upward 
    // until we've searched the global scope.
    if(ident_path.size() == 1)
    {   
        const Scope *parsed_scope = &s_table_ptr->scopes.at(scope_idx);
    
        while(true)
        {
            // We don't want to search Component fields, just skip it.
            if(parsed_scope->owning_symbol_type != SymbolType::COMPONENT)
            {
                for(const auto &elem: parsed_scope->sym_name_to_symbol_idx)
                {
                    // Identifier match.
                    if(elem.first == ident_path[0]) return elem.second;
                }
            }

            // There are no more scopes above this, we failed to find the 
            // symbol.
            if(!parsed_scope->parent_scope_idx)
            {
                std::cerr << parsed_file << ":" << symbol_line << ":" << 
                symbol_col << ": Undefined symbol: " << ident_path[0] << '\n';
                exit(1);
            }

            // Parse the scope above this one.
            parsed_scope = &s_table_ptr->scopes.at(
                *parsed_scope->parent_scope_idx);
        }
    }

    // We have a scope chain, start from the global namespace and search for
    // the target scope.

    const Scope *parsed_scope = &s_table_ptr->scopes.at(
        s_table_ptr->symbols.at(s_table_ptr->global_symbol_idx)->
        scope_idx.value());

    const std::string *targ_ident = &ident_path[0];

    uint64_t target_scope_idx;

    while(true)
    {
        bool match = false;
        
        for(const auto &elem: parsed_scope->sym_name_to_symbol_idx)
        {
            // Identifier match.
            if(elem.first == *targ_ident)
            {
                // We need to make sure the symbol we've found is a module, and
                // not just a random symbol.

                match = true; 
                break;
            }
        }

        if(match) break;

        // We didn't find one of the module names in the scope chain.
    }
    
    // const Scope * parsed_scope = &s_table_ptr->scopes.at(scope_idx);

    // for(const auto &elem: parsed_scope->sym_name_to_symbol_idx)
    // {
    //     if(elem.first == ident) return elem.second;
    // }

    // if(parsed_scope->parent_scope_idx.has_value()) 
    //     return find_symbol_idx(ident, parsed_scope->parent_scope_idx.value(),
    //         symbol_line, symbol_col);

    // // Didn't find the symbol.
    // std::cerr << parsed_file << ":" << symbol_line << ":" << symbol_col << 
    //     ": Undefined symbol: " << ident << '\n';
    // exit(1);
} 

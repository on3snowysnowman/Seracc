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

                resolve_function(static_cast<FunctionDecl*>(ptr.get()));
                break;

            case DeclKind::COMPONENT:

                resolve_component(static_cast<ComponentDecl*>(ptr.get()));
                break;

            default:

                std::cerr << "Invalid declaration type found for: " << ptr->name
                    << '\n';
                exit(1); 
                break;
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

}

void SymbolResolver::resolve_function(FunctionDecl * const ptr)
{

}

void SymbolResolver::resolve_component(ComponentDecl * const ptr)
{

}

void SymbolResolver::resolve_type_decl(TypeDecl * const ptr, 
    uint64_t scope_idx)
{
    switch(ptr->kind)
    {
        case TypeKind::NAMED:

            NamedTypeDecl * const reint_ptr = 
                static_cast<NamedTypeDecl*>(ptr);

            reint_ptr->resolved_symbol_idx = 
                find_symbol_idx(reint_ptr->type_name, scope_idx, ptr->line,
                ptr->col);
            break;

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

            std::cerr << "Invalid TypeKind for type declaration\n";
            exit(1);
            break;
    }
}

void SymbolResolver::resolve_statement(Statement * const ptr) {}

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
                find_symbol_idx(reint_ptr->name, scope_idx, reint_ptr->line,
                    reint_ptr->col);
            break;
        }

        case ExpressionType::MEMBER_ACCESS:
        {
            MemberAccExpr * const reint_ptr = 
                static_cast<MemberAccExpr*>(ptr);

            resolve_expression(reint_ptr->base_expr.get(), scope_idx);
            
            reint_ptr->resolved_symbol_idx = 
                find_symbol_idx(reint_ptr->member_name, scope_idx,
                reint_ptr->line, reint_ptr->col);

            break;
        }

        case ExpressionType::SUBSCRIPT:

            break;

        case ExpressionType::TERNARY:

            break;

        case ExpressionType::UNARY:

            break;
    }
}

uint64_t SymbolResolver::find_symbol_idx(const std::string &ident, 
    uint64_t scope_idx, uint64_t symbol_line, uint64_t symbol_col)
{
    const Scope * parsed_scope = &s_table_ptr->scopes.at(scope_idx);

    for(const auto &elem: parsed_scope->sym_name_to_symbol_idx)
    {
        if(elem.first == ident) return elem.second;
    }

    if(parsed_scope->parent_scope_idx.has_value()) 
        find_symbol_idx(ident, parsed_scope->parent_scope_idx.value(),
            symbol_line, symbol_col);

    // Didn't find the symbol.
    std::cerr << parsed_file << ":" << symbol_line << ":" << symbol_col << 
        ": Undefined symbol: " << ident << '\n';
    exit(1);
} 
// TypeChecker.hpp

#pragma once

#include "Program.hpp"
#include "SymbolDeclarations.hpp"
#include "ASTDeclarations.hpp"


class TypeChecker
{

public:

    TypeChecker();

    void type_check(const Program &p, const SymbolTable &s_table);

private:

    void print_error_location(uint32_t line, uint32_t col) const;
    void print_type_mismatch(const TypeDecl *first, const TypeDecl *second,
        uint32_t line, uint32_t col)
        const;

    // Parses a type decl chain until it reaches the base NamedTypeDecl
    // and returns its resolved symbol idx, erroring if it does not have
    // one.
    uint64_t get_resolved_idx_from_type_decl(const TypeDecl * const ptr) const;
    
    void check_module(ModuleDecl * const ptr);
    void check_struct(StructDecl * const ptr);
    void check_component(ComponentDecl * const ptr);
    void check_function(FunctionDecl * const ptr, 
        std::optional<ComponentDecl*> owning_component = {});
    void check_block(ScopeBody * const ptr, 
        std::optional<const TypeDecl*> expected_ret_type = {});

    void check_field(FieldDecl * const ptr);
    void check_param(Parameter * const ptr);
    void check_var_decl(VarDeclStmt * const ptr);
    void check_declaration(Declaration * const ptr, 
        std::optional<ComponentDecl*> enclosing_component = {});
    void check_statement(Statement * const ptr, 
        std::optional<const TypeDecl*> expected_ret_type = {});

    void print_type_decl(const TypeDecl *decl) const;

    struct CheckExprResult
    {
        const TypeDecl* type_ptr;
        bool is_type_mutable;    
    };

    void check_cast(const CheckExprResult &first, const CheckExprResult &second,  
        uint32_t expr_line, uint32_t expr_col) const;

    void cmp_named_decls(const NamedTypeDecl *first, 
        const NamedTypeDecl *second, uint32_t expr_line, uint32_t expr_col) 
        const;
    void cmp_ptr_decls(const PtrTypeDecl *first,
        const PtrTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const;
    void cmp_type_decls(const TypeDecl *first, const TypeDecl *second,
        uint32_t expr_line, uint32_t expr_col);

    bool is_symbol_mutable(const Symbol *ptr, uint32_t symbol_line,
        uint32_t symbol_col) const;
    bool is_type_mutable(const TypeDecl *ptr) const;

    CheckExprResult check_ident_expr(const IdentExpr * const ptr);
    CheckExprResult check_expression(const Expression * const ptr);

    const char * parsed_file = nullptr;
    const SymbolTable * s_table = nullptr;
};

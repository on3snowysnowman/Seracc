// TypeChecker.hpp

#pragma once

#include "Program.hpp"
#include "SymbolDeclarations.hpp"
#include "ASTDeclarations.hpp"


class TypeChecker
{

public:

    TypeChecker();
    ~TypeChecker();

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
        const TypeDecl* type_ptr = nullptr;
        std::optional<BuiltinPtrType> builtin_ptr_type;
        bool is_var_mutable = false;    
        bool is_lvalue = false;
    };

    
    CheckExprResult check_cast(const CheckExprResult &first, 
        const CheckExprResult &second,  uint32_t expr_line, 
        uint32_t expr_col) const;

    void cmp_named_decls(const NamedTypeDecl *first, 
        const NamedTypeDecl *second, uint32_t expr_line, uint32_t expr_col) 
        const;
    void cmp_ptr_decls(const PtrTypeDecl *first,
        const PtrTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const;
    void cmp_type_decls(const TypeDecl *first, const TypeDecl *second,
        uint32_t expr_line, uint32_t expr_col) const;

    bool is_symbol_mutable(const Symbol *ptr, uint32_t symbol_line,
        uint32_t symbol_col) const;
    bool is_var_mutable(const NamedTypeDecl *ptr) const;
    bool is_ptr_opaque_ptr(const TypeDecl *ptr) const;

    CheckExprResult check_post_unary_expr(const UnaryExpr * const ptr,
        bool is_post_increment);
    CheckExprResult check_addr_of_unary_expr(const UnaryExpr * const ptr);
    CheckExprResult check_unary_expr(const UnaryExpr * const ptr);
    CheckExprResult check_ident_expr(const IdentExpr * const ptr);
    CheckExprResult check_expression(const Expression * const ptr);

    const char * parsed_file = nullptr;
    const SymbolTable * s_table = nullptr;

    // TypeDecl that are created on heap that are managed by the TypeChecker 
    // class and will be freed on its dtor.
    std::vector<TypeDecl*> created_decls;
};

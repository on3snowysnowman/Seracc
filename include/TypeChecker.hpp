// TypeChecker.hpp

#pragma once

#include <optional>
#include <vector>

#include "Program.hpp"
#include "SymbolDeclarations.hpp"

enum class FitsInOption
{
    NUMBER,
    HEX,
    BINARY
};


class TypeChecker
{

public:

    TypeChecker();
    ~TypeChecker();

    void type_check(const Program &p, const SymbolTable &sym_table);

private:

    // Members

    const Program *program = nullptr;
    const SymbolTable *s_table = nullptr;

    std::vector<TypeDecl*> decls_to_delete;


    // Methods
    
    // Output
    void print_error_location(uint32_t line, uint32_t col) const;
    void print_type(const TypeDecl *ptr) const;
    void print_type_mismatch(const TypeDecl *first, const TypeDecl *second,
        uint32_t expr_line, uint32_t expr_col) const;
    void print_invalid_init_expr(uint32_t line, uint32_t col,
        const TypeDecl *type) const;

    // Get the symbol idx that a TypeDecl references
    uint64_t resolve_type_decl(const TypeDecl *ptr) const;
    bool is_type_uint_literal(const TypeDecl *ptr) const;

    // Given a scope id (accessing_sym_id) that wants to access a private 
    // field in some scope (targ_scope_id), check to make sure that this is a 
    // proper access.
    void check_private_access(const uint64_t targ_scope_id, 
        const uint64_t accessing_scope_id, uint32_t expr_line, 
        uint32_t expr_col, const std::vector<std::string> &ident_path) const;

    // Type checking/comparing
    void cmp_ptr_types(const PtrTypeDecl *first, 
        const PtrTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const;
    void cmp_named_types(const NamedTypeDecl *first, 
        const NamedTypeDecl *second, uint32_t expr_line, uint32_t expr_col) 
        const;
    void cmp_types(const TypeDecl *first, const TypeDecl *second,
        uint32_t expr_line, uint32_t expr_col) const;
    void check_type(const TypeDecl *ptr, const uint64_t type_scope_id);

    // Main checking
    void check_module(const ModuleDecl *ptr);
    void check_component(const ComponentDecl *ptr);  
    void check_struct(const StructDecl *ptr);
    void check_function(const FunctionDecl *ptr, 
        std::optional<uint64_t> owning_component_sym_idx);
    void check_scope_body(const ScopeBody *ptr,
        std::optional<const TypeDecl*> expected_return_type);
    void check_var_decl_stmt(const VarDeclStmt *ptr);
    void check_statement(const Statement *ptr,
        const uint64_t scope_id,
        std::optional<const TypeDecl*> expected_return_type);
    void check_param(const Parameter *ptr);
    void check_decl(const Declaration *ptr, 
        std::optional<uint64_t> owning_component_sym_idx = {});

    struct CheckExprResult
    {
        const TypeDecl *type_decl = nullptr;
        bool is_lvalue = false;
        bool is_var_and_mutable = false;
    };

    void init_literal_typedecl(NamedTypeDecl *new_decl, const std::string &s, 
        FitsInOption option) const;

    const StructDecl*get_struct_decl_from_type(const TypeDecl *type, 
        uint32_t expr_line, uint32_t expr_col) const;

    // Expression checking
    CheckExprResult check_struct_init_expr(const StructInitExpr *expr,
        const TypeDecl *var_type, const uint64_t var_scope_id);
    CheckExprResult check_struct_create_expr(const StructCreateExpr *expr,
        const uint64_t scope_id);
    void recurse_check_arr_init(const ArrInitExpr *init_expr, 
        const uint8_t depth, const std::vector<size_t> &size_values, 
        const TypeDecl *element_type, const uint64_t scope_id);
    CheckExprResult check_arr_init_expr(const ArrInitExpr *expr, 
        const ArrTypeDecl *arr_type, const uint64_t var_scope_id);
    CheckExprResult check_ident_expr(const IdentExpr *ptr, 
        const uint64_t scope_id);
    CheckExprResult check_expression(const Expression *ptr, 
        const uint64_t scope_id); 
};

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
    void print_invalid_type_assignment(const TypeDecl *first, 
        const TypeDecl *second, uint32_t expr_line, uint32_t expr_col) const;
    void print_invalid_integral_type(const TypeDecl *type, uint32_t line, 
        uint32_t col) const;
    void print_invalid_bool_type(const TypeDecl *type, uint32_t line, 
        uint32_t col) const;

    // Get the symbol idx that a TypeDecl references
    uint64_t resolve_type_decl(const TypeDecl *ptr) const;

    // Attempts to convert an expression that represents a single uint literal
    // into its numeric value. If optional is empty, conversion failed.
    std::optional<uint64_t> expr_to_uint64(const Expression *ptr) const;
    bool is_type_literal(const TypeDecl *ptr) const;
    bool is_type_numeric(const TypeDecl *ptr) const;
    bool is_type_integral(const TypeDecl *ptr) const;
    bool is_type_uintegral(const TypeDecl *ptr) const;
    bool is_type_numeric_literal(const TypeDecl *ptr) const;
    bool is_type_int_literal(const TypeDecl *ptr) const;
    bool is_type_uint_literal(const TypeDecl *ptr) const;
    bool is_type_bool(const TypeDecl *ptr) const;
    std::optional<const StructDecl*> is_namedtype_struct(
        const NamedTypeDecl *ptr) const;
    std::optional<const ComponentDecl*> is_namedtype_component(
        const NamedTypeDecl *ptr) const;

    // Given a scope id (accessing_sym_id) that wants to access a private 
    // field in some scope (targ_scope_id), check to make sure that this is a 
    // proper access.
    void check_private_access(const uint64_t targ_scope_id, 
        const uint64_t accessing_scope_id, uint32_t expr_line, 
        uint32_t expr_col, const std::vector<std::string> &ident_path) const;

    // Type checking/comparing
    bool cmp_ptr_types(const PtrTypeDecl *first, const PtrTypeDecl *second) 
        const;
    bool cmp_arr_types(const ArrTypeDecl *first, const ArrTypeDecl *second)
        const;
    bool cmp_ref_types(const RefTypeDecl *first, const RefTypeDecl *second)
        const;
    bool cmp_two_builtin_type(const NamedTypeDecl *first_builtin, 
        const NamedTypeDecl *second_builtin) const;
    bool cmp_builtin_type(const NamedTypeDecl *builtin_type, 
        const NamedTypeDecl *other_type) const;
    bool cmp_named_types(const NamedTypeDecl *first, 
        const NamedTypeDecl *second) const;
    bool cmp_types(const TypeDecl *first, const TypeDecl *second) const;
    void check_type(TypeDecl *ptr, const uint64_t type_scope_id, 
        TypeKind last_type = TypeKind::INVALID);
    void check_named_cast(const NamedTypeDecl *casting_type,
        const NamedTypeDecl *being_casted_type, uint32_t expr_line, 
        uint32_t expr_col);

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

    const StructDecl* get_struct_decl_from_type(const TypeDecl *type, 
        uint32_t expr_line, uint32_t expr_col) const;

    const FieldDecl* get_struct_member(const StructDecl *decl, 
        const std::string &member_name, uint32_t expr_line, uint32_t expr_col);
    const FieldDecl* get_comp_member(const ComponentDecl *decl,
        const std::string &member_name, uint32_t expr_line, uint32_t expr_col);
    const FunctionDecl* get_func_decl(const Expression *ptr);
    
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
    CheckExprResult check_prepost_incdec(const UnaryExpr *ptr, 
        const CheckExprResult operand_res);
    CheckExprResult check_expr_ensure_integral(const Expression *ptr, 
        const uint64_t scope_id);
    CheckExprResult check_addr_of_expr(const UnaryExpr *ptr, 
        const CheckExprResult operand_res);
    CheckExprResult check_ref_of_expr(const UnaryExpr *ptr, 
        const CheckExprResult operand_res);
    CheckExprResult check_deref_expr(const UnaryExpr *ptr,
        const CheckExprResult operand_res);
    CheckExprResult check_unary_expr(const UnaryExpr *ptr,
        const uint64_t scope_id);
    CheckExprResult check_arith_expr(const BinaryExpr *ptr,
        const uint64_t scope_id, bool is_addsub_expr);
    CheckExprResult check_shift_expr(const BinaryExpr *ptr,
        const uint64_t scope_id);
    CheckExprResult check_cmp_expr(const BinaryExpr *ptr,
        const uint64_t scope_id, bool is_equals_expr);
    CheckExprResult check_log_expr(const BinaryExpr *ptr,
        const uint64_t scope_id);
    CheckExprResult check_binary_expr(const BinaryExpr *ptr, 
        const uint64_t scope_id);
    CheckExprResult check_cast_expr(const CastExpr *ptr, 
        const uint64_t scope_id);
    CheckExprResult check_subscript_expr(const SubscriptExpr *ptr,
        const uint64_t scope_id);
    CheckExprResult check_memaccess_expr(const MemberAccExpr *ptr, 
        const uint64_t scope_id);
    CheckExprResult check_call_expr(const CallExpr *ptr, 
        const uint64_t scope_id);
    CheckExprResult check_expression(const Expression *ptr, 
        const uint64_t scope_id); 
};

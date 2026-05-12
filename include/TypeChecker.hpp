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

    const Program * program = nullptr;
    const SymbolTable * s_table = nullptr;

    std::vector<TypeDecl*> decls_to_delete;


    // Methods
    
    void print_error_location(uint32_t line, uint32_t col) const;
    void print_type(const TypeDecl *ptr) const;
    void print_type_mismatch(const TypeDecl *first, const TypeDecl *second,
        uint32_t expr_line, uint32_t expr_col) const;

    // Get the symbol idx that a TypeDecl references
    uint64_t resolve_type_decl(const TypeDecl * ptr) const;

    void cmp_ptr_types(const PtrTypeDecl *first, 
        const PtrTypeDecl *second, uint32_t expr_line, uint32_t expr_col) const;
    void cmp_named_types(const NamedTypeDecl *first, 
        const NamedTypeDecl *second, uint32_t expr_line, uint32_t expr_col) 
        const;
    void cmp_types(const TypeDecl *first, const TypeDecl *second,
        uint32_t expr_line, uint32_t expr_col) const;
    void check_type(const TypeDecl *ptr) const;

    void check_module(const ModuleDecl *ptr);
    void check_component(const ComponentDecl *ptr);  
    void check_function(const FunctionDecl * ptr, 
        std::optional<uint64_t> owning_component_sym_idx);
    void check_scope_body(const ScopeBody * ptr,
        std::optional<const TypeDecl*> expected_return_type);
    void check_statement(const Statement * ptr,
        std::optional<const TypeDecl*> expected_return_type);
    void check_param(const Parameter * ptr);
    void check_decl(const Declaration * ptr, 
        std::optional<uint64_t> owning_component_sym_idx = {});
    
    struct CheckExprResult
    {
        const TypeDecl *type_decl = nullptr;
        bool is_var_and_mutable = false;
    };

    void init_literal_typedecl(NamedTypeDecl *new_decl, const std::string &s, 
        FitsInOption option) const;

    CheckExprResult check_expression(const Expression *ptr); 
};

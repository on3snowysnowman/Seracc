// TypeChecker.hpp

#pragma once

#include <optional>

#include "Program.hpp"
#include "SymbolDeclarations.hpp"


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


    // Methods
    
    void print_error_location(uint32_t line, uint32_t col) const;

    // Get's the symbol idx that a TypeDecl references
    uint64_t resolve_type_decl(const TypeDecl * ptr) const;

    void check_module(const ModuleDecl *ptr);
    void check_component(const ComponentDecl *ptr);  
    void check_function(const FunctionDecl * ptr, 
        std::optional<uint64_t> owning_component_sym_idx);
    void check_scope_body(const ScopeBody * ptr,
        std::optional<const TypeDecl*> expected_return_type);
    void check_statement(const Statement * ptr,
        std::optional<const TypeDecl*> expected_return_type);
    void check_param(const Parameter * ptr);
    const TypeDecl* check_expression(const Expression *ptr); 

    void check_decl(const Declaration * ptr, 
        std::optional<uint64_t> owning_component_sym_idx = {});
};

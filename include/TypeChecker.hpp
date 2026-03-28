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

    // Parses a type decl chain until it reaches the base NamedTypeDecl
    // and returns its resolved symbol idx, erroring if it does not have
    // one.
    uint64_t get_resolved_idx_from_type_decl(const TypeDecl * const ptr) const;
    
    void check_module(ModuleDecl * const ptr);
    void check_struct(StructDecl * const ptr);
    void check_component(ComponentDecl * const ptr);
    void check_function(FunctionDecl * const ptr, 
        std::optional<ComponentDecl*> owning_component = {});
    void check_block(ScopeBody * const ptr);

    // Given an expression, returns the index of the type symbol the expression
    // results in.
    uint64_t resolve_type_from_expr(Expression * const ptr);
    void check_field(FieldDecl * const ptr);
    void check_param(Parameter * const ptr);
    void check_var_decl(VarDeclStmt * const ptr);
    void check_type_decl(TypeDecl * const ptr);
    void check_declaration(Declaration * const ptr, 
        std::optional<ComponentDecl*> enclosing_component = {});
    void check_statement(Statement * const ptr);
    void check_expression(Expression * const ptr);

    const char * parsed_file = nullptr;
    const SymbolTable * s_table = nullptr;
};

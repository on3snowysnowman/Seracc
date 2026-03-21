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

    void check_module(ModuleDecl * const ptr);
    void check_struct(StructDecl * const ptr);
    void check_component(ComponentDecl * const ptr);
    void check_function(FunctionDecl * const ptr);
    void check_block(ScopeBody * const ptr);

    // Given an expression, returns the index of the type symbol the expression
    // results in.
    uint64_t resolve_type_from_expr(Expression * const ptr);
    void check_field(FieldDecl * const ptr);
    void check_param(Parameter * const ptr);
    void check_var_decl(VarDeclStmt * const ptr);
    void check_type_decl(TypeDecl * const ptr);
    void check_declaration(Declaration * const ptr);
    void check_statement(Statement * const ptr);
    void check_expression(Expression * const ptr);

    const char * parsed_file = nullptr;
    const SymbolTable * s_table = nullptr;
};

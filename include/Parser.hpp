// Parser.hpp

#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <string>

#include "Lexer.hpp"
#include "Declarations.hpp"


struct Program
{
    std::vector<std::unique_ptr<Decl>> decls;
};


class Parser
{

public:

    Parser();

    Program parse_program(const char *file_path);

private:

    // Members

    Token current_token;

    Lexer lexer;


    // Methods

    void handle_tok_mismatch(const Token &got_token, TokenType expected);

    void expect_scope();

    std::unique_ptr<Decl> parse_top_level();

    std::unique_ptr<NamespaceDecl> parse_namespace();

    std::unique_ptr<FunctionDecl> parse_function();

    std::unique_ptr<StructDecl> parse_struct();

    std::unique_ptr<ComponentDecl> parse_component();

    Field parse_field();

    TypeRef parse_type_ref();

    bool check(TokenType t) const;

    bool consume_if(TokenType t);

    Token expect(TokenType t);

    const Token& peek() const; 
    
    Token advance();
};

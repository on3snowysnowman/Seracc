// Parser.hpp

#pragma once

#include "Program.hpp"
#include "Token.hpp"
#include "Lexer.hpp"


class Parser
{

public:

    Parser();

    Program parse(const char *in_file_path);

private:

    // Members

    Lexer lexer;

    Token current_token;

    const char *parsed_file = nullptr;

    // Methods

    void print_error_location(uint32_t line,uint32_t col) const;
    void handle_tok_mismatch(const Token &got_tok, TokenID expected);
    void handle_unexpected_token(const Token &got_tok);

    std::unique_ptr<Declaration> parse_top_level();
    std::unique_ptr<NamespaceDecl> parse_namespace();
    std::unique_ptr<FunctionDecl> parse_function(bool is_pub);
    std::unique_ptr<StructDecl> parse_struct(bool is_pub);
    std::unique_ptr<ComponentDecl> parse_component(bool is_pub);
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<TypeDecl> parse_type_decl();
    std::unique_ptr<FieldDecl> parse_field(bool is_pub);
    ScopeBody parse_scope();
    Parameter parse_param();

    bool check(TokenID id) const;
    bool consume_if(TokenID id);
    Token expect(TokenID id);
    const Token& peek() const; 
    Token advance();
};



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

    void parse_program(const char *in_file);

private:

    // Members

    Token current_token;

    Lexer lexer;

    // Methods

    void handle_error(const Token &t);

    std::unique_ptr<Decl> parse_top_level();

    std::unique_ptr<Decl> parse_function();

    std::unique_ptr<Decl> parse_struct();

    std::unique_ptr<Decl> parse_component();

    void expect(TokenType t_type);

    const Token& peek() const; 
    
    Token advance();
};

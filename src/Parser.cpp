// Parser.cpp

#include "Parser.hpp"


// Ctors / Dtor

Parser::Parser() {}


// Public

void Parser::print_error_location(uint32_t line, uint32_t col) const
{
    std::cout << parsed_file << ":" << line << ':' << col;
}

void Parser::handle_tok_mismatch(const Token &got_tok, TokenID expected) 
{
    print_error_location(got_tok.line, got_tok.col);
    std::cerr << ": Expect token of type: " << 
        tokID_readable[static_cast<int>(expected)] << " but got token:\n" << 
        got_tok << '\n';
    exit(1);
}

Program Parser::parse(const char *in_file_path) 
{
    lexer.load(in_file_path);

    Program prog;
    prog.source_file_name = in_file_path;
    parsed_file = in_file_path;

    advance(); // Get first token.

    prog.ast = std::move(parse_top_level());

    lexer.close();
    parsed_file = nullptr;

    return prog;
}


// Private

std::unique_ptr<NamespaceDecl> Parser::parse_top_level() 
{
    std::unique_ptr<NamespaceDecl> ptr = std::make_unique<NamespaceDecl>();

    ptr->name = "global";
    ptr->line = 1;
    ptr->col = 1;

    return ptr;
}

std::unique_ptr<NamespaceDecl> Parser::parse_namespace() 
{

}

std::unique_ptr<FunctionDecl> Parser::parse_function() 
{

}

std::unique_ptr<StructDecl> Parser::parse_struct() 
{

}

std::unique_ptr<ComponentDecl> Parser::parse_component() 
{

}

std::unique_ptr<Statement> Parser::parse_statement() 
{


}
std::unique_ptr<Expression> Parser::parse_expression() 
{


}

std::unique_ptr<TypeDecl> Parser::parse_type_decl() 
{

}

FieldDecl Parser::parse_field() 
{

}

ParameterDecl Parser::parse_param() 
{

}

bool Parser::check(TokenID id) const 
{

}

bool Parser::consume_if(TokenID id) 
{

}

Token Parser::expect(TokenID id) 
{
    const Token tok = peek();

    if(tok.id != id)
    {
        handle_tok_mismatch(tok, id);
    } 

    advance();
    return tok;
}

const Token& Parser::peek() const
{
    return current_token;
}

Token Parser::advance() 
{
    Token tok = current_token;

    current_token = lexer.next_token();
    return tok;
}

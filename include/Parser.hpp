// Parser.hpp

#pragma once

#include "Lexer.hpp"

class Parser
{

public:

    Parser();


private:

    // Members

    Token current_token;

    Lexer lexer;
    

    // Methods

    void expect(TokenType t_type) const;

    const Token& peek() const; 
    
    Token advance();
};

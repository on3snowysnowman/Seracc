
#include "Lexer.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>


// Ctors / Dtors

Lexer::Lexer() {}


// Public

void Lexer::load(const char *in_file_path)
{
    std::ifstream in_file(in_file_path);

    if(!in_file)
    {
        std::cerr << "Failed to open input file: " << in_file_path << '\n';
        exit(1);
    }


    std::ostringstream buffer;

    buffer << in_file.rdbuf();

    source = buffer.str();

    in_file.close();

    current_idx = 0;
    current_line = 1;
    current_col = 1;
    file_being_parsed = in_file_path;
}

void Lexer::close()
{
    source.clear();
    current_idx = 0;
    current_line = 1;
    current_col = 1;
    file_being_parsed = nullptr;
}

Token Lexer::next_token() 
{
    skip_trivia();

    Token t;
    t.line = current_line;
    t.col = current_col;

    if(at_eof())
    {
        t.id = TokenID::END_OF_FILE;

        return t;
    }

    const char c = peek();

    // Identifier
    if(std::isalpha(c) || c == '_')
    {
        parse_ident(t);
        return t;
    }

    // Number
    if(std::isdigit(c))
    {
        parse_number(t);
        return t;
    }

    parse_non_ident_or_number(t);

    return t; 
}


// Private

bool Lexer::at_eof() const { return current_idx >= source.size(); }

void Lexer::handle_invalid_char(char c)
{
    std::cout << file_being_parsed << ":" << current_line << ":" << current_col
        << ": Invalid character: " << c << '\n';
    exit(1);
}

void Lexer::skip_till_newline()
{
    while(!at_eof() && peek() != '\n')
    {
        advance();
    } 
}

void Lexer::skip_trivia()
{   
    while(!at_eof())
    {
        const char c = peek();

        if(c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            advance();
            continue;
        }

        // Comment
        if(c == '/' && peek(1) == '/') 
        {
            skip_till_newline();
            continue;
        }

        // Non trivia character, we are done.
        break;
    }
}

void Lexer::parse_ident(Token &t)
{
    char c = peek();

    while((std::isalnum(c) || c == '_'))
    {
        t.text.push_back(c);
        
        advance();
        
        if(at_eof()) break;

        c = peek();
    }

    // Check if the parsed identifier is a keyword
    const auto it = rdbl_kw_to_id.find(t.text);

    // Ident is a keyword
    if(it != rdbl_kw_to_id.end())
    {
        t.id = it->second;
        return;
    }

    // Ident is not a keyword
    t.id = TokenID::IDENTIFIER;
}

void Lexer::parse_number(Token &t)
{
    t.id = TokenID::NUM_LITERAL;

    char c = peek();

    while(std::isdigit(c))
    {
        t.text.push_back(c);

        advance();

        if(at_eof()) break;

        c = peek();
    }
}

void Lexer::parse_non_ident_or_number(Token &t)
{
    char c = advance(); 

    if(c == '-')
    {
        // Check for "->"
        if(peek() == '>')
        {
            advance(); // Consume '>'
            t.id = TokenID::ARROW;
            return;
        }

        t.id = TokenID::MINUS;
        return;
    }

    if(c == '=')
    {
        // Check for "=="
        if(peek() == '=')
        {
            advance(); // Consume '='
            t.id = TokenID::EQUAL_EQUAL;
            return;
        }

        t.id = TokenID::ASSIGN;
        return;
    }

    switch(c)
    {
        case '(':

            t.id = TokenID::LPAREN;
            break;

        case ')':

            t.id = TokenID::RPAREN;
            break;

        case '{':

            t.id = TokenID::LBRACE;
            break;
    
        case '}':

            t.id = TokenID::RBRACE;
            break;

        case '[':

            t.id = TokenID::LBRACKET;
            break;

        case ']':

            t.id = TokenID::RBRACKET;
            break;

        case ',':

            t.id = TokenID::COMMA;
            break;

        case ';':

            t.id = TokenID::SEMICOLON;
            break;

        case '.':

            t.id = TokenID::DOT;
            break;


        case ':':

            t.id = TokenID::COLON;
            break;

        case '&':

            t.id = TokenID::AMPERSAND;
            break;

        case '@':

            t.id = TokenID::AT;
            break;

        case '+':

            t.id = TokenID::PLUS;
            break;

        case '*':

            t.id = TokenID::ASTERISK;
            break;

        case '/':

            t.id = TokenID::FORW_SLASH;
            break;

        case '\\':

            t.id = TokenID::BACK_SLASH;
            break;

        case '<':

            t.id = TokenID::LESS_THAN;
            break;

        case '>':

            t.id = TokenID::GREATER_THAN;
            break;

        default:

            handle_invalid_char(c); // exits 
    }
}

char Lexer::peek(uint32_t offset) const 
{
    return source.at(current_idx + offset);
}

char Lexer::advance()
{
    if(peek() == '\n')
    {
        ++current_line;
        current_col = 1;
    }
    else ++current_col;

    return source.at(current_idx++);
}

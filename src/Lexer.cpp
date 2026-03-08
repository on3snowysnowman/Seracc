
#include "Lexer.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>


// Ctors / Dtors

Lexer::Lexer() {}


// Public

// void Lexer::load(const char *in_file_path)
// {
//     std::ifstream in_file(in_file_path);

//     if(!in_file)
//     {
//         std::cerr << "Failed to open input file: " << in_file_path << '\n';
//         exit(1);
//     }

//     std::ostringstream buffer;

//     buffer << in_file.rdbuf();

//     source = buffer.str();

//     in_file.close();

//     current_idx = 0;
//     current_line = 1;
//     current_col = 1;
//     file_being_parsed = in_file_path;
// }

// void Lexer::close()
// {
//     source.clear();
//     current_idx = 0;
//     current_line = 1;
//     current_col = 1;
//     file_being_parsed = nullptr;
// }

std::vector<Token> Lexer::lex(const char *in_file_path)
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

    std::vector<Token> tokens;
    do
    {
        tokens.push_back(next_token());
    } while (tokens.back().id != TokenID::END_OF_FILE);

    return tokens;
}


// Private

bool Lexer::at_eof() const { return current_idx >= source.size(); }

void Lexer::print_error_location()
{
    std::cerr << file_being_parsed << ":" << current_line << ":" << current_col;
}

void Lexer::handle_invalid_char(char c)
{
    print_error_location();
    std::cerr << ": Invalid character: " << c << '\n';
    exit(1);
}

void Lexer::handle_unexpected_eof()
{
    if(!at_eof()) return;

    std::cerr << file_being_parsed << ":" << current_line << ":" << current_col  
        << ": Reached end of file expecting more characters\n";
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

    // The ident is a keyword
    if(it != rdbl_kw_to_id.end())
    {
        t.id = it->second;
    }

    else if(t.text == "nullptr")
    {
        t.id = TokenID::NULLPTR_LITERAL;
    }

    else if(t.text == "false" || t.text == "true")
    {
        t.id = TokenID::BOOL_LITERAL;
    }

    else
    {
        t.id = TokenID::IDENTIFIER;
    }
}

void Lexer::parse_int_or_flt_literal(Token &t)
{
    t.id = TokenID::INT_LITERAL; // Default to int literal

    // Wether a dot has been found in the literal.
    bool dot_found = false;

    char c = peek();

    while(true)
    {
        if(!isdigit(c))
        {
            if(c == '.')
            {
                if(dot_found)
                {
                    handle_invalid_char(c);
                }

                dot_found = true;
                t.id = TokenID::FLOAT_LITERAL;
            }
            
            // We have found a not dot digit, done.
            else break;
        }

        t.text.push_back(c);
        advance();
        if(at_eof()) break;
        c = peek();
    } 

    // Trailing dot at end of value.
    if(dot_found && *(--t.text.end()) == '.')
    {
        std::cerr << file_being_parsed << ":" << t.line << ":" << t.col << 
            ": Trailing dot at end of float literal.\n\n";
        exit(1);
    }
}

bool is_hex_digit(char c)
{
    return (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || 
        std::isdigit(c);
}

void Lexer::parse_hex_literal(Token &t)
{
    t.id = TokenID::HEX_LITERAL;

    char c = peek();

    while(is_hex_digit(c))
    {
        t.text.push_back(c);

        advance();

        if(at_eof()) break;

        c = peek();
    }
}

void Lexer::parse_bin_literal(Token &t)
{
    t.id = TokenID::BIN_LITERAL;

    char c = peek();

    while(c == '0' || c == '1')
    {
        t.text.push_back(c);

        advance();

        if(at_eof()) break;

        c = peek();
    }
}

void Lexer::parse_str_literal(Token &t)
{
    t.id = TokenID::STR_LITERAL;

    char c = peek();

    while(c != '\"') // c == "
    {
        // If we have a backslash, we need to make sure that if there is a 
        // double quote next it is not seen as the end of the string literal, 
        // and is added to the string literal. We add the backslash along with 
        // it, since when we lower to C, C will need the '\"' for the same 
        // reason we need it here. 
        if(c == '\\') 
        {
            t.text.push_back(c); // Add backslash 
            advance();
            handle_unexpected_eof();
            c = peek();
        }

        t.text.push_back(c);

        advance();

        if(at_eof())
        {
            print_error_location();
            std::cerr << ": Reached end of file while parsing string literal "
                "starting on line: " << t.line << '\n';
            exit(1);
        }

        c = peek();
    }

    if(c != '"')
    {
        print_error_location();
        std::cerr << ": Missing closing double quote for string literal\n";
        exit(1);
    }

    advance(); // Consume "
}

void Lexer::parse_char_literal(Token &t)
{
    t.id = TokenID::CHAR_LITERAL;

    char c = peek();

    if(c == '\\')
    {
        t.text.push_back(c); // Add backslash
        advance();
        handle_unexpected_eof();
        c = peek();
    }

    t.text.push_back(c);

    advance(); // Consume added character
    c = peek();

    handle_unexpected_eof();

    if(c != '\'')
    {
        print_error_location();
        std::cerr << ": Either too many characters in char literal or missing "
            "closing quote\n";
        exit(1);
    }

    advance(); // Consume closing quote
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

        case '!':

            t.id = TokenID::EXCLAMATION_POINT;
            break;

        case '~':

            t.id = TokenID::TILDE;
            break;

        default:

            handle_invalid_char(c); // exits 
    }
}

bool Lexer::is_peek_within_range(uint32_t offset) const
{
    return current_idx + offset < source.size();
}

char Lexer::peek(uint32_t offset) const 
{
    if(!is_peek_within_range(offset))
    {
        std::cerr << "Lexer attempted to peek outside source\n";
        exit(1);
    }

    return source[current_idx + offset];
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

    char c = peek();

    // Identifier
    if(std::isalpha(c) || c == '_')
    {
        parse_ident(t);
        return t;
    }

    // Could be int, float, hex or bin literal
    if(std::isdigit(c))
    {       
        if(c == '0' && !at_eof())
        {
            t.text.push_back(c);
            advance();
            handle_unexpected_eof();
            c = peek();

            // Hex literal
            if(c == 'x')
            {
                t.text.push_back(c);
                advance(); // Consume 'x'
                handle_unexpected_eof();
                parse_hex_literal(t);
                return t;
            }

            // Binary literal
            else if(c == 'b')
            {
                t.text.push_back(c);
                advance(); // Consume 'b'
                handle_unexpected_eof();
                parse_bin_literal(t);
                return t;
            }
        }

        parse_int_or_flt_literal(t);
        return t;
    }

    // String literal
    if(c == '"')
    {
        advance(); // Consume " 
        handle_unexpected_eof();
        parse_str_literal(t);
        return t;
    }

    // Char literal
    if(c == '\'')
    {
        advance(); // Consume '
        handle_unexpected_eof();
        parse_char_literal(t);
        return t;
    }

    parse_non_ident_or_number(t);

    return t; 
}


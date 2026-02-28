// Lexer.cpp


#include "Lexer.hpp"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cctype>

/**
 * while not EOF:
 * skip whitespace + comments
 * look at current char
 * decide which token rule applies
 * consume characters
 * emit token
 */


static const std::unordered_map<std::string, TokenType> readable_to_tok_type
{
    {"fn", KW_FN},
    {"component", KW_COMPONENT},
    {"namespace", KW_NAMESPACE},
    {"pub", KW_PUB},
    {"mut", KW_MUT},
    {"ref", KW_REF},
    {"if", KW_IF},
    {"else", KW_ELSE},
    {"while", KW_WHILE}
};

static constexpr const char* token_to_readable[]
{
    "END_OF_FILE",
    "IDENTIFIER",
    "INT_LITERAL",
    "KW_FN",
    "KW_COMPONENT",
    "KW_NAMESPACE",
    "KW_PUB",
    "KW_MUT",
    "KW_REF",
    "KW_IF",
    "KW_ELSE",
    "KW_WHILE",
    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",
    "COMMA",
    "SEMICOLON",
    "DOT",
    "COLON",
    "ARROW", // '->'
    "ASSIGN", // '='
    "PLUS",
    "MINUS",
    "ASTERISK",
    "FORW_SLASH",
    "EQUAL_EQUAL" // '=='
};

// Ctors / Dtor

Lexer::Lexer() {}


// Public

void Lexer::open(const char *file_path)
{
    std::ifstream in_stream(file_path);

    if(!in_stream)
    {
        std::cerr << "Lexer failed to open file: " << file_path << '\n';
        exit(1);
    }

    std::stringstream buffer;

    buffer << in_stream.rdbuf();
    
    in_stream.close();

    source = buffer.str();

    // Perform full file lex here for debugging.
    current_idx = 0;
    current_line = 1;
    current_col = 1;

    while(!at_eof())
    {
        Token tok = next_token();

        if(tok.type == END_OF_FILE) break;

        std::cout << "-- Got token -- \ntype: " << 
            token_to_readable[tok.type] << "\nline: " << tok.line << "\ncol: "
            << tok.col << "\nlen: " << tok.len << "\ntext: " << tok.text
            << "\n\n";
    }

    std::cout << "EOF reached\n";
}


// Private

void output_invalid_char(char c, uint32_t line, uint32_t col)
{
    std::cerr << "Invalid character: " << c << " @ line: " << line << 
        " col: " << col << '\n';
}

Token Lexer::next_token() 
{
    skip_trivia();

    if(at_eof())
    {
        return make_token(END_OF_FILE, current_idx, current_line, current_col, 
            "");
    }

    char c = peek();

    if(std::isalpha(c) || c == '_')
    {
        return lex_ident_or_kword();
    }

    if(std::isdigit(c))
    {
        return lex_number();
    }

    if(
        c == '(' || 
        c == ')' ||
        c == '{' ||
        c == '}' ||
        c == '[' ||
        c == ']' ||
        c == ',' ||
        c == ';' || 
        c == '.' || 
        c == ':' || 
        c == '=' ||
        c == '+' ||
        c == '-' ||
        c == '*' ||
        c == '/')
    {
        return lex_op_or_punct();
    }

    output_invalid_char(c, current_line, current_col);
    exit(1);
}

Token Lexer::make_token(TokenType type, uint32_t start_idx, uint32_t start_line,
    uint32_t start_col, const std::string &text) const
{
    return Token
    {
        .type = type,
        .line = start_line,
        .col = start_col,
        .len = (uint32_t)(current_idx - start_idx),
        .text = text
    };
}

Token Lexer::lex_ident_or_kword() 
{
    uint32_t start_idx = current_idx;
    uint32_t start_line = current_line;
    uint32_t start_col = current_col;

    std::string parsed;

    char c = peek();

    // Parse the word
    while(std::isalpha(c) || std::isdigit(c) || c == '_')
    {
        parsed.push_back(advance());
        c = peek();
    }

    TokenType t_type;

    auto find_kw_it = readable_to_tok_type.find(parsed);

    // Parsed is a keyword
    if(find_kw_it != readable_to_tok_type.end())
    {
        t_type = find_kw_it->second;
    }
    else
    {
        t_type = IDENTIFIER;
    }

    return make_token(t_type, start_idx, start_line, start_col, parsed);
}

Token Lexer::lex_number() 
{
    uint32_t start_idx = current_idx;
    uint32_t start_line = current_line;
    uint32_t start_col = current_col;

    std::string parsed;

    char c = peek();

    while(std::isdigit(c))
    {
        parsed.push_back(advance());
        c = peek();
    }

    return make_token(INT_LITERAL, start_idx, start_line, start_col, parsed);
}

Token Lexer::lex_op_or_punct() 
{
    uint32_t start_idx = current_idx;
    uint32_t start_line = current_line;
    uint32_t start_col = current_col;

    char c = advance();

    TokenType t_type;

    if(c == '-')
    {
        // Check for "->"
        if(peek() == '>')
        {
            advance();
            t_type = ARROW;
        }
        else
        {
            t_type = MINUS;
        }
    }

    else if(c == '=')
    {
        if(peek() == '=')
        {
            advance();
            t_type = EQUAL_EQUAL;
        }
        else
        {
            t_type = ASSIGN;
        }
    }

    else
    {
        switch(c)
        {
            case '(':

                t_type = LPAREN;
                break;

            case ')':

                t_type = RPAREN;
                break;

            case '{':

                t_type = LBRACE;
                break;
        
            case '}':

                t_type = RBRACE;
                break;

            case '[':

                t_type = LBRACKET;
                break;

            case ']':

                t_type = RBRACKET;
                break;

            case ',':

                t_type = COMMA;
                break;

            case ';':

                t_type = SEMICOLON;
                break;

            case '.':

                t_type = DOT;
                break;


            case ':':

                t_type = COLON;
                break;

            case '+':

                t_type = PLUS;
                break;

            case '*':

                t_type = ASTERISK;
                break;

            case '/':

                t_type = FORW_SLASH;
                break;

            default:

                output_invalid_char(c, current_line, current_col);
                exit(1);    
        }

    }

    return make_token(t_type, start_idx, start_line, start_col, "");
}

void Lexer::skip_till_newline()
{
    char c = peek();

    while(!at_eof() && c != '\n')
    {
        advance();
        c = peek();
    }
}

void Lexer::skip_trivia() 
{   
    while(!at_eof())
    {   
        char c = peek();

        if(c == ' ' || c == '\t' || c == '\r')
        {
            advance();
            continue;
        }

        // Comment
        if(c == '/' && peek(1) == '/')
        {
            // Skip "//"
            advance();
            advance(); 
            skip_till_newline();
            if(!at_eof()) advance(); // Skip '\n
            continue;
        }

        if(c == '\n')
        {
            advance();
            continue;
        }

        break;
    }
}

bool Lexer::match(char expected) 
{
    if(expected != peek()) return false;
    
    advance();
    return true;
}

bool Lexer::at_eof() const
{
    return current_idx >= source.size();
}

char Lexer::peek(int offset) 
{
    if(current_idx + offset >= source.size()) return '\0';

    return source.at(current_idx + offset);
}

char Lexer::advance() 
{
    char c = source.at(current_idx++);

    if(c == '\n')
    {
        ++current_line;
        current_col = 1;
        return c;
    }

    ++current_col;
    return c;
}



#include "Parser.hpp"

#include<iostream>
#include <stack>

// Ctors / Dtor

Parser::Parser() {}


// Public

Program Parser::parse_program(const char *_file_path)
{
    file_path = _file_path;

    Program prog;
    prog.file_path = file_path;

    lexer.load(_file_path);
    advance(); // Get first token.

    // std::unique_ptr<Decl> decl;

    std::unique_ptr<Decl> decl;

    while ((decl = parse_top_level())) 
    {
        prog.decls.push_back(std::move(decl));
    }

    return prog;
}


// Private

void Parser::print_error_location(uint32_t line, uint32_t col) const
{
    std::cout << file_path << ":" << line << ':' << col;
}

void Parser::handle_tok_mismatch(const Token &got_token, TokenType expected) 
{
    print_error_location(got_token.line, got_token.col);
    std::cerr << ": Expect token of type: " << 
        token_to_readable[expected] << " but got token:\n" << got_token << '\n';
    exit(1);
}

void Parser::expect_scope()
{
    expect(COLON);
    expect(COLON);
}

std::unique_ptr<Decl> Parser::parse_top_level() 
{
    Token tok = peek();

    if(check(END_OF_FILE)) return nullptr;

    if(check(KW_NAMESPACE)) return parse_namespace();
    
    if(check(KW_FN)) return parse_function();

    if(consume_if(KW_TYPE))
    {
        if(check(KW_STRUCT)) return parse_struct();
        if(check(KW_COMPONENT)) return parse_component();
    
        print_error_location(peek().line, peek().col);
        std::cerr << ": \"type\" not followed by a declaration of a struct or "
            "component.\n";
        exit(1);
    }

    std::cerr << "Unexpected Token: " << tok << '\n';
    exit(1);
}

std::unique_ptr<NamespaceDecl> Parser::parse_namespace() 
{
    std::unique_ptr<NamespaceDecl> decl = std::make_unique<NamespaceDecl>();

    Token tok = expect(KW_NAMESPACE);

    decl->name = expect(IDENTIFIER).text;
    decl->line = tok.line;
    decl->col = tok.col;

    expect(LBRACE);

    while(!check(RBRACE))
    {
        if(check(END_OF_FILE))
        {
            std::cerr << "Reached end of file expecting '}' for namespace: "
                << decl->name << '\n';
            exit(1);
        }

        decl->decls.push_back(std::move(parse_top_level()));
    }

    expect(RBRACE);
    return decl;
}

std::unique_ptr<FunctionDecl> Parser::parse_function() 
{
    std::unique_ptr<FunctionDecl> decl = std::make_unique<FunctionDecl>();
    
    Token fn_tok = expect(KW_FN);
    
    decl->line = fn_tok.line;
    decl->col = fn_tok.col;
    decl->is_pub = true;

    // Publicity handled outside this call.

    // Expecting a receiver header
    if(check(LPAREN))
    {
        decl->receiver_meta.emplace();

        expect(LPAREN);

        if(!check(KW_REF))
        {
            print_error_location(peek().line, peek().col);
            std::cerr << ": Invalid function syntax. After 'fn', either write:"
            << "\n  fn <name>(...) -> <type> { ... }\n"
            << "or for a component receiver function:\n"
            << "  fn (ref [mut] <Type> <selfName>) <name>(...) -> <type> "
                "{ ... }\n";
            exit(1);
        }

        expect(KW_REF); // Consume KW_REF

        // Receiver is reference to mutable.
        if(consume_if(KW_MUT))
        {
            decl->receiver_meta->receiver_type.ref_type = ReferenceType::REFMUT;
        }
        // Reference to immutable
        else
        {
            decl->receiver_meta->receiver_type.ref_type = ReferenceType::REF;
        }

        if (check(ASTERISK))
        {
            print_error_location(peek().line, peek().col);
            std::cerr << ": Pointer receiver types are not allowed. Use "
                "obj_ptr->method(), which desugars to (*obj_ptr).method().\n";
            exit(1);
        }

        // Get type name
        decl->receiver_meta->receiver_type.name = expect(IDENTIFIER).text;

        while(check(COLON))
        {
            expect_scope();
            decl->receiver_meta->receiver_type.name += "::" +
                expect(IDENTIFIER).text;
        }

        // Get instance name
        decl->receiver_meta->receiver_name = expect(IDENTIFIER).text;

        expect(RPAREN);
    }

    decl->name = expect(IDENTIFIER).text;
    
    expect(LPAREN);

    uint32_t paren_depth = 1;
    while (paren_depth > 0)
    {
        if (check(END_OF_FILE))
        {
            std::cerr << "Reached end of file expecting ')' in parameter list "
                    << "for function: " << decl->name << '\n';
            exit(1);
        }

        if (check(LPAREN)) ++paren_depth;
        else if (check(RPAREN)) --paren_depth;

        advance(); // consumes final RPAREN when depth hits 0
    }

    expect(ARROW);

    decl->return_type = parse_type_ref();

    std::stack<uint32_t> lbrace_lines;
    
    Token tok = expect(LBRACE);
    decl->body.start_idx = tok.start_idx;

    lbrace_lines.push(tok.line);

    while(lbrace_lines.size() > 0)
    {
        if(check(END_OF_FILE))
        {
            std::cerr << "Reached end of file expecting '}' for '{' on line: "
                << lbrace_lines.top() << '\n';
            exit(1);
        }

        if(check(LBRACE)) lbrace_lines.push(peek().line);
        else if(check(RBRACE)) lbrace_lines.pop();

        tok = peek();
        advance();
    }

    decl->body.end_idx = tok.start_idx + tok.len;

    return decl;
}

std::unique_ptr<StructDecl> Parser::parse_struct() 
{
    std::unique_ptr<StructDecl> decl = std::make_unique<StructDecl>();

    Token tok = expect(KW_STRUCT);

    decl->name = expect(IDENTIFIER).text;

    decl->line = tok.line;
    decl->col = tok.col;

    expect(LBRACE);

    while(!check(RBRACE))
    {
        if(check(END_OF_FILE))
        {
            std::cout << "Reached end of file expecting '}' for struct: "
                << decl->name << '\n';
            exit(1);
        }

        decl->fields.push_back(parse_field());
    }

    expect(RBRACE);

    return decl;
}

std::unique_ptr<ComponentDecl> Parser::parse_component() 
{
    std::unique_ptr<ComponentDecl> decl = std::make_unique<ComponentDecl>();

    Token tok = expect(KW_COMPONENT);

    decl->name = expect(IDENTIFIER).text;

    decl->line = tok.line;
    decl->col = tok.col;

    expect(LBRACE);

    while(!check(RBRACE))
    {
        if(check(END_OF_FILE))
        {
            std::cout << "Reached end of file expecting '}' for component: "
                << decl->name << '\n';
            exit(1);
        }

        bool is_pub = consume_if(KW_PUB);
        
        if(check(KW_FN))
        {
            std::unique_ptr<FunctionDecl> func = parse_function();
            func->is_pub = is_pub;
            decl->functions.push_back(std::move(func));
        }
        
        else if(check(KW_MUT) || check(KW_REF) || check(ASTERISK) || 
            check(IDENTIFIER))
        {
            Field field = parse_field();
            field.is_pub = is_pub;
            decl->fields.push_back(field);
        }

        else if(consume_if(KW_TYPE))
        {
            NestedContainer nc;
            nc.is_pub = is_pub;

            if(check(KW_STRUCT)) 
            {
                nc.decl = parse_struct();
            }

            else if(check(KW_COMPONENT))
            {
                nc.decl = parse_component();
            }

            else
            {
                print_error_location(peek().line, peek().col);
                std::cerr << ": \"type\" not followed by a declaration of a "
                    "struct or component.\n";
                exit(1);
            }

            decl->nested_cnts.push_back(std::move(nc));
        }

        else
        {
            print_error_location(peek().line, peek().col);
            std::cerr << ": Invalid token in component body: \n" << peek() 
                << "\n";
            exit(1);
        }
    }

    expect(RBRACE);

    return decl;
}

Field Parser::parse_field()
{
    bool is_mut = consume_if(KW_MUT);

    TypeRef t = parse_type_ref();

    Token tok = expect(IDENTIFIER);

    std::string name = tok.text;

    if(!check(SEMICOLON))
    {
        print_error_location(tok.line, tok.col + 1);
        std::cerr << ": Missing ';'\n";
        exit(1);
    }

    expect(SEMICOLON);

    Field f;

    f.is_mut = is_mut;
    f.is_pub = true;
    f.t = std::move(t);
    f.name = std::move(name);

    return f;
}

TypeRef Parser::parse_type_ref() 
{
    TypeRef tr;
    
    if(consume_if(KW_REF))
    {
        if(consume_if(KW_MUT))
        {
            tr.ref_type = ReferenceType::REFMUT;
        }
        else
        {
            tr.ref_type = ReferenceType::REF;
        }
    }

    else tr.ref_type = ReferenceType::NONE;

    while(consume_if(ASTERISK))
    {
        tr.ptr_pointee_mut.push_back(consume_if(KW_MUT));
    }

    tr.name = expect(IDENTIFIER).text;

    // Check if the name identifier is a namespace qualified names
    while(check(COLON))
    {
        expect_scope();

        tr.name += "::";
        tr.name += expect(IDENTIFIER).text;
    }

    return tr;
}

bool Parser::check(TokenType t) const 
{
    return peek().type == t;
}

bool Parser::consume_if(TokenType t)
{
    if(peek().type != t) return false;
    advance();
    return true;
}

Token Parser::expect(TokenType t) 
{
    const Token tok = peek();

    if(tok.type != t)
    {
        handle_tok_mismatch(tok, t);
    } 

    advance();
    return tok;
}

const Token& Parser::peek() const 
{
    return current_token;
}

// Returns current token AFTER ADVANCING.
Token Parser::advance() 
{
    current_token = lexer.next_token();
    return current_token;
}

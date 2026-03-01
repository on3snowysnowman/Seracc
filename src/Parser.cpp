
#include "Parser.hpp"

#include<iostream>
#include <stack>

// Ctors / Dtor

Parser::Parser() {}


// Public

Program Parser::parse_program(const char *file_path)
{
    Program prog;

    lexer.load(file_path);
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

static void print_line_and_col(uint32_t line, uint32_t col)
{
    std::cout << "Line: " << line << "\nCol: " << col << '\n';
}

void Parser::handle_tok_mismatch(const Token &got_token, TokenType expected) 
{
    std::cerr << "Expect token of type: " << 
        token_to_readable[expected] << " but got token:\n" << got_token << '\n';
    print_line_and_col(got_token.line, got_token.col);
    exit(1);
}

std::unique_ptr<Decl> Parser::parse_top_level() 
{
    const Token &tok = peek();

    if(tok.type == END_OF_FILE) return nullptr;

    if(tok.type == KW_NAMESPACE) return parse_namespace();
    
    if(tok.type == KW_FN) return parse_function();

    if(tok.type == KW_TYPE)
    {
        advance(); // Consume KW_TYPE token.

        if(consume_if(KW_STRUCT)) return parse_struct();
        if(consume_if(KW_COMPONENT)) return parse_component();
    
        std::cerr << "\"type\" not followed by a declaration of a struct or "
            "component.\n";
        print_line_and_col(tok.line, tok.col);
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

    tok = expect(LBRACE);

    uint32_t brace_depth = 1;

    // Store the line position of each LBRACE encountered so we can produce a 
    // helpful error when a matching RBRACE is not found.
    std::stack<uint32_t> lbrace_line_positions;

    while(brace_depth > 0)
    {
        if(check(END_OF_FILE))
        {
            std::cout << "Reached end of file expecting '}' for '{' on line: "
                << lbrace_line_positions.top() << '\n';
            exit(1);
        }

        if(check(LBRACE))
        {
            lbrace_line_positions.push(peek().line);
            ++brace_depth;
        } 
        if(check(RBRACE)) 
        {
            lbrace_line_positions.pop();
            --brace_depth;
        }

        advance(); // Consumes final RBRACE when depth hits 0.
    }

    return decl;
}

std::unique_ptr<FunctionDecl> Parser::parse_function() 
{
    std::unique_ptr<FunctionDecl> decl = std::make_unique<FunctionDecl>();

    decl->receiver_meta.emplace();

    // Publicity handled outside this call.

    expect(KW_FN);

    // Expecting a receiver header
    if(check(LPAREN))
    {
        advance();

        if(!check(KW_REF))
        {
            std::cerr << "Receiver parameter must always be marked with \"ref\""
                "\n";
            print_line_and_col(peek().line, peek().col);
            exit(1);
        }

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

        // Get type name
        decl->receiver_meta->receiver_type.name = expect(IDENTIFIER).text;

        // Get instance name
        decl->receiver_meta->receiver_name = expect(IDENTIFIER).text;

        expect(RPAREN);
    }

    decl->name = expect(IDENTIFIER).text;
    expect(LPAREN);

    // Parse params
    while(!check(RPAREN))
    {
        advance(); // Consume RPAREN

        if(check(END_OF_FILE))
        {
            // Don't need a super helpful error here since we're only 
            // temporarily not parsing the function list.
            std::cerr << "Reached end of file before finishing parameter list."
                "\n";
        }

        // Skip param parsing for now.
        // decl->parameters.push_back(parse_param());
        // expect(COMMA);
    }

    expect(RPAREN);
    expect(ARROW);

    decl->return_type = parse_type_ref();

    // Store the Token of each lbrace encountered so we can provided an error 
    // message if a matching rbrace is not found, as well as track the character
    // span of each BlockStub.
    std::stack<Token> lbrace_lines;

    // Stack of stubs. Top stub is most recently encountered.
    std::stack<BlockStub> stubs;

    // Push 
    lbrace_lines.push(expect(LBRACE));

    // Add the initial block stub 
    BlockStub stub;
    stub.start_idx = lbrace_lines.top().start_idx;

    stubs.push(stub);

    while(lbrace_lines.size() > 0)
    {
        // We're closing a block stub. 
        if(check(RBRACE)) 
        {
            lbrace_lines.pop();

            // Get the block stub this RBRACE is closing.
            BlockStub stub = stubs.top();
            stubs.pop();

            // The end of this block stub is the starting index of the current
            // RBRACE token.
            stub.end_idx = peek().start_idx;

            // Add the BlockStub to the stubs of this function.
            decl->stubs.push_back(std::move(stub));

            advance(); // Consume RBRACE
        }
        // Opening a block stub.
        else if(check(LBRACE)) 
        {
            // Create a new block stub.
            BlockStub stub;
            // Stub's starting index is the starting index of this LBRACE token.
            stub.start_idx = peek().start_idx;
            stubs.push(std::move(stub));

            // Add a new LBRACE to the stack.
            lbrace_lines.push(peek());
            advance(); // Consume LBRACE
        }

        // parse_block_stub does not consume RBRACE so it can be handled on the 
        // next loop iteration.
        parse_block_stub(stubs.top());
    }

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
        
        // Must be a field.
        else
        {
            Field field = parse_field();
            field.is_pub = is_pub;
            decl->fields.push_back(field);
        }
    }

    expect(RBRACE);

    return decl;
}

Field Parser::parse_field()
{
    bool is_mut = consume_if(KW_MUT);

    TypeRef t = parse_type_ref();

    std::string name = expect(IDENTIFIER).text;
    expect(SEMICOLON);

    return Field 
    {
        .is_mut = is_mut,
        .t = t,
        .name = name,
        .is_pub{}
    };
}

Param Parser::parse_param()
{
    Param p;

    // Parameter is an unqualified parameter
    if(check(IDENTIFIER))
    {
        p.is_unqual_param = true;
        p.t.name = expect(IDENTIFIER).text; // Type name
        p.name = expect(IDENTIFIER).text; // Parameter name
        return p;
    }

    p.is_unqual_param = false;
    p.is_mut = consume_if(KW_MUT);
    p.t = parse_type_ref();
    p.name = expect(IDENTIFIER).text;
    return p;
}

void Parser::parse_block_stub(BlockStub &stub)
{

}

TypeRef Parser::parse_type_ref() 
{
    TypeRef tr;
    tr.ref_type = ReferenceType::NONE;
    
    if(consume_if(KW_REF))
    {
        tr.ref_type = ReferenceType::REF;
    }

    else if(consume_if(KW_MUT))
    {
        expect(KW_REF);

        tr.ref_type = ReferenceType::REFMUT;
    }

    else tr.ref_type = ReferenceType::NONE;

    while(consume_if(ASTERISK))
    {
        tr.ptr_pointee_mut.push_back(consume_if(KW_MUT));
    }

    tr.name = expect(IDENTIFIER).text;

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

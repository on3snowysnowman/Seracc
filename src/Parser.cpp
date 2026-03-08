// Parser.cpp

#include "Parser.hpp"


// Ctors / Dtor

Parser::Parser() {}


// Public

void Parser::print_error_location(uint32_t line, uint32_t col) const
{
    std::cerr << parsed_file << ":" << line << ':' << col;
}

void Parser::handle_tok_mismatch(const Token &got_tok, TokenID expected) 
{
    print_error_location(got_tok.line, got_tok.col);
    std::cerr << ": Expect token of type: " << 
        tokID_readable[static_cast<int>(expected)] << " but got token:\n" << 
        got_tok << '\n';
    exit(1);
}

void Parser::handle_unexpected_token(const Token &got_tok)
{
    print_error_location(got_tok.line, got_tok.col);
    std::cerr << ": Unexpected token:\n" << got_tok;
    exit(1);
}

Program Parser::parse(const char *in_file_path) 
{
    Lexer l;

    tokens = l.lex(in_file_path);

    // lexer.load(in_file_path);

    Program prog;
    prog.source_file_name = in_file_path;
    parsed_file = in_file_path;

    advance(); // Get first token.

    prog.ast->name = "global";
    prog.ast->line = 0;
    prog.ast->col = 0;

    std::unique_ptr<Declaration> decl = parse_top_level();

    while(decl)
    {
        prog.ast->decls.push_back(std::move(decl));
        decl = parse_top_level();
    }


    // prog.ast = std::move(parse_top_level());

    // lexer.close();
    parsed_file = nullptr;

    return prog;
}


// Private

std::unique_ptr<Declaration> Parser::parse_top_level() 
{
    Token t = peek();

    bool is_exported = false;

    if(consume_if(TokenID::KW_EXPORT)) is_exported = true;

    if(check(TokenID::END_OF_FILE)) return nullptr;

    if(check(TokenID::KW_NAMESPACE)) return parse_namespace();

    if(check(TokenID::KW_FN)) return parse_function(is_exported);

    if(consume_if(TokenID::KW_TYPE)) 
    {
        if(check(TokenID::KW_STRUCT)) return parse_struct(is_exported);
        if(check(TokenID::KW_COMPONENT)) return parse_component(is_exported);
        
        print_error_location(peek().line, peek().col);
        std::cerr << ": \"type\" not followed by a declaration of a struct or "
            "component.\n";
        exit(1);
    }

    // Global variable
    if(check(TokenID::IDENTIFIER)) return parse_field(is_exported);

    handle_unexpected_token(t);
    
    return nullptr;
}

std::unique_ptr<NamespaceDecl> Parser::parse_namespace() 
{
    std::unique_ptr<NamespaceDecl> ptr = std::make_unique<NamespaceDecl>();

    ptr->line = peek().line;
    ptr->col = peek().col;

    expect(TokenID::KW_NAMESPACE);
    
    ptr->name = expect(TokenID::IDENTIFIER).text;

    expect(TokenID::LBRACE);

    while(!check(TokenID::RBRACE))
    {
        std::unique_ptr<Declaration> decl = parse_top_level();

        if(!decl)
        {
            std::cerr << parsed_file << ": Reached end of file expecting '}' "
                "for namespace: \"" << ptr->name << "\" found here: ";
                print_error_location(ptr->line, ptr->col);
                std::cout << '\n';
            exit(1);
        }

        ptr->decls.push_back(std::move(decl));
    }

    expect(TokenID::RBRACE);
    return ptr;
}

std::unique_ptr<FunctionDecl> Parser::parse_function(bool is_pub) 
{
    std::unique_ptr<FunctionDecl> ptr = std::make_unique<FunctionDecl>();

    ptr->is_pub = is_pub;
    ptr->line = peek().line;
    ptr->col = peek().col;

    expect(TokenID::KW_FN);

    ptr->name = expect(TokenID::IDENTIFIER).text;

    expect(TokenID::LPAREN);

    // Parse parameters
    while(!check(TokenID::RPAREN))
    {
        ptr->params.push_back(parse_param());

        // Trailing comma
        if(consume_if(TokenID::COMMA) && check(TokenID::RPAREN))
        {
            print_error_location(peek().line, peek().col);
            std::cout << ": Trailing comma in parameter list\n";
            exit(1);
        }
    }

    expect(TokenID::RPAREN);

    expect(TokenID::ARROW);

    // Read return type
    ptr->ret_type = parse_type_decl();

    ptr->body = parse_scope();

    return ptr;
}

std::unique_ptr<StructDecl> Parser::parse_struct(bool is_pub) 
{
    std::unique_ptr<StructDecl> ptr = std::make_unique<StructDecl>();

    ptr->is_pub = is_pub;
    ptr->line = peek().line;
    ptr->col = peek().col;

    expect(TokenID::KW_STRUCT);
    
    ptr->name = expect(TokenID::IDENTIFIER).text;

    const auto it = defined_types.find(ptr->name);

    // This struct has been defined already been defined
    if(it != defined_types.end())
    {
        print_error_location(ptr->line, ptr->col);
        std::cerr << ": Type \"" << ptr->name << "\" has already been defined "
            "here: " << it->second.file_defined << ":" << it->second.line << 
            ":" << it->second.col << '\n';
        exit(1);
    }

    else
    {
        std::cerr << "Defined type: " << ptr->name << '\n';
        defined_types.insert({ptr->name, {ptr->line, ptr->col, parsed_file}});
    }

    expect(TokenID::LBRACE);

    // Parse struct body.
    while(!check(TokenID::RBRACE))
    {
        // Parsing a nested struct
        if(consume_if(TokenID::KW_TYPE))
        {
            ptr->decls.push_back(parse_struct(true));
            continue;
        }

        // Parsing a field
        ptr->decls.push_back(parse_field(true));
    }

    expect(TokenID::RBRACE);

    return ptr;
}

std::unique_ptr<ComponentDecl> Parser::parse_component(bool is_pub) 
{
    (void)is_pub;
    return nullptr;
}

std::unique_ptr<Statement> Parser::parse_statement() 
{
    std::unique_ptr<Statement> ptr = nullptr;

    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    // Variable declaration
    if(consume_if(TokenID::KW_LET))
    {
        ptr = std::make_unique<VarDeclStmt>();
        ptr->stmt_type = StatementType::VAR_DECL;
        VarDeclStmt *reint_ptr = 
            reinterpret_cast<VarDeclStmt*>(ptr.get());

        reint_ptr->is_binding_mutable = consume_if(TokenID::KW_MUT);
        reint_ptr->var_name = expect(TokenID::IDENTIFIER).text;

        expect(TokenID::COLON);

        reint_ptr->type_decl = parse_type_decl();

        // Variable has assignment expression
        if(consume_if(TokenID::ASSIGN))
        {
            reint_ptr->init_expr = parse_expression();
        }
    }

    // Container declaration
    else if(consume_if(TokenID::KW_TYPE))
    {
        if(check(TokenID::KW_STRUCT))
        {
            ptr = std::make_unique<StructDeclStmt>();
            ptr->stmt_type = StatementType::STRUCT_DECL;
            StructDeclStmt *reint_ptr = 
                reinterpret_cast<StructDeclStmt*>(ptr.get());

            reint_ptr->decl = parse_struct(true);
        }

        else if(check(TokenID::KW_COMPONENT))
        {
            ptr = std::make_unique<ComponentDeclStmt>();
            ptr->stmt_type = StatementType::COMPONENT_DECL;
            ComponentDeclStmt *reint_ptr = 
                reinterpret_cast<ComponentDeclStmt*>(ptr.get());

            reint_ptr->decl = parse_component(true);
        }

        else
        {
            print_error_location(peek().line, peek().col);
            std::cerr << ": \"type\" not followed by a declaration of a struct or "
                "component.\n";
            exit(1);
        }
    }

    // Return statement
    else if(consume_if(TokenID::KW_RET))
    {
        ptr = std::make_unique<RetStmt>();
        ptr->stmt_type = StatementType::RETURN;

        RetStmt *reint_ptr = 
                reinterpret_cast<RetStmt*>(ptr.get());
    
        reint_ptr->ret_expr = parse_expression();
    }

    else if(consume_if(TokenID::KW_IF))
    {

    }

    else if(consume_if(TokenID::KW_WHILE))
    {

    }

    else if(consume_if(TokenID::KW_FOR))
    {

    }

    // Block Declaration
    else if(check(TokenID::LBRACE))
    {
        ptr = std::make_unique<BlockStmt>();
        ptr->stmt_type = StatementType::BLOCK;

        BlockStmt *reint_ptr = 
            reinterpret_cast<BlockStmt*>(ptr.get());

        reint_ptr->block_decl = parse_scope();
    }

    else
    {
        print_error_location(peek().line, peek().col);
        std::cerr << ": Expecting statement but got token: \n" << 
            peek() << '\n';
        exit(1);
    }

    expect(TokenID::SEMICOLON);

    ptr->line = start_line;
    ptr->col = start_col;

    return ptr;
}

std::unique_ptr<Expression> Parser::parse_asterisk_expression()
{
    // Expression could be deref or a type identifier.
    std::unique_ptr<Expression> ptr = nullptr; 

    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    // Token starts on the asterisk.

    // Save this token position so we can return back.
    uint64_t saved_token_idx = current_token_idx;

    expect(TokenID::ASTERISK);
    
    // If there is a parenthesis right after the asterisk, it must be a deref.
    if(consume_if(TokenID::LPAREN))
    {
        ptr = std::make_unique<UnaryExpr>();
        ptr->exp_type = ExpressionType::UNARY;

        UnaryExpr * const reint_ptr = reinterpret_cast<UnaryExpr*>(ptr.get());

        reint_ptr->op_type = UnaryOp::DEREF;

        // Get the expression inside the parenthesis that is being dereferenced.
        reint_ptr->operand = parse_expression();
        expect(TokenID::RBRACE);

        ptr->line = start_line;
        ptr->col = start_col;
        return ptr;
    }

    // There is no parenthesis right after the asterisk, so we're either dealing
    // a type declaration which would be a cast or derefencing a variable.

    
}

std::unique_ptr<Expression> Parser::parse_parenth_expression()
{
    // Save the index of the current token so we can return here after deducing
    // what the expression inside the parenthesis is.
    uint64_t saved_token_idx = current_token_idx;

    // Parse the expression inside the parenthesis
    std::unique_ptr<Expression> got_expression = parse_expression();

    if(got_expression->exp_type == ExpressionType::IDENTIFIER)
    {
        // Check if the identifier is 
    }
}

std::unique_ptr<Expression> Parser::parse_expression() 
{
    std::unique_ptr<Expression> ptr = nullptr;

    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    if(check(TokenID::INT_LITERAL))
    {
        ptr = std::make_unique<IntLitExpr>();
        ptr->exp_type = ExpressionType::INT_LITERAL;
        
        IntLitExpr *reint_ptr = reinterpret_cast<IntLitExpr*>(ptr.get());

        reint_ptr->value = peek().text;
    }

    else if(check(TokenID::FLOAT_LITERAL))
    {
        ptr = std::make_unique<FloatLitExpr>();
        ptr->exp_type = ExpressionType::FLOAT_LITERAL;
        
        FloatLitExpr *reint_ptr = reinterpret_cast<FloatLitExpr*>(ptr.get());

        reint_ptr->value = peek().text;
    }

    else if(check(TokenID::HEX_LITERAL))
    {
        ptr = std::make_unique<HexLitExpr>();
        ptr->exp_type = ExpressionType::HEX_LITERAL;
        
        HexLitExpr *reint_ptr = reinterpret_cast<HexLitExpr*>(ptr.get());

        reint_ptr->value = peek().text;
    }

    else if(check(TokenID::BIN_LITERAL))
    {
        ptr = std::make_unique<BinaryLitExpr>();
        ptr->exp_type = ExpressionType::BIN_LITERAL;
        
        BinaryLitExpr *reint_ptr = reinterpret_cast<BinaryLitExpr*>(ptr.get());

        reint_ptr->value = peek().text;
    }

    else if(check(TokenID::STR_LITERAL))
    {
        ptr = std::make_unique<StringLitExpr>();
        ptr->exp_type = ExpressionType::STR_LITERAL;
        
        StringLitExpr *reint_ptr = reinterpret_cast<StringLitExpr*>(ptr.get());

        reint_ptr->value = peek().text;
    }

    else if(check(TokenID::IDENTIFIER))
    {
        // Could be many operations, or just a identifier expression.
        
    }

    // Prefix increment. Unary + is not supported.
    else if(check(TokenID::PLUS))
    {
        expect(TokenID::PLUS);
        
        ptr = std::make_unique<UnaryExpr>();
        ptr->exp_type = ExpressionType::UNARY;

        UnaryExpr *reint_ptr = reinterpret_cast<UnaryExpr*>(ptr.get());

        reint_ptr->op_type = UnaryOp::PRE_INC;
        reint_ptr->operand = parse_expression();
    }

    // Could be prefix decrement or negation 
    else if(consume_if(TokenID::MINUS))
    {
        ptr = std::make_unique<UnaryExpr>();
        ptr->exp_type = ExpressionType::UNARY;

        UnaryExpr *reint_ptr = reinterpret_cast<UnaryExpr*>(ptr.get());

        // Prefix decrement
        if(check(TokenID::MINUS))
        {
            reint_ptr->op_type = UnaryOp::PRE_DEC;
            reint_ptr->operand = parse_expression();
        }

        // Negation
        else if(check(TokenID::IDENTIFIER))
        {
            reint_ptr->op_type = UnaryOp::NEGATE;
            reint_ptr->operand = parse_expression();
        }

        else handle_unexpected_token(peek());
    }

    // Logical Not
    else if(consume_if(TokenID::EXCLAMATION_POINT))
    {
        ptr = std::make_unique<UnaryExpr>();
        ptr->exp_type = ExpressionType::UNARY;

        UnaryExpr *reint_ptr = reinterpret_cast<UnaryExpr*>(ptr.get());
        
        reint_ptr->op_type = UnaryOp::LOG_NOT;
        reint_ptr->operand = parse_expression();
    }

    // Bitwise Not
    else if(consume_if(TokenID::TILDE))
    {
        ptr = std::make_unique<UnaryExpr>();
        ptr->exp_type = ExpressionType::UNARY;

        UnaryExpr *reint_ptr = reinterpret_cast<UnaryExpr*>(ptr.get());

        reint_ptr->op_type = UnaryOp::BIT_NOT;
        reint_ptr->operand = parse_expression();
    }

    // Address of
    else if(consume_if(TokenID::AMPERSAND))
    {
        ptr = std::make_unique<UnaryExpr>();
        ptr->exp_type = ExpressionType::UNARY;

        UnaryExpr *reint_ptr = reinterpret_cast<UnaryExpr*>(ptr.get());

        reint_ptr->op_type = UnaryOp::ADDRESS_OF;
        reint_ptr->operand = parse_expression();
    }

    // Could be deref or a type identifier 
    else if(consume_if(TokenID::ASTERISK))
    {
        ptr = parse_asterisk_expression();

        // ptr = std::make_unique<UnaryExpr>();
        // ptr->exp_type = ExpressionType::UNARY;

        // UnaryExpr *reint_ptr = reinterpret_cast<UnaryExpr*>(ptr.get());

        // reint_ptr->op_type = UnaryOp::DEREF;
        // reint_ptr->operand = parse_expression();
    }

    // Parenthesized expression
    else if(consume_if(TokenID::LPAREN))
    { 
        ptr = parse_parenth_expression();
    }   

    else
    {
        print_error_location(peek().line, peek().col);
        std::cerr << ": Expecting expression but got token: \n" << 
            peek() << '\n';
        exit(1);
    }

    ptr->line = start_line;
    ptr->col = start_col;

    return nullptr;
}

std::unique_ptr<TypeDecl> Parser::parse_type_decl() 
{
    std::unique_ptr<TypeDecl> ptr = nullptr;
    
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    // Named 
    if(check(TokenID::IDENTIFIER))
    {
        ptr = std::make_unique<NamedTypeDecl>();
        ptr->kind = TypeKind::NAMED;
        NamedTypeDecl *reint_ptr = reinterpret_cast<NamedTypeDecl*>(ptr.get());

        reint_ptr->type_name = expect(TokenID::IDENTIFIER).text;        
    }

    // Pointer 
    else if(consume_if(TokenID::ASTERISK))
    {
        ptr = std::make_unique<PtrTypeDecl>();
        ptr->kind = TypeKind::PTR;
        PtrTypeDecl *reint_ptr = reinterpret_cast<PtrTypeDecl*>(ptr.get());

        reint_ptr->points_to_mutable = consume_if(TokenID::KW_MUT);
        reint_ptr->pointee = parse_type_decl();
    }

    // Reference
    else if(consume_if(TokenID::KW_REF))
    {   
        ptr = std::make_unique<RefTypeDecl>();
        ptr->kind = TypeKind::REF;
        RefTypeDecl *reint_ptr = reinterpret_cast<RefTypeDecl*>(ptr.get());

        reint_ptr->ref_to_mutable = consume_if(TokenID::KW_MUT);
        reint_ptr->referred = parse_type_decl();
    }

    // Function pointer
    else if(consume_if(TokenID::KW_FN))
    {
        ptr = std::make_unique<FuncPtrDecl>();
        ptr->kind = TypeKind::FUNC_PTR;
        FuncPtrDecl *reint_ptr = reinterpret_cast<FuncPtrDecl*>(ptr.get());

        expect(TokenID::LPAREN);

        expect(TokenID::LPAREN);

        // Parse parameters
        while(true)
        {
            reint_ptr->param_types.push_back(parse_param());

            // If the next token is not a comma, it must be a right paren
            if(!check(TokenID::COMMA))
            {
                expect(TokenID::RPAREN);
                break;
            }

            // Comma present, read in next parameter
            expect(TokenID::COMMA);
        }

        expect(TokenID::ARROW);

        // Read return type
        reint_ptr->ret_type = parse_type_decl();
    }

    else
    {
        print_error_location(peek().line, peek().col);
        std::cerr << ": Expecting type declaration but got token: \n" << peek() 
            << '\n';
        exit(1);
    }

    ptr->line = start_line;
    ptr->col = start_col;

    // Check if after parsing the type, if this is an array of that type.
    if(check(TokenID::LBRACKET))
    {
        std::unique_ptr<ArrTypeDecl> arr_decl = std::make_unique<ArrTypeDecl>();
    
        arr_decl->kind = TypeKind::ARRAY;

        arr_decl->line = ptr->line;
        arr_decl->col = ptr->col;
        arr_decl->depth = 1;

        // The parsed type now becomes the type of the array.
        arr_decl->element_type = std::move(ptr);
    
        // Get the expression of the first array.
        arr_decl->size_exprs.push_back(parse_expression());

        expect(TokenID::RBRACKET);

        // Iterate through any further depths of the array.
        while(check(TokenID::LBRACKET))
        {
            ++arr_decl->depth;
            arr_decl->size_exprs.push_back(parse_expression());
            expect(TokenID::RBRACKET);
        }

        return arr_decl; // Return our array declaration, not the main 'ptr' obj
    }

    return ptr; // Return nullptr for now until complete
}

std::unique_ptr<FieldDecl> Parser::parse_field(bool is_pub) 
{
    (void)is_pub;
    return nullptr;
}

ScopeBody Parser::parse_scope()
{
    ScopeBody body;

    body.line = peek().line;
    body.col = peek().col;

    expect(TokenID::LBRACE);

    while(!check(TokenID::RBRACE))
    {
        body.statements.push_back(parse_statement());
    }

    expect(TokenID::RBRACE);

    return body;
}

Parameter Parser::parse_param() 
{
    Parameter p;

    p.line = peek().line;
    p.col = peek().col;
    p.is_binding_mutable = false;

    // If this is an unqualified parameter
    if(check(TokenID::IDENTIFIER))
    {
        p.is_unqual_param = true;
    }

    else if(consume_if(TokenID::KW_MUT))
    {
        // If this parameter is marked mut, we need to make sure that it is also
        // marked val.
        if(!check(TokenID::KW_VAL))
        {
            print_error_location(peek().line, peek().col);
            std::cerr << "Non ref parameter marked mut but not passed in by "
                "val.\n";
            exit(1);
        }

        p.is_binding_mutable = true;
    }

    p.type_decl = parse_type_decl();

    return p;
}

bool Parser::check(TokenID id) const 
{
    return peek().id == id;
}

bool Parser::consume_if(TokenID id) 
{
    if(peek().id != id) return false;
    advance();
    return true;
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
    return tokens.at(current_token_idx);
}

Token Parser::advance() 
{
    Token tok = peek();

    // current_token = lexer.next_token();
    ++current_token_idx;
    return tok;
}

void Parser::rewind(uint64_t token_idx)
{
    current_token_idx = token_idx;
}

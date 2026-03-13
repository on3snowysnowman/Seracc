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
    current_token_idx = 0;

    Lexer l;

    tokens = l.lex(in_file_path);

    // lexer.load(in_file_path);

    Program prog;
    prog.source_file_name = in_file_path;
    parsed_file = in_file_path;

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

    if(check(TokenID::KW_NAMESPACE)) 
    {
        if(is_exported)
        {
            std::cerr << "Namespaces cannot be marked \"export\"\n";
            exit(1);
        }

        return parse_namespace();
    }

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
                std::cerr << '\n';
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

    // This function has a receiver parameter
    if(consume_if(TokenID::LPAREN))
    {
        ReceiverData r_data;
        r_data.receiver_name = expect(TokenID::IDENTIFIER).text;
        expect(TokenID::COLON);
        r_data.receiver_type_decl = parse_type_decl();

        ptr->receiver_data.emplace(std::move(r_data));

        expect(TokenID::RPAREN);
    }

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
            std::cerr << ": Trailing comma in parameter list\n";
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

    // This struct has already been defined
    if(it != defined_types.end())
    {
        print_error_location(ptr->line, ptr->col);
        std::cerr << ": Type \"" << ptr->name << "\" has already been defined "
            "here: " << it->second.file_defined << ":" << it->second.line << 
            ":" << it->second.col << '\n';
        exit(1);
    }

    // Otherwise, register this struct type as defined.
    else defined_types.insert({ptr->name, {ptr->line, ptr->col, parsed_file}});

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
    std::unique_ptr<ComponentDecl> ptr = std::make_unique<ComponentDecl>();

    ptr->is_pub = is_pub;
    ptr->line = peek().line;
    ptr->col = peek().col;

    expect(TokenID::KW_COMPONENT);

    ptr->name = expect(TokenID::IDENTIFIER).text;

    const auto it = defined_types.find(ptr->name);

    // This component has already been defined
    if(it != defined_types.end())
    {
        print_error_location(ptr->line, ptr->col);
        std::cerr << ": Type \"" << ptr->name << "\" has already been defined "
            "here: " << it->second.file_defined << ":" << it->second.line << 
            ":" << it->second.col << '\n';
        exit(1);
    }

        // Otherwise, register this struct type as defined.
    else defined_types.insert({ptr->name, {ptr->line, ptr->col, parsed_file}});

    expect(TokenID::LBRACE);

    // Parse component body
    while(!check(TokenID::RBRACE))
    {
        bool is_member_pub = consume_if(TokenID::KW_PUB);

        // Parsing a nested container
        if(consume_if(TokenID::KW_TYPE))
        {
            if(check(TokenID::KW_STRUCT)) 
                ptr->decls.push_back(parse_struct(is_member_pub));
            
            else if(check(TokenID::KW_COMPONENT))
                ptr->decls.push_back(parse_component(is_member_pub));

            else
            {
                print_error_location(peek().line, peek().col);
                std::cerr << ": \"type\" not followed by a declaration of a "
                    "struct or component.\n";
                exit(1);
            }
        }

        else if(check(TokenID::KW_FN)) 
            ptr->decls.push_back(parse_function(is_member_pub));

        else
            ptr->decls.push_back(parse_field(is_member_pub));
    }

    expect(TokenID::RBRACE);

    return ptr;
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
        VarDeclStmt *reint_ptr = 
            static_cast<VarDeclStmt*>(ptr.get());

        reint_ptr->is_binding_mutable = consume_if(TokenID::KW_MUT);
        reint_ptr->var_name = expect(TokenID::IDENTIFIER).text;

        expect(TokenID::COLON);

        reint_ptr->type_decl = parse_type_decl();

        // Variable has assignment expression
        if(consume_if(TokenID::ASSIGN))
        {
            reint_ptr->init_expr = parse_expression();
        }

        expect(TokenID::SEMICOLON);
    }

    // Container declaration
    else if(consume_if(TokenID::KW_TYPE))
    {
        if(check(TokenID::KW_STRUCT))
        {
            ptr = std::make_unique<StructDeclStmt>();
            StructDeclStmt *reint_ptr = 
                static_cast<StructDeclStmt*>(ptr.get());

            reint_ptr->decl = parse_struct(true);
        }

        else if(check(TokenID::KW_COMPONENT))
        {
            ptr = std::make_unique<ComponentDeclStmt>();
            ComponentDeclStmt *reint_ptr = 
                static_cast<ComponentDeclStmt*>(ptr.get());

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

        RetStmt *reint_ptr = 
                static_cast<RetStmt*>(ptr.get());
    
        reint_ptr->ret_expr = parse_expression();
        expect(TokenID::SEMICOLON);
    }

    else if(consume_if(TokenID::KW_IF))
    {
        ptr = std::make_unique<IfStmt>();

        IfStmt * reint_ptr = static_cast<IfStmt*>(ptr.get());

        expect(TokenID::LPAREN);

        reint_ptr->condition_expr = parse_expression();

        expect(TokenID::RPAREN);

        reint_ptr->then_body = parse_scope();

        if(consume_if(TokenID::KW_ELSE))
        {
            if(check(TokenID::KW_IF))
            {
                reint_ptr->else_branch = parse_statement();
                return ptr;
            }
            
            auto else_block = std::make_unique<BlockStmt>();
            else_block->block_decl = parse_scope();
            reint_ptr->else_branch = std::move(else_block);
        }
    }

    else if(consume_if(TokenID::KW_WHILE))
    {
        ptr = std::make_unique<WhileStmt>();

        WhileStmt * const reint_ptr = static_cast<WhileStmt*>(ptr.get());

        expect(TokenID::LPAREN);

        reint_ptr->condition_expr = parse_expression();

        expect(TokenID::RPAREN);

        reint_ptr->body = parse_scope();
    }

    else if(consume_if(TokenID::KW_FOR))
    {
        ptr = std::make_unique<ForStmt>();

        ForStmt * const reint_ptr = static_cast<ForStmt*>(ptr.get());

        expect(TokenID::LPAREN);

        if(!check(TokenID::SEMICOLON))
        {
            reint_ptr->init_stmt = parse_statement();
        }
        // Depending on the statement, the semicolon may not have been consumed.
        consume_if(TokenID::SEMICOLON);
        
        if(!check(TokenID::SEMICOLON))
        {
            reint_ptr->condition_expr = parse_expression();
        }
        expect(TokenID::SEMICOLON);

        if(!check(TokenID::RPAREN))
        {
            reint_ptr->increment_expr = parse_expression();
        }

        expect(TokenID::RPAREN);

        reint_ptr->body = parse_scope();
    }

    // Block Declaration
    else if(check(TokenID::LBRACE))
    {
        ptr = std::make_unique<BlockStmt>();

        BlockStmt *reint_ptr = 
            static_cast<BlockStmt*>(ptr.get());

        reint_ptr->block_decl = parse_scope();
    }

    else
    {
        // Anything else we assume is an expression and attempt to parse it.
        ptr = std::make_unique<ExprStmt>();

        ExprStmt * const reint_ptr = 
            static_cast<ExprStmt*>(ptr.get());

        reint_ptr->expr = parse_expression();
        expect(TokenID::SEMICOLON);
    }

    ptr->line = start_line;
    ptr->col = start_col;

    return ptr;
}

std::unique_ptr<Expression> Parser::parse_expression()
{
    return parse_assignment();
}

std::unique_ptr<Expression> Parser::parse_assignment()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<Expression> left = parse_log_or();

    AssignOp op = AssignOp::INVALID;

    if(consume_if(TokenID::ASSIGN)) op = AssignOp::ASSIGN;
    else if(consume_if(TokenID::PLUS_ASSIGN)) op = AssignOp::ADD_ASSIGN;
    else if(consume_if(TokenID::MINUS_ASSIGN)) op = AssignOp::SUB_ASSIGN;
    else if(consume_if(TokenID::ASTERISK_ASSIGN)) op = AssignOp::MUL_ASSIGN;
    else if(consume_if(TokenID::FORW_SLASH_ASSIGN)) op = AssignOp::DIV_ASSIGN;
    else if(consume_if(TokenID::MODULO_ASSIGN)) op = AssignOp::MOD_ASSIGN;
    else return left;

    auto full_expr = std::make_unique<AssignExpr>();

    full_expr->line = start_line;
    full_expr->col = start_col;
    full_expr->op_type = op;
    
    full_expr->lhs = std::move(left);

    // Parse RHS using log_or. We don't support chained assignments.
    full_expr->rhs = parse_log_or();

    return full_expr;
}

std::unique_ptr<Expression> Parser::parse_log_or()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<Expression> left = parse_log_and();

    while(true)
    {
        if(!consume_if(TokenID::LOGIC_OR)) break;

        auto full_expr = std::make_unique<BinaryExpr>();

        full_expr->line = start_line;
        full_expr->col = start_col;
        full_expr->op_type = BinaryOp::LOG_OR;
        full_expr->lhs = std::move(left);
        full_expr->rhs = parse_log_and();

        left = std::move(full_expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_log_and()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<Expression> left = parse_equality();

    while(true)
    {
        if(!consume_if(TokenID::LOGIC_AND)) break;

        auto full_expr = std::make_unique<BinaryExpr>();

        full_expr->line = start_line;
        full_expr->col = start_col;
        full_expr->op_type = BinaryOp::LOG_AND;
        full_expr->lhs = std::move(left);
        full_expr->rhs = parse_equality();

        left = std::move(full_expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_equality()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<Expression> left = parse_relational();

    while(true)
    {
        BinaryOp op = BinaryOp::INVALID;

        if(consume_if(TokenID::EQUAL_EQUAL)) op = BinaryOp::EQ;
        else if(consume_if(TokenID::NOT_EQUAL)) op = BinaryOp::NE;
        else break;

        auto full_expr = std::make_unique<BinaryExpr>();

        full_expr->line = start_line;
        full_expr->col = start_col;
        full_expr->op_type = op;
        full_expr->lhs = std::move(left);
        full_expr->rhs = parse_relational();

        left = std::move(full_expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_relational()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<Expression> left = parse_additive();

    while(true)
    {
        BinaryOp op = BinaryOp::INVALID;

        if(consume_if(TokenID::LESS_THAN)) op = BinaryOp::LT;
        else if(consume_if(TokenID::GREATER_THAN)) op = BinaryOp::GT;
        else if(consume_if(TokenID::LESS_EQUAL)) op = BinaryOp::LE;
        else if(consume_if(TokenID::GREATER_EQUAL)) op = BinaryOp::GE;
        else break;

        auto full_expr = std::make_unique<BinaryExpr>();

        full_expr->line = start_line;
        full_expr->col = start_col;
        full_expr->op_type = op;
        full_expr->lhs = std::move(left);
        full_expr->rhs = parse_additive();

        left = std::move(full_expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_additive()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<Expression> left = parse_multiplicative();

    while(true)
    {
        BinaryOp op = BinaryOp::INVALID;

        if(consume_if(TokenID::PLUS)) op = BinaryOp::ADD;
        else if(consume_if(TokenID::MINUS)) op = BinaryOp::SUB;
        else break; // Did not find a proper operator, we're done.
        
        auto full_expr = std::make_unique<BinaryExpr>();

        full_expr->line = start_line;
        full_expr->col = start_col;
        full_expr->op_type = op;
        full_expr->lhs = std::move(left);
        full_expr->rhs = parse_multiplicative();
        left = std::move(full_expr);
    }

    return left;   
}

std::unique_ptr<Expression> Parser::parse_multiplicative()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<Expression> left = parse_unary();

    while(true)
    {
        BinaryOp op = BinaryOp::INVALID;

        if(consume_if(TokenID::ASTERISK)) op = BinaryOp::MUL;
        else if(consume_if(TokenID::FORW_SLASH)) op = BinaryOp::DIV;
        else if(consume_if(TokenID::MODULO)) op = BinaryOp::MOD;
        else break; // Did not find a proper operator, we're done.
        
        auto full_expr = std::make_unique<BinaryExpr>();

        full_expr->line = start_line;
        full_expr->col = start_col;
        full_expr->op_type = op;
        full_expr->lhs = std::move(left);
        full_expr->rhs = parse_unary();
        left = std::move(full_expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_unary()
{
    if(check(TokenID::PLUS_PLUS))
    {   
        Token t = expect(TokenID::PLUS_PLUS);

        auto ptr = std::make_unique<UnaryExpr>();
        
        ptr->op_type = UnaryOp::PRE_INC;
        ptr->operand = parse_unary();
        ptr->line = t.line;
        ptr->col = t.col;
        return ptr;
    }

    else if(check(TokenID::MINUS_MINUS))
    {
        Token t = expect(TokenID::MINUS_MINUS);

        auto ptr = std::make_unique<UnaryExpr>();
        
        ptr->op_type = UnaryOp::PRE_DEC;
        ptr->operand = parse_unary();
        ptr->line = t.line;
        ptr->col = t.col;
        return ptr;
    }

    else if(check(TokenID::MINUS))
    {
        Token t = expect(TokenID::MINUS);

        auto ptr = std::make_unique<UnaryExpr>();
        
        ptr->op_type = UnaryOp::NEGATE;
        ptr->operand = parse_unary();
        ptr->line = t.line;
        ptr->col = t.col;
        return ptr;
    }

    else if(check(TokenID::EXCLAMATION_POINT))
    {
        Token t = expect(TokenID::EXCLAMATION_POINT);

        auto ptr = std::make_unique<UnaryExpr>();
        
        ptr->op_type = UnaryOp::LOG_NOT;
        ptr->operand = parse_unary();
        ptr->line = t.line;
        ptr->col = t.col;
        return ptr;
    }

    else if(check(TokenID::TILDE))
    {
        Token t = expect(TokenID::TILDE);

        auto ptr = std::make_unique<UnaryExpr>();
        
        ptr->op_type = UnaryOp::BIT_NOT;
        ptr->operand = parse_unary();
        ptr->line = t.line;
        ptr->col = t.col;
        return ptr;
    }

    else if(check(TokenID::AMPERSAND))
    {
        Token t = expect(TokenID::AMPERSAND);

        auto ptr = std::make_unique<UnaryExpr>();
        
        ptr->op_type = UnaryOp::ADDRESS_OF;
        ptr->operand = parse_unary();
        ptr->line = t.line;
        ptr->col = t.col;
        return ptr;
    }

    else if(check(TokenID::ASTERISK))
    {
        Token t = expect(TokenID::ASTERISK);

        auto ptr = std::make_unique<UnaryExpr>();
        
        ptr->op_type = UnaryOp::DEREF;
        ptr->operand = parse_unary();
        ptr->line = t.line;
        ptr->col = t.col;
        return ptr;
    }

    else if(check(TokenID::LPAREN))
    {
        Token t = expect(TokenID::LPAREN);

        uint64_t saved_tok_idx = current_token_idx;

        // Attempt to parse a type with error on failure as false. 
        auto type_ptr = parse_type_decl(false);

        // If the ptr was not returned as nullptr, we have successfully parsed
        // a type, and this is a cast
        if(type_ptr != nullptr) 
        {
            auto expr_ptr = std::make_unique<CastExpr>();

            expr_ptr->to_cast_type = std::move(type_ptr); 
            expr_ptr->line = t.line;
            expr_ptr->col = t.col;
            expect(TokenID::RPAREN);
            expr_ptr->expr_to_cast = parse_unary();
            return expr_ptr;
        }

        // If the type parsing failed, rewind to the token before we starting
        // attempting to consume the non type.
        else rewind(saved_tok_idx);
    }

    return parse_postfix();
}

std::unique_ptr<Expression> Parser::parse_postfix()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    // Expression that came before the postfix.
    std::unique_ptr<Expression> pre_expr = parse_primary();

    while(true)
    {
        // Function call
        if(consume_if(TokenID::LPAREN))
        {
            std::unique_ptr<CallExpr> full_expr = 
                std::make_unique<CallExpr>();

            full_expr->line = start_line;
            full_expr->col = start_col;

            // Move the pre expression into the full expression's callee expr
            full_expr->callee_expr = std::move(pre_expr);
            
            // Parse arguments
            while(!check(TokenID::RPAREN))
            {
                full_expr->args.push_back(parse_expression());

                // Trailing comma
                if(consume_if(TokenID::COMMA) && check(TokenID::RPAREN))
                {
                    print_error_location(peek().line, peek().col);
                    std::cerr << ": Trailing comma in argument list\n";
                    exit(1);
                }
            }

            expect(TokenID::RPAREN);

            // Move the full expr now into the pre expr so we can continue 
            // parsing any further postfix operations with this as the pre expr.
            pre_expr = std::move(full_expr);
        }

        // Subscript
        else if(consume_if(TokenID::LBRACKET))
        {
            std::unique_ptr<SubscriptExpr> full_expr = 
                std::make_unique<SubscriptExpr>();
            
            full_expr->line = start_line;
            full_expr->col = start_col;

            // Move the pre expression into the full expression's base expr
            full_expr->base_expr = std::move(pre_expr);

            // Get the expression for the index. 
            full_expr->index_expr = parse_expression();

            expect(TokenID::RBRACKET);
        
            pre_expr = std::move(full_expr);
        }

        // Member access by dot
        else if(consume_if(TokenID::DOT))
        {
            std::unique_ptr<MemberAccExpr> full_expr = 
                std::make_unique<MemberAccExpr>();

            full_expr->line = start_line;
            full_expr->col = start_col;
            full_expr->base_expr = std::move(pre_expr);
            full_expr->member_name = expect(TokenID::IDENTIFIER).text;
            full_expr->via_pointer = false;

            pre_expr = std::move(full_expr);
        }

        // Member access by arrow
        else if(consume_if(TokenID::ARROW))
        {
            std::unique_ptr<MemberAccExpr> full_expr =
                std::make_unique<MemberAccExpr>();
                
            full_expr->line = start_line;
            full_expr->col = start_col;
            full_expr->base_expr = std::move(pre_expr);
            full_expr->member_name = expect(TokenID::IDENTIFIER).text;
            full_expr->via_pointer = true;

            pre_expr = std::move(full_expr);
        }

        // Post increment
        else if(consume_if(TokenID::PLUS_PLUS))
        {
            std::unique_ptr<UnaryExpr> full_expr = 
                std::make_unique<UnaryExpr>();

            full_expr->line = start_line;
            full_expr->col = start_col;
            full_expr->op_type = UnaryOp::POST_INC;
            full_expr->operand = std::move(pre_expr);
            
            pre_expr = std::move(full_expr);
        }

        else if(consume_if(TokenID::MINUS_MINUS))
        {
            std::unique_ptr<UnaryExpr> full_expr = 
                std::make_unique<UnaryExpr>();

            full_expr->line = start_line;
            full_expr->col = start_col;
            full_expr->op_type = UnaryOp::POST_DEC;
            full_expr->operand = std::move(pre_expr);

            pre_expr = std::move(full_expr);
        }

        // No more postfix expressions.
        else break;
    }

    return pre_expr;
}

std::unique_ptr<Expression> Parser::parse_primary()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    if(check(TokenID::INT_LITERAL))
    {
        auto ptr = std::make_unique<IntLitExpr>();

        IntLitExpr * const reint_ptr = static_cast<IntLitExpr*>(ptr.get());

        ptr->line = start_line;
        ptr->col = start_col;
        reint_ptr->value = expect(TokenID::INT_LITERAL).text;
        return ptr;
    }

    if(check(TokenID::FLOAT_LITERAL))
    {
        auto ptr = std::make_unique<FloatLitExpr>();

        FloatLitExpr * const reint_ptr = static_cast<FloatLitExpr*>(ptr.get());

        ptr->line = peek().line;
        ptr->col = peek().col;
        reint_ptr->value = expect(TokenID::FLOAT_LITERAL).text;
        return ptr;
    }

    if(check(TokenID::HEX_LITERAL))
    {
        auto ptr = std::make_unique<HexLitExpr>();

        HexLitExpr * const reint_ptr = static_cast<HexLitExpr*>(ptr.get());

        ptr->line = peek().line;
        ptr->col = peek().col;
        reint_ptr->value = expect(TokenID::HEX_LITERAL).text;
        return ptr;
    }

    if(check(TokenID::BIN_LITERAL))
    {
        auto ptr = std::make_unique<BinaryLitExpr>();

        BinaryLitExpr * const reint_ptr = static_cast<BinaryLitExpr*>(ptr.get());

        ptr->line = peek().line;
        ptr->col = peek().col;
        reint_ptr->value = expect(TokenID::BIN_LITERAL).text;
        return ptr;
    }

    if(check(TokenID::STR_LITERAL))
    {
        auto ptr = std::make_unique<StringLitExpr>();

        StringLitExpr * const reint_ptr = static_cast<StringLitExpr*>(ptr.get());

        ptr->line = peek().line;
        ptr->col = peek().col;
        reint_ptr->value = expect(TokenID::STR_LITERAL).text;
        return ptr;
    }

    if(check(TokenID::CHAR_LITERAL))
    {
        auto ptr = std::make_unique<CharLitExpr>();

        CharLitExpr * const reint_ptr = static_cast<CharLitExpr*>(ptr.get());

        ptr->line = peek().line;
        ptr->col = peek().col;
        reint_ptr->value = expect(TokenID::CHAR_LITERAL).text.at(0);
        return ptr;
    }

    if(check(TokenID::BOOL_LITERAL))
    {
        auto ptr = std::make_unique<BoolLitExpr>();

        BoolLitExpr * const reint_ptr = static_cast<BoolLitExpr*>(ptr.get());

        ptr->line = peek().line;
        ptr->col = peek().col;
        reint_ptr->value = expect(TokenID::BOOL_LITERAL).text == "true";
        return ptr;
    }

    if(check(TokenID::NULLPTR_LITERAL))
    {
        auto ptr = std::make_unique<NullptrLitExpr>();

        ptr->line = peek().line;
        ptr->col = peek().col;
        expect(TokenID::NULLPTR_LITERAL);
        return ptr;
    }

    if(consume_if(TokenID::LPAREN))
    {
        auto ptr = parse_expression();
        expect(TokenID::RPAREN);
        return ptr;
    }

    if(check(TokenID::IDENTIFIER))
    {
        auto ptr = std::make_unique<IdentExpr>();

        IdentExpr * const reint_ptr = static_cast<IdentExpr*>(ptr.get());

        ptr->line = peek().line;
        ptr->col = peek().col;
        reint_ptr->name = expect(TokenID::IDENTIFIER).text;
        return ptr;
    }

    print_error_location(start_line, start_col);
    std::cerr << ": Invalid expression\n";
    exit(1);    
}

std::unique_ptr<TypeDecl> Parser::parse_type_decl(bool error_on_invalid) 
{
    std::unique_ptr<TypeDecl> ptr = nullptr;
    
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    // Named 
    if(check(TokenID::IDENTIFIER))
    {
        ptr = std::make_unique<NamedTypeDecl>();
        ptr->kind = TypeKind::NAMED;
        NamedTypeDecl *reint_ptr = static_cast<NamedTypeDecl*>(ptr.get());

        if(!error_on_invalid && !check(TokenID::IDENTIFIER)) return nullptr;

        reint_ptr->type_name = expect(TokenID::IDENTIFIER).text;        
    }

    // Pointer 
    else if(consume_if(TokenID::ASTERISK))
    {
        ptr = std::make_unique<PtrTypeDecl>();
        ptr->kind = TypeKind::PTR;
        PtrTypeDecl *reint_ptr = static_cast<PtrTypeDecl*>(ptr.get());

        reint_ptr->points_to_mutable = consume_if(TokenID::KW_MUT);
        reint_ptr->pointee = parse_type_decl();
    }

    // Reference
    else if(consume_if(TokenID::KW_REF))
    {   
        ptr = std::make_unique<RefTypeDecl>();
        ptr->kind = TypeKind::REF;
        RefTypeDecl *reint_ptr = static_cast<RefTypeDecl*>(ptr.get());

        reint_ptr->ref_to_mutable = consume_if(TokenID::KW_MUT);
        reint_ptr->referred = parse_type_decl();
    }

    // Function pointer
    else if(consume_if(TokenID::KW_FN))
    {
        ptr = std::make_unique<FuncPtrDecl>();
        ptr->kind = TypeKind::FUNC_PTR;
        FuncPtrDecl *reint_ptr = static_cast<FuncPtrDecl*>(ptr.get());

        if(!error_on_invalid && !check(TokenID::LPAREN)) return nullptr;

        expect(TokenID::LPAREN);

        if(!error_on_invalid && !check(TokenID::LPAREN)) return nullptr;

        expect(TokenID::LPAREN);

        // Parse parameters
        while(true)
        {
            reint_ptr->param_types.push_back(parse_param());

            // If the next token is not a comma, it must be a right paren
            if(!check(TokenID::COMMA))
            {
                if(!error_on_invalid && check(TokenID::RPAREN)) return nullptr;
                expect(TokenID::RPAREN);
                break;
            }

            // Comma present, read in next parameter
            if(!error_on_invalid && check(TokenID::COMMA)) return nullptr;
            expect(TokenID::COMMA);
        }

        if(!error_on_invalid && check(TokenID::ARROW)) return nullptr;
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
    if(consume_if(TokenID::LBRACKET))
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
    auto ptr = std::make_unique<FieldDecl>();

    ptr->line = peek().line;
    ptr->col = peek().col;

    ptr->is_pub = is_pub;
    ptr->is_binding_mutable = consume_if(TokenID::KW_MUT);
    ptr->name = expect(TokenID::IDENTIFIER).text;
    expect(TokenID::COLON);
    ptr->type_decl = parse_type_decl();
    expect(TokenID::SEMICOLON);

    return ptr;
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
    p.is_binding_mutable = consume_if(TokenID::KW_MUT);

    p.name = expect(TokenID::IDENTIFIER).text;

    expect(TokenID::COLON);

    // This is an unqualified parameter
    if(check(TokenID::IDENTIFIER))
    {
        p.is_unqual_param = true;
    }

    // Parameter is passed in as copy
    else if(consume_if(TokenID::KW_VAL))
    {
        if(check(TokenID::KW_REF))
        {
            print_error_location(peek().line, peek().col);
            std::cerr << ": Reference parameters cannot be passed in by val.\n";
            exit(1); 
        }

        p.passed_by_copy = true;
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

// Parser.cpp

#include "Parser.hpp"

#include "Builtins.hpp"
#include "OperatorPrecedence.hpp"


// Ctors / Dtor

Parser::Parser() 
{
    register_builtin_types();
}


// Public

Program Parser::parse(const char *in_file_path) 
{
    current_token_idx = 0;

    Lexer l;

    tokens = l.lex(in_file_path);

    Program prog;
    prog.source_file_name = in_file_path;
    parsed_file = in_file_path;

    prog.ast->ident = "global";
    prog.ast->line = 0;
    prog.ast->col = 0;

    std::unique_ptr<Declaration> decl = parse_top_level();

    while(decl)
    {
        prog.ast->decls.push_back(std::move(decl));
        decl = parse_top_level();
    }

    parsed_file = nullptr;

    return prog;
}


// Private

void Parser::register_builtin_types()
{
    for(size_t i = 0; i < readable_to_builtin.size(); ++i)
    {
        defined_types.emplace(readable_to_builtin.at(i).first,
            DefinedType{0, 0, "BUILTIN"});
    }
}

void Parser::print_ident_path(const std::vector<std::string> &ident_path) const
{
    if(ident_path.size() == 0) return;

    size_t i = 0;

    while(true)
    {
        std::cout << ident_path.at(i);

        ++i;

        if(i >= ident_path.size()) break;

        std::cout << "::";
    }
}

void Parser::print_error_location(uint32_t line, uint32_t col) const
{
    std::cout << parsed_file << ":" << line << ':' << col;
}

void Parser::print_missing_brace(uint32_t expected_line, uint32_t expected_col,
    uint32_t lbrace_line, uint32_t lbrace_col) const
{
    print_error_location(expected_line, expected_col);
    std::cout << " -> Missing '}' to match '{' here: ";
    print_error_location(lbrace_line, lbrace_col);
    std::cout << ".\n";
}

void Parser::handle_tok_mismatch(const Token &got_tok, TokenID expected) 
{
    print_error_location(got_tok.line, got_tok.col);
    std::cout << " -> Expect token of type: " << 
        tokID_readable[static_cast<int>(expected)] << " but got token:\n" << 
        got_tok << '\n';
    exit(1);
}

void Parser::handle_unexpected_token(const Token &got_tok)
{
    print_error_location(got_tok.line, got_tok.col);
    std::cout << " -> Unexpected token:\n" << got_tok;
    exit(1);
}

void Parser::handle_register_type(const std::string &name, uint32_t line,
    uint32_t col)
{
    auto it  = defined_types.find(name);

    // This type has already been defined.
    if(it != defined_types.end())
    {
        print_error_location(line, col);
        std::cout << " -> Struct type: \"" << name << "\" already defined here: "
            << it->second.file_defined << ':' << it->second.line << ':' << 
            it->second.col << ".\n";
        exit(1);
    }

    // Register this type as defined.
    defined_types.emplace(name, DefinedType{line, col, parsed_file});
}

std::unique_ptr<Declaration> Parser::parse_top_level() 
{
    Token t = peek();
    
    bool is_exported = false;

    if(consume_if(TokenID::KW_EXPORT)) is_exported = true;

    if(check(TokenID::END_OF_FILE)) return nullptr;

    if(check(TokenID::KW_MODULE)) 
    {
        if(is_exported)
        {
            std::cout << "Sub Modules cannot be marked \"export\"\n";
            exit(1);
        }

        return parse_module();
    }

    if(check(TokenID::KW_FN)) return parse_function(is_exported);

    if(consume_if(TokenID::KW_TYPE)) 
    {
        if(check(TokenID::KW_STRUCT)) return parse_struct(is_exported);
        if(check(TokenID::KW_COMPONENT)) return parse_component(is_exported);
        
        print_error_location(peek().line, peek().col);
        std::cout << " -> \"type\" not followed by a declaration of a struct or "
            "component.\n";
        exit(1);
    }

    // Global variable
    if(check(TokenID::IDENTIFIER) || check(TokenID::KW_MUT)) 
        return parse_field(is_exported);

    handle_unexpected_token(t);
    
    return nullptr;
}

std::unique_ptr<ModuleDecl> Parser::parse_module() 
{
    // We need to handle modules that are defined with a scope chain such as 
    // "TopScope::NestedScope::AnotherNestedScope{}". The "TopScope" ast node
    // is the one we are retuning, and that is the "top_level_mod" below.

    std::unique_ptr<ModuleDecl> top_level_mod = 
        std::make_unique<ModuleDecl>();

    // This module ptr points to the module that contains the declarations of 
    // the potentially scope chained module. If this is a just a single scope
    // defined module, module_to_fill will simply be the same as top level mod.
    // However, if this is a scope chained module, then we need to return the 
    // top level module, but build a scoped-chain of module ast nodes and only
    // fill the last one, since that is the last defined scope of the module 
    // and it contains the declarations to parse.
    ModuleDecl * module_to_fill = top_level_mod.get();
    expect(TokenID::KW_MODULE);

    top_level_mod->line = peek().line;
    top_level_mod->col = peek().col;


    top_level_mod->ident = expect(TokenID::IDENTIFIER).text;

    // Iterate through any scope chain this module has
    while(consume_if(TokenID::SCOPE_RESOLUTION))
    {
        // Create a new module
        std::unique_ptr<ModuleDecl> nested_ptr = std::make_unique<ModuleDecl>();
        nested_ptr->line = peek().line;
        nested_ptr->col = peek().col;   
        nested_ptr->ident = expect(TokenID::IDENTIFIER).text;

        // Save the nested ptr
        ModuleDecl * raw_nested_ptr = nested_ptr.get();
        
        // Move the nested pointer into the declarations of the last constructed
        // module in the scope chain.
        module_to_fill->decls.push_back(std::move(nested_ptr));
        
        // Set the module to fill to be the last constructed module in the scope
        // chain, which was the nested ptr we just made.
        module_to_fill = raw_nested_ptr;
    }
    
    // If there was a scope chain, module_to_fill now points to the deepest 
    // nested ModuleDecl. If there was no scope chain, there's just one module
    // and it's top_level_module.

    expect(TokenID::LBRACE);

    while(!check(TokenID::RBRACE))
    {
        std::unique_ptr<Declaration> decl = parse_top_level();

        if(!decl)
        {
            std::cout << parsed_file << " -> Reached end of file expecting '}' "
                "for module: \""; 
            std::cout << top_level_mod->ident;
            std::cout << "\" found here: ";
            print_error_location(top_level_mod->line, top_level_mod->col);
            std::cout << '\n';
            exit(1);
        }

        top_level_mod->decls.push_back(std::move(decl));
    }

    expect(TokenID::RBRACE);
    return top_level_mod;
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
        if(check(TokenID::KW_MUT))
        {
            print_error_location(peek().line, peek().col);
            std::cout << " -> Receiver parameter can't be marked mutable.\n";
            exit(1);
        }

        Parameter p;
        p.line = peek().line;
        p.col = peek().col;
        p.is_binding_mutable = false;
        p.is_unqual_param = false;
        p.passed_by_copy = false;
        p.name = expect(TokenID::IDENTIFIER).text;
        expect(TokenID::COLON);
        p.type_decl = parse_type_decl();
                
        ptr->receiver_data.emplace(std::move(p));
        expect(TokenID::RPAREN);
    }

    ptr->name = expect(TokenID::IDENTIFIER).text;

    expect(TokenID::LPAREN);

    // Parse parameters
    while(!consume_if(TokenID::RPAREN))
    {
        ptr->params.push_back(parse_param());

        if(!consume_if(TokenID::COMMA))
        {
            expect(TokenID::RPAREN);
            break;
        }

        if(check(TokenID::RPAREN))

        // Trailing comma
        // if(consume_if(TokenID::COMMA) && check(TokenID::RPAREN))
        {
            print_error_location(peek().line, peek().col);
            std::cout << " -> Trailing comma in parameter list\n";
            exit(1);
        }
    }

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

    uint32_t ident_line = peek().line;
    uint32_t ident_col = peek().col;

    ptr->name = expect(TokenID::IDENTIFIER).text;
    
    handle_register_type(ptr->name, ident_line, ident_col);

    uint32_t lbrace_line = peek().line;
    uint32_t lbrace_col = peek().col;

    expect(TokenID::LBRACE);

    // Parse struct body.
    while(!check(TokenID::RBRACE))
    {
        if(check(TokenID::END_OF_FILE))
        {
            print_missing_brace(peek().line, peek().col, lbrace_line, 
                lbrace_col);
            exit(1);
        }
        
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

    uint32_t ident_line = peek().line;
    uint32_t ident_col = peek().col;

    ptr->name = expect(TokenID::IDENTIFIER).text;
    
    handle_register_type(ptr->name, ident_line, ident_col);
    
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
                std::cout << " -> \"type\" not followed by a declaration of a "
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

std::unique_ptr<VarDeclStmt> Parser::parse_var_decl_stmt()
{
    auto ptr = std::make_unique<VarDeclStmt>();

    ptr->line = peek().line;
    ptr->col = peek().col;

    VarDeclStmt *reint_ptr = 
        static_cast<VarDeclStmt*>(ptr.get());

    reint_ptr->is_binding_mutable = consume_if(TokenID::KW_MUT);
    reint_ptr->var_name = expect(TokenID::IDENTIFIER).text;

    expect(TokenID::COLON);

    reint_ptr->type_decl = parse_type_decl();

    // Variable has assignment expression
    if(consume_if(TokenID::ASSIGN))
    {
        // Struct or Array init.
        if(check(TokenID::LBRACE))
        {
            if(reint_ptr->type_decl->kind == TypeKind::NAMED)
                reint_ptr->init_expr = parse_struct_init(); 
            else reint_ptr->init_expr = parse_arr_init();
        }

        else reint_ptr->init_expr = parse_expression();
    }

    expect(TokenID::SEMICOLON);

    return ptr;
}

std::unique_ptr<Statement> Parser::parse_statement() 
{
    std::unique_ptr<Statement> ptr = nullptr;

    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    // Variable declaration
    if(consume_if(TokenID::KW_LET)) return parse_var_decl_stmt();

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
            std::cout << " -> \"type\" not followed by a declaration of a struct or "
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
    
        // We have a return statement expression.
        if(!check(TokenID::SEMICOLON))
        {
            reint_ptr->ret_expr = parse_expression();
        }

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
        // std::cout << "Parsing expression: ";
        // print_error_location(start_line, start_col);
        // std::cout << '\n';

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

std::unique_ptr<Expression> Parser::parse_arr_init()
{
    std::unique_ptr<ArrInitExpr> ptr = std::make_unique<ArrInitExpr>();

    ptr->line = peek().line;
    ptr->col = peek().col;

    expect(TokenID::LBRACE);

    while(!check(TokenID::RBRACE))
    {
        ptr->init_args.push_back(parse_expression());

        // Trailing comma 
        if(consume_if(TokenID::COMMA) && check(TokenID::RBRACE))
        {
            print_error_location(peek().line, peek().col);
            std::cout << " -> Trailing comma in initialization list\n";
            exit(1);
        }
    }

    expect(TokenID::RBRACE);

    return ptr;
}

std::unique_ptr<Expression> Parser::parse_struct_init()
{
    std::unique_ptr<StructInitExpr> ptr = std::make_unique<StructInitExpr>();

    ptr->line = peek().line;
    ptr->col = peek().col;

    expect(TokenID::LBRACE);

    while(consume_if(TokenID::DOT))
    {
        std::string member_name = expect(TokenID::IDENTIFIER).text;
        expect(TokenID::ASSIGN);
        std::unique_ptr<Expression> init_expr = parse_expression();

        ptr->init_args.push_back({std::move(member_name), 
            std::move(init_expr)});

        if(consume_if(TokenID::COMMA) && check(TokenID::RBRACE))
        {
            print_error_location(peek().line, peek().col);
            std::cout << " -> Trailing comma in initialization list\n";
            exit(1);
        }
    }

    expect(TokenID::RBRACE);

    return ptr;
}

std::unique_ptr<Expression> Parser::parse_expression(uint32_t min_prec)
{
    std::unique_ptr<Expression> lhs = parse_prefix();

    return pratt_parse(std::move(lhs), min_prec);
}

std::unique_ptr<Expression> Parser::pratt_parse(
    std::unique_ptr<Expression> lhs, uint32_t min_prec)
{
    while(true)
    {
        const TokenID tok_id = peek().id;
        const OpData op_data = get_opdata(tok_id);

        if(op_data.precedence == 0 || op_data.precedence <= min_prec) break;

        if(tok_id == TokenID::LPAREN)
        {
            lhs = parse_func_call(std::move(lhs));
            continue;
        }

        if(tok_id == TokenID::LBRACKET)
        {
            lhs = parse_arr_sub(std::move(lhs));
            continue;
        }

        if(tok_id == TokenID::DOT || tok_id == TokenID::ARROW)
        {
            lhs = parse_member_access(std::move(lhs));
            continue;
        }

        if(tok_id == TokenID::PLUS_PLUS)
        {
            lhs = parse_post_inc_dec(std::move(lhs));
            continue;
        }

        // Struct init expr
        if(tok_id == TokenID::LBRACE)
        {
            std::unique_ptr<StructCreateExpr> ptr = 
                std::make_unique<StructCreateExpr>();

            ptr->line = lhs->line;
            ptr->col = lhs->col;

            ptr->lhs = std::move(lhs);
            ptr->create_expr = parse_struct_init();

            lhs = std::move(ptr);
            continue;
        }

        // We didn't have any postfix operators.

        uint32_t min_rhs_prec = op_data.assoc == Associativity::RIGHT ? 
            op_data.precedence - 1 : op_data.precedence;

        lhs = parse_infix(std::move(lhs), min_rhs_prec);
    }

    return lhs;
}

std::unique_ptr<Expression> Parser::parse_infix(
    std::unique_ptr<Expression> lhs, uint32_t min_rhs_prec)
{
    const TokenID tok_id = peek().id;

    const static std::unordered_map<TokenID, BinaryOp> bin_op_lookup
    {
        {TokenID::PLUS, BinaryOp::ADD},
        {TokenID::MINUS, BinaryOp::SUB},
        {TokenID::ASTERISK, BinaryOp::MUL},
        {TokenID::FORW_SLASH, BinaryOp::DIV},
        {TokenID::MODULO, BinaryOp::MOD},
        {TokenID::EQUAL_EQUAL, BinaryOp::EQ},
        {TokenID::NOT_EQUAL, BinaryOp::NE},
        {TokenID::LESS_THAN, BinaryOp::LT},
        {TokenID::LESS_EQUAL, BinaryOp::LE},
        {TokenID::GREATER_THAN, BinaryOp::GT},
        {TokenID::GREATER_EQUAL, BinaryOp::GE},
        {TokenID::LOGIC_AND, BinaryOp::LOG_AND},
        {TokenID::LOGIC_OR, BinaryOp::LOG_OR},
        {TokenID::SHIFT_LEFT, BinaryOp::LSHIFT},
        {TokenID::SHIFT_RIGHT, BinaryOp::RSHIFT}
    };

    const static auto bin_op_lookup_end = bin_op_lookup.end();

    auto bin_op_it = bin_op_lookup.find(tok_id);

    // Binary Expression
    if(bin_op_it != bin_op_lookup_end)
    {
        std::unique_ptr<BinaryExpr> ptr = std::make_unique<BinaryExpr>();

        ptr->op_type = bin_op_it->second;
        ptr->line = lhs->line;
        ptr->col = lhs->col;
        
        // Consume operator
        advance();
        ptr->lhs = std::move(lhs);
        ptr->rhs = parse_expression(min_rhs_prec);
        return ptr;        
    }

    const static std::unordered_map<TokenID, AssignOp> assign_op_lookup
    {
        {TokenID::ASSIGN, AssignOp::ASSIGN},
        {TokenID::PLUS_ASSIGN, AssignOp::ADD_ASSIGN},
        {TokenID::MINUS_ASSIGN, AssignOp::SUB_ASSIGN},
        {TokenID::ASTERISK_ASSIGN, AssignOp::MUL_ASSIGN},
        {TokenID::FORW_SLASH_ASSIGN, AssignOp::DIV_ASSIGN},
        {TokenID::MODULO_ASSIGN, AssignOp::MOD_ASSIGN},
        {TokenID::BIT_AND_ASSIGN, AssignOp::BIT_AND_ASSIGN},
        {TokenID::BIT_OR_ASSIGN, AssignOp::BIT_OR_ASSIGN},
        {TokenID::BIT_XOR_ASSIGN, AssignOp::BIT_XOR_ASSIGN},
        {TokenID::SHIFT_LEFT_ASSIGN, AssignOp::LSHIFT_ASSIGN},
        {TokenID::SHIFT_RIGHT_ASSIGN, AssignOp::RSHIFT_ASSIGN}
    };

    const static auto assign_op_lookup_end = assign_op_lookup.end();

    auto assign_op_it = assign_op_lookup.find(tok_id);

    // Assignment expression
    if(assign_op_it != assign_op_lookup_end)
    {
        std::unique_ptr<AssignExpr> ptr = std::make_unique<AssignExpr>();

        ptr->op_type = assign_op_it->second;
        ptr->line = lhs->line;
        ptr->col = lhs->col;
        
        advance(); // Consume Assignment
        ptr->lhs = std::move(lhs);
        ptr->rhs = parse_expression(min_rhs_prec);
        return ptr;
    }

    // Ternary operator
    if(tok_id == TokenID::TERNARY)
    {
        std::unique_ptr<TernaryExpr> ptr = std::make_unique<TernaryExpr>();

        ptr->line = lhs->line;
        ptr->col = lhs->col;

        advance(); // Consume '?'

        ptr->condition = std::move(lhs);
        ptr->on_true_expr = parse_expression();

        expect(TokenID::COLON);

        ptr->on_false_expr = parse_expression(
            get_opdata(TokenID::TERNARY).precedence - 1);

        return ptr;
    }

    // Invalid token
    print_error_location(peek().line, peek().col);
    std::cout << " -> Invalid expression, unexpected token: " << 
        peek().id << '\n';
    exit(1);
}

std::unique_ptr<Expression> Parser::parse_func_call(
    std::unique_ptr<Expression> lhs)
{
    expect(TokenID::LPAREN);
    
    std::unique_ptr<CallExpr> ptr = std::make_unique<CallExpr>();

    ptr->line = lhs->line;
    ptr->col = lhs->col;

    ptr->callee_expr = std::move(lhs);

    while(!consume_if(TokenID::RPAREN))
    {
        ptr->args.push_back(parse_expression());
        
        uint32_t line = peek().line;
        uint32_t col = peek().col;

        if(!consume_if(TokenID::COMMA))
        {
            expect(TokenID::RPAREN);
            break;
        }

        if(check(TokenID::RPAREN))
        {
            print_error_location(line, col);
            std::cout << " -> Trailing comma in parameter list\n";
            exit(1);
        }
    }

    return ptr;
}   

std::unique_ptr<Expression> Parser::parse_arr_sub(
    std::unique_ptr<Expression> lhs)
{
    expect(TokenID::LBRACKET);

    std::unique_ptr<SubscriptExpr> ptr = std::make_unique<SubscriptExpr>();

    ptr->line = lhs->line;
    ptr->col = lhs->col;

    ptr->base_expr = std::move(lhs);
    ptr->index_expr = parse_expression();
    expect(TokenID::RBRACKET);

    return ptr;
}

std::unique_ptr<Expression> Parser::parse_member_access(
    std::unique_ptr<Expression> lhs)
{
    std::unique_ptr<MemberAccExpr> ptr = std::make_unique<MemberAccExpr>();

    ptr->line = lhs->line;
    ptr->col = lhs->col;

    ptr->via_pointer = check(TokenID::ARROW);
    advance();

    ptr->base_expr = std::move(lhs);
    ptr->member_name = expect(TokenID::IDENTIFIER).text;
    return ptr;
}

std::unique_ptr<Expression> Parser::parse_post_inc_dec(
    std::unique_ptr<Expression> lhs)
{
    std::unique_ptr<UnaryExpr> ptr = std::make_unique<UnaryExpr>();

    ptr->line = lhs->line;
    ptr->col = lhs->col;

    ptr->op_type = check(TokenID::PLUS_PLUS) ? 
        UnaryOp::POST_INC : UnaryOp::POST_DEC;
    advance();

    ptr->operand = std::move(lhs);
    return ptr;
}

std::unique_ptr<Expression> Parser::parse_prefix()
{
    std::unique_ptr<Expression> ptr;

    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    switch(peek().id)
    {
        case TokenID::INT_LITERAL:
        {
            ptr = std::make_unique<IntLitExpr>();

            IntLitExpr * reint_ptr = static_cast<IntLitExpr*>(ptr.get());

            reint_ptr->value = expect(TokenID::INT_LITERAL).text;
            break;
        }

        case TokenID::FLOAT_LITERAL:
        {
            ptr = std::make_unique<FloatLitExpr>();

            FloatLitExpr * reint_ptr = static_cast<FloatLitExpr*>(ptr.get());

            reint_ptr->value = expect(TokenID::FLOAT_LITERAL).text;
            break;
        }

        case TokenID::CHAR_LITERAL:
        {
            ptr = std::make_unique<CharLitExpr>();

            CharLitExpr * reint_ptr = static_cast<CharLitExpr*>(ptr.get());

            reint_ptr->value = expect(TokenID::CHAR_LITERAL).text.at(0);
            break;
        }

        case TokenID::BOOL_LITERAL:
        {
            ptr = std::make_unique<BoolLitExpr>();

            BoolLitExpr * reint_ptr = static_cast<BoolLitExpr*>(ptr.get());

            reint_ptr->value = expect(TokenID::BOOL_LITERAL).text == "true";
            break;
        }

        case TokenID::NULLPTR_LITERAL:
        {
            ptr = std::make_unique<NullptrLitExpr>();
            advance();
            break;
        }

        case TokenID::HEX_LITERAL:
        {
            ptr = std::make_unique<HexLitExpr>();

            HexLitExpr * reint_ptr = static_cast<HexLitExpr*>(ptr.get());

            reint_ptr->value = expect(TokenID::HEX_LITERAL).text;
            break;
        }

        case TokenID::BIN_LITERAL:
        {
            ptr = std::make_unique<BinaryLitExpr>();

            BinaryLitExpr * reint_ptr = static_cast<BinaryLitExpr*>(ptr.get());

            reint_ptr->value = expect(TokenID::BIN_LITERAL).text;
            break;
        }

        case TokenID::STR_LITERAL:
        {
            ptr = std::make_unique<StringLitExpr>();

            StringLitExpr * reint_ptr = static_cast<StringLitExpr*>(ptr.get());

            reint_ptr->value = expect(TokenID::STR_LITERAL).text;
            break;
        }

        case TokenID::IDENTIFIER:
        {
            ptr = std::make_unique<IdentExpr>();

            IdentExpr * reint_ptr = static_cast<IdentExpr*>(ptr.get());

            do
            {
                reint_ptr->ident_path.push_back(
                    expect(TokenID::IDENTIFIER).text);
            } while (consume_if(TokenID::SCOPE_RESOLUTION));

            break;
        }

        // Pre increment
        case TokenID::PLUS_PLUS:
        {
            ptr = std::make_unique<UnaryExpr>();

            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::PRE_INC;

            // Consume '++'
            advance();

            reint_ptr->operand = parse_expression(PREFIX_PRECEDENCE);
            break;
        }

        // Pre decrement
        case TokenID::MINUS_MINUS:
        {
            ptr = std::make_unique<UnaryExpr>();

            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::PRE_DEC;

            // Consume '--'
            advance();

            reint_ptr->operand = parse_expression(PREFIX_PRECEDENCE);
            break;
        }

        // Numerical negation
        case TokenID::MINUS:
        {
            ptr = std::make_unique<UnaryExpr>();

            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::NEGATE;

            // Consume -
            advance();

            reint_ptr->operand = parse_expression(PREFIX_PRECEDENCE);
            break;
        }

        // Logical negation
        case TokenID::EXCLAMATION_POINT:
        {
            ptr = std::make_unique<UnaryExpr>();

            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::LOG_NOT;

            // Consume !
            advance();

            reint_ptr->operand = parse_expression(PREFIX_PRECEDENCE);
            break;
        }

        // Bitwise negation
        case TokenID::TILDE:
        {
            ptr = std::make_unique<UnaryExpr>();

            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::BIT_NOT;

            // Consume '++'
            advance();

            reint_ptr->operand = parse_expression(PREFIX_PRECEDENCE);
            break;
        }

        // Address of
        case TokenID::AMPERSAND:
        {
            ptr = std::make_unique<UnaryExpr>();

            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::ADDRESS_OF;

            // Consume '++'
            advance();

            reint_ptr->operand = parse_expression(PREFIX_PRECEDENCE);
            break;
        }

        // Dereference
        case TokenID::ASTERISK:
        {
            ptr = std::make_unique<UnaryExpr>();
            
            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::DEREF;

            // Consume '*'
            advance();

            reint_ptr->operand = parse_expression(PREFIX_PRECEDENCE);

            break;
        }

        case TokenID::LPAREN: return parse_paren_or_cast();

        default:
            print_error_location(peek().line, peek().col);
            std::cout << " -> Invalid expression, unexpected token: " << 
                peek().id << '\n';
            exit(1);
    }

    ptr->line = start_line;
    ptr->col = start_col;
    
    return ptr;
}

std::unique_ptr<Expression> Parser::parse_paren_or_cast()
{
    expect(TokenID::LPAREN);

    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    const uint64_t saved_tok_idx = current_token_idx;

    // Attempt to parse as a type
    std::unique_ptr<TypeDecl> type_result = parse_type_decl(false);

    // Successfully parsed a type, this is a cast.
    if(type_result != nullptr)
    {
        std::unique_ptr<CastExpr> ptr = std::make_unique<CastExpr>();
            
        ptr->to_cast_type = std::move(type_result);
        ptr->line = start_line;
        ptr->col = start_col;
        expect(TokenID::RPAREN);
        ptr->expr_to_cast = parse_expression(PREFIX_PRECEDENCE);
        return ptr;
    }

    // Not a cast, this is a parenthesized expression.

    rewind(saved_tok_idx);

    std::unique_ptr<Expression> ptr = parse_expression();

    expect(TokenID::RPAREN);

    return ptr;
}


// std::unique_ptr<Expression> Parser::parse_expression(
//     std::initializer_list<TokenID> delimeters)
// {
//     std::unique_ptr<Expression> result = parse_assignment(nullptr);

//     bool hit_delimiter = false;

//     while(true)
//     {
//         for(TokenID id : delimeters)
//         {
//             if(peek().id == id)
//             {
//                 hit_delimiter = true;
//                 break;
//             }
//         }

//         if(hit_delimiter) break;

//         uint64_t start_tok_idx = current_token_idx;
    
//         result = parse_assignment(std::move(result));
   
//         if(current_token_idx == start_tok_idx)
//         {
//             print_error_location(peek().line, peek().col);
//             std::cout << " -> Expected Token of type: ";

//             auto del_begin = delimeters.begin();

//             while(del_begin != delimeters.end())
//             {
//                 std::cout << *del_begin;

//                 ++del_begin;

//                 if(del_begin != delimeters.end())
//                 {
//                     std::cout << " or ";
//                 }
//             }

//             std::cout << " but got token: \n" << peek() << '\n';
//             exit(1);
//         }
//     }

//     return result;

//     // return parse_assignment(nullptr);
// }

// std::unique_ptr<Expression> Parser::parse_assignment(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking for assignment\n";

//     bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         pre_expr_null = true;
//         // std::cout << "pre_expr was nullptr\n";
//         pre_expr = parse_log_or(nullptr);
//     }

//     AssignOp op = AssignOp::INVALID;

//     if(consume_if(TokenID::ASSIGN)) op = AssignOp::ASSIGN;
//     else if(consume_if(TokenID::PLUS_ASSIGN)) op = AssignOp::ADD_ASSIGN;
//     else if(consume_if(TokenID::MINUS_ASSIGN)) op = AssignOp::SUB_ASSIGN;
//     else if(consume_if(TokenID::ASTERISK_ASSIGN)) op = AssignOp::MUL_ASSIGN;
//     else if(consume_if(TokenID::FORW_SLASH_ASSIGN)) op = AssignOp::DIV_ASSIGN;
//     else if(consume_if(TokenID::MODULO_ASSIGN)) op = AssignOp::MOD_ASSIGN;
//     else return pre_expr_null ? std::move(pre_expr) : 
//         parse_log_or(std::move(pre_expr));

//     auto full_expr = std::make_unique<AssignExpr>();

//     full_expr->line = start_line;
//     full_expr->col = start_col;
//     full_expr->op_type = op;
    
//     full_expr->lhs = std::move(pre_expr);
//     full_expr->rhs = parse_expression();

//     return full_expr;
// }

// std::unique_ptr<Expression> Parser::parse_log_or(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking for log_or\n";

//     bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         pre_expr_null = true;
//         pre_expr = parse_log_and(std::move(pre_expr));
//     } 

//     while(true)
//     {
//         if(!consume_if(TokenID::LOGIC_OR)) break;

//         auto full_expr = std::make_unique<BinaryExpr>();

//         full_expr->line = start_line;
//         full_expr->col = start_col;
//         full_expr->op_type = BinaryOp::LOG_OR;
//         full_expr->lhs = std::move(pre_expr);
//         full_expr->rhs = parse_log_and(nullptr);

//         pre_expr = std::move(full_expr);
//     }

//     return pre_expr_null ? std::move(pre_expr) : 
//         parse_log_and(std::move(pre_expr));
// }

// std::unique_ptr<Expression> Parser::parse_log_and(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking for log_and\n";

//     bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         pre_expr_null = true;
//         pre_expr = parse_equality(nullptr);
//     }

//     while(true)
//     {
//         if(!consume_if(TokenID::LOGIC_AND)) break;

//         auto full_expr = std::make_unique<BinaryExpr>();

//         full_expr->line = start_line;
//         full_expr->col = start_col;
//         full_expr->op_type = BinaryOp::LOG_AND;
//         full_expr->lhs = std::move(pre_expr);
//         full_expr->rhs = parse_equality(nullptr);

//         pre_expr = std::move(full_expr);
//     }

//     return pre_expr_null ? std::move(pre_expr) :
//         parse_equality(std::move(pre_expr));
// }

// std::unique_ptr<Expression> Parser::parse_equality(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking for equality\n";

//     bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         pre_expr_null = true;
//         // std::cout << "pre_expr was nullptr\n";

//         pre_expr = parse_relational(nullptr);
//     }

//     while(true)
//     {
//         BinaryOp op = BinaryOp::INVALID;

//         if(consume_if(TokenID::EQUAL_EQUAL)) op = BinaryOp::EQ;
//         else if(consume_if(TokenID::NOT_EQUAL)) op = BinaryOp::NE;
//         else break;

//         auto full_expr = std::make_unique<BinaryExpr>();

//         full_expr->line = start_line;
//         full_expr->col = start_col;
//         full_expr->op_type = op;
//         full_expr->lhs = std::move(pre_expr);
//         full_expr->rhs = parse_relational(nullptr);

//         // pre_expr = std::move(full_expr);
//         return full_expr;
//     }

//     return pre_expr_null ? std::move(pre_expr) : 
//         parse_relational(std::move(pre_expr));
// }

// std::unique_ptr<Expression> Parser::parse_relational(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // std::cout << "Checking relational: ";
//     // print_error_location(peek().line, peek().col);
//     // std::cout << '\n';

//     bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         pre_expr_null = true;
//         pre_expr = parse_additive(nullptr);
//     }

//     while(true)
//     {
//         BinaryOp op = BinaryOp::INVALID;

//         if(consume_if(TokenID::LESS_THAN)) op = BinaryOp::LT;
//         else if(consume_if(TokenID::GREATER_THAN)) op = BinaryOp::GT;
//         else if(consume_if(TokenID::LESS_EQUAL)) op = BinaryOp::LE;
//         else if(consume_if(TokenID::GREATER_EQUAL)) op = BinaryOp::GE;
//         else break;

//         auto full_expr = std::make_unique<BinaryExpr>();

//         full_expr->line = start_line;
//         full_expr->col = start_col;
//         full_expr->op_type = op;
//         full_expr->lhs = std::move(pre_expr);
//         full_expr->rhs = parse_additive(nullptr);

//         pre_expr = std::move(full_expr);
//     }

//     return pre_expr_null ? std::move(pre_expr) : 
//         parse_additive(std::move(pre_expr));
// }

// std::unique_ptr<Expression> Parser::parse_additive(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking additive\n";

//     bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         pre_expr_null = true;

//         // std::cout << "pre_expr was nullptr\n";
//         pre_expr = parse_multiplicative(nullptr);
//         // std::cout << "After getting pre expr, we are at: ";
//         // print_error_location(peek().line, peek().col);
//         // std::cout << '\n';
//     }

//     while(true)
//     {
//         BinaryOp op = BinaryOp::INVALID;

//         if(consume_if(TokenID::PLUS)) op = BinaryOp::ADD;
//         else if(consume_if(TokenID::MINUS)) op = BinaryOp::SUB;
//         else break; // Did not find a proper operator, we're done.
        
//         auto full_expr = std::make_unique<BinaryExpr>();

//         full_expr->line = start_line;
//         full_expr->col = start_col;
//         full_expr->op_type = op;
//         full_expr->lhs = std::move(pre_expr);
        
//         full_expr->rhs = parse_multiplicative(nullptr);
//         pre_expr = std::move(full_expr);
//     }

//     return pre_expr_null ? std::move(pre_expr) : 
//         parse_multiplicative(std::move(pre_expr));   
// }

// std::unique_ptr<Expression> Parser::parse_multiplicative(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking multi\n";

//     bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         pre_expr_null = true;

//         // std::cout << "pre_expr was nullptr\n";
//         pre_expr = parse_unary(nullptr);
//     }

//     while(true)
//     {
//         BinaryOp op = BinaryOp::INVALID;

//         if(consume_if(TokenID::ASTERISK)) op = BinaryOp::MUL;
//         else if(consume_if(TokenID::FORW_SLASH)) op = BinaryOp::DIV;
//         else if(consume_if(TokenID::MODULO)) op = BinaryOp::MOD;
//         else break; // Did not find a proper operator, we're done.
        
//         auto full_expr = std::make_unique<BinaryExpr>();

//         full_expr->line = start_line;
//         full_expr->col = start_col;
//         full_expr->op_type = op;
//         full_expr->lhs = std::move(pre_expr);
//         full_expr->rhs = parse_unary(nullptr);
//         pre_expr = std::move(full_expr);
//     }

//     return pre_expr_null ? std::move(pre_expr) : 
//         parse_unary(std::move(pre_expr));
// }

// std::unique_ptr<Expression> Parser::parse_unary(
//     std::unique_ptr<Expression> pre_expr)
// {
//     // print_error_location(peek().line, peek().col);
//     // std::cout << " -> Checking unary\n";
    
//     if(check(TokenID::PLUS_PLUS))
//     {   
//         if(pre_expr != nullptr)
//         {
//             print_error_location(peek().line, peek().col);
//             std::cout << " -> PRE INC can't have pre expression?\n";
//             exit(1);
        
//             // std::cout << "unary pre_expr was nullptr\n";
//             return parse_postfix(std::move(pre_expr));
//         }

//         Token t = expect(TokenID::PLUS_PLUS);

//         auto ptr = std::make_unique<UnaryExpr>();
        
//         ptr->op_type = UnaryOp::PRE_INC;
//         ptr->operand = parse_unary(nullptr);
//         ptr->line = t.line;
//         ptr->col = t.col;
//         return ptr;
//     }

//     // Pre Dec
//     else if(check(TokenID::MINUS_MINUS))
//     {
//         if(pre_expr != nullptr)
//         {
//             print_error_location(peek().line, peek().col);
//             std::cout << " -> PRE DEC can't have pre expression?\n";
//             exit(1);

//             return parse_postfix(std::move(pre_expr));
//         }

//         Token t = expect(TokenID::MINUS_MINUS);

//         auto ptr = std::make_unique<UnaryExpr>();
        
//         ptr->op_type = UnaryOp::PRE_DEC;
//         ptr->operand = parse_unary(nullptr);
//         ptr->line = t.line;
//         ptr->col = t.col;
//         return ptr;
//     }

//     // Numerical Negation
//     else if(check(TokenID::MINUS))
//     {
//         Token t = expect(TokenID::MINUS);

//         auto ptr = std::make_unique<UnaryExpr>();
        
//         ptr->op_type = UnaryOp::NEGATE;
//         if(pre_expr == nullptr)
//         {
//             ptr->operand = parse_unary(nullptr);
//         }
//         // else ptr->operand = std::move(pre_expr);
//         else
//         {
//             print_error_location(t.line, t.col);
//             std::cout << " -> NEGATE can't have pre expression?\n";
//             exit(1);
//         }
//         ptr->line = t.line;
//         ptr->col = t.col;
//         return ptr;
//     }

//     // Logical Negation
//     else if(check(TokenID::EXCLAMATION_POINT))
//     {
//         Token t = expect(TokenID::EXCLAMATION_POINT);

//         auto ptr = std::make_unique<UnaryExpr>();
        
//         ptr->op_type = UnaryOp::LOG_NOT;
//         if(pre_expr == nullptr)
//         {
//             ptr->operand = parse_unary(nullptr);
//         }
//         // else ptr->operand = std::move(pre_expr);
//         else
//         {
//             print_error_location(t.line, t.col);
//             std::cout << " -> LOG_NOT can't have pre expression?\n";
//             exit(1);
//         }
//         ptr->line = t.line;
//         ptr->col = t.col;
//         return ptr;
//     }

//     else if(check(TokenID::TILDE))
//     {
//         Token t = expect(TokenID::TILDE);

//         auto ptr = std::make_unique<UnaryExpr>();
        
//         ptr->op_type = UnaryOp::BIT_NOT;
//         if(pre_expr == nullptr)
//         {
//             ptr->operand = parse_unary(nullptr);
//         }
//         // else ptr->operand = std::move(pre_expr);
//         else
//         {
//             print_error_location(t.line, t.col);
//             std::cout << " -> TILDE can't have pre expression?\n";
//             exit(1);
//         }
//         ptr->line = t.line;
//         ptr->col = t.col;
//         return ptr;
//     }

//     else if(check(TokenID::AMPERSAND))
//     {
//         Token t = expect(TokenID::AMPERSAND);

//         auto ptr = std::make_unique<UnaryExpr>();
        
//         ptr->op_type = UnaryOp::ADDRESS_OF;
//         if(pre_expr == nullptr)
//         {
//             ptr->operand = parse_unary(nullptr);
//         }
//         // else ptr->operand = std::move(pre_expr);
//         else
//         {
//             print_error_location(t.line, t.col);
//             std::cout << " -> AMPERSAND can't have pre expression?\n";
//             exit(1);
//         }
//         ptr->line = t.line;
//         ptr->col = t.col;
//         return ptr;
//     }

//     else if(check(TokenID::ASTERISK))
//     {
//         Token t = expect(TokenID::ASTERISK);

//         auto ptr = std::make_unique<UnaryExpr>();
        
//         ptr->op_type = UnaryOp::DEREF;
//         if(pre_expr == nullptr)
//         {
//             ptr->operand = parse_unary(nullptr);
//         }
//         // else ptr->operand = std::move(pre_expr);
//         else
//         {
//             print_error_location(t.line, t.col);
//             std::cout << " -> DEREF can't have pre expression?\n";
//             exit(1);
//         }
//         ptr->line = t.line;
//         ptr->col = t.col;
//         return ptr;
//     }

//     // Parenthesized expression.
//     else if(check(TokenID::LPAREN) && pre_expr == nullptr)
//     {
//         Token t = expect(TokenID::LPAREN);

//         uint64_t saved_tok_idx = current_token_idx;

//         // Attempt to parse a type with error on failure as false. 
//         auto type_ptr = parse_type_decl(false);

//         // If the ptr was not returned as nullptr, we have successfully parsed
//         // a type, and this is a cast
//         if(type_ptr != nullptr) 
//         {
//             auto expr_ptr = std::make_unique<CastExpr>();
            
//             expr_ptr->to_cast_type = std::move(type_ptr); 
//             expr_ptr->line = t.line;
//             expr_ptr->col = t.col;
//             expect(TokenID::RPAREN);
//             if(pre_expr == nullptr)
//             {
//                 expr_ptr->expr_to_cast = parse_unary(nullptr);
//             }
//             else expr_ptr->expr_to_cast = std::move(pre_expr);
//             return expr_ptr;
//         }

//         // If the type parsing failed, rewind to the token before we starting
//         // attempting to consume the non type.
//         rewind(saved_tok_idx);

//         std::unique_ptr<Expression> full_expr = 
//             parse_expression();

//         // expect(TokenID::RPAREN);

//         // while(!consume_if(TokenID::RPAREN))
//         // {
//         //     full_expr = parse_assignment(std::move(full_expr));
//         //     // std::cin.get();
//         // }

//         expect(TokenID::RPAREN);

//         return full_expr;

//     }

//     return parse_postfix(std::move(pre_expr));
// }

// std::unique_ptr<Expression> Parser::parse_postfix(
//     std::unique_ptr<Expression> pre_expr)
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking postfix\n";

//     // bool pre_expr_null = false;

//     if(pre_expr == nullptr)
//     {
//         // pre_expr_null = true;

//         // Expression that came before the postfix.
//         pre_expr = parse_primary();
//         // return parse_primary();
//     }

//     while(true)
//     {
//         // Function call
//         if(consume_if(TokenID::LPAREN))
//         {
//             std::unique_ptr<CallExpr> full_expr = 
//                 std::make_unique<CallExpr>();

//             full_expr->line = start_line;
//             full_expr->col = start_col;

//             // Move the pre expression into the full expression's callee expr
//             full_expr->callee_expr = std::move(pre_expr);
            
//             // Parse arguments
//             while(!check(TokenID::RPAREN))
//             {
//                 full_expr->args.push_back(parse_expression(
//                     {TokenID::COMMA, TokenID::RPAREN}));

//                 // Trailing comma
//                 if(consume_if(TokenID::COMMA) && check(TokenID::RPAREN))
//                 {
//                     print_error_location(peek().line, peek().col);
//                     std::cout << " -> Trailing comma in argument list\n";
//                     exit(1);
//                 }
//             }

//             expect(TokenID::RPAREN);

//             // Move the full expr now into the pre expr so we can continue 
//             // parsing any further postfix operations with this as the pre expr.
//             pre_expr = std::move(full_expr);
//         }

//         // Subscript
//         else if(consume_if(TokenID::LBRACKET))
//         {
//             std::unique_ptr<SubscriptExpr> full_expr = 
//                 std::make_unique<SubscriptExpr>();
            
//             full_expr->line = start_line;
//             full_expr->col = start_col;

//             // Move the pre expression into the full expression's base expr
//             full_expr->base_expr = std::move(pre_expr);

//             // Get the expression for the index. 
//             full_expr->index_expr = parse_expression({TokenID::RBRACKET});

//             // while(!consume_if(TokenID::RBRACKET))
//             // {
//             //     full_expr->index_expr = parse_assignment(
//             //         std::move(full_expr->index_expr));
//             // }

//             expect(TokenID::RBRACKET);
        
//             pre_expr = std::move(full_expr);
//         }

//         // Member access by dot
//         else if(consume_if(TokenID::DOT))
//         {
//             std::unique_ptr<MemberAccExpr> full_expr = 
//                 std::make_unique<MemberAccExpr>();

//             full_expr->line = start_line;
//             full_expr->col = start_col;
//             full_expr->base_expr = std::move(pre_expr);
//             full_expr->member_name = expect(TokenID::IDENTIFIER).text;
//             full_expr->via_pointer = false;

//             pre_expr = std::move(full_expr);
//         }

//         // Member access by arrow
//         else if(consume_if(TokenID::ARROW))
//         {
//             std::unique_ptr<MemberAccExpr> full_expr =
//                 std::make_unique<MemberAccExpr>();
                
//             full_expr->line = start_line;
//             full_expr->col = start_col;
//             full_expr->base_expr = std::move(pre_expr);
//             full_expr->member_name = expect(TokenID::IDENTIFIER).text;
//             full_expr->via_pointer = true;

//             pre_expr = std::move(full_expr);
//         }

//         // Post increment
//         else if(consume_if(TokenID::PLUS_PLUS))
//         {
//             std::unique_ptr<UnaryExpr> full_expr = 
//                 std::make_unique<UnaryExpr>();

//             full_expr->line = start_line;
//             full_expr->col = start_col;
//             full_expr->op_type = UnaryOp::POST_INC;
//             full_expr->operand = std::move(pre_expr);
            
//             pre_expr = std::move(full_expr);
//         }

//         else if(consume_if(TokenID::MINUS_MINUS))
//         {
//             std::unique_ptr<UnaryExpr> full_expr = 
//                 std::make_unique<UnaryExpr>();

//             full_expr->line = start_line;
//             full_expr->col = start_col;
//             full_expr->op_type = UnaryOp::POST_DEC;
//             full_expr->operand = std::move(pre_expr);

//             pre_expr = std::move(full_expr);
//         }

//         // No more postfix expressions.
//         else break;
//     }

//     // return pre_expr_null ? std::move(pre_expr) : 
//     //     parse_primary();

//     return pre_expr;
// }

// std::unique_ptr<Expression> Parser::parse_primary()
// {
//     uint32_t start_line = peek().line;
//     uint32_t start_col = peek().col;

//     // print_error_location(start_line, start_col);
//     // std::cout << " -> Checking primary\n";

//     if(check(TokenID::INT_LITERAL))
//     {
//         auto ptr = std::make_unique<IntLitExpr>();

//         IntLitExpr * const reint_ptr = static_cast<IntLitExpr*>(ptr.get());

//         ptr->line = start_line;
//         ptr->col = start_col;
//         reint_ptr->value = expect(TokenID::INT_LITERAL).text;
//         return ptr;
//     }

//     if(check(TokenID::FLOAT_LITERAL))
//     {
//         auto ptr = std::make_unique<FloatLitExpr>();

//         FloatLitExpr * const reint_ptr = static_cast<FloatLitExpr*>(ptr.get());

//         ptr->line = peek().line;
//         ptr->col = peek().col;
//         reint_ptr->value = expect(TokenID::FLOAT_LITERAL).text;
//         return ptr;
//     }

//     if(check(TokenID::HEX_LITERAL))
//     {
//         auto ptr = std::make_unique<HexLitExpr>();

//         HexLitExpr * const reint_ptr = static_cast<HexLitExpr*>(ptr.get());

//         ptr->line = peek().line;
//         ptr->col = peek().col;
//         reint_ptr->value = expect(TokenID::HEX_LITERAL).text;
//         return ptr;
//     }

//     if(check(TokenID::BIN_LITERAL))
//     {
//         auto ptr = std::make_unique<BinaryLitExpr>();

//         BinaryLitExpr * const reint_ptr = static_cast<BinaryLitExpr*>(ptr.get());

//         ptr->line = peek().line;
//         ptr->col = peek().col;
//         reint_ptr->value = expect(TokenID::BIN_LITERAL).text;
//         return ptr;
//     }

//     if(check(TokenID::STR_LITERAL))
//     {
//         auto ptr = std::make_unique<StringLitExpr>();

//         StringLitExpr * const reint_ptr = static_cast<StringLitExpr*>(ptr.get());

//         ptr->line = peek().line;
//         ptr->col = peek().col;
//         reint_ptr->value = expect(TokenID::STR_LITERAL).text;
//         return ptr;
//     }

//     if(check(TokenID::CHAR_LITERAL))
//     {
//         auto ptr = std::make_unique<CharLitExpr>();

//         CharLitExpr * const reint_ptr = static_cast<CharLitExpr*>(ptr.get());

//         ptr->line = peek().line;
//         ptr->col = peek().col;
//         reint_ptr->value = expect(TokenID::CHAR_LITERAL).text.at(0);
//         return ptr;
//     }

//     if(check(TokenID::BOOL_LITERAL))
//     {
//         auto ptr = std::make_unique<BoolLitExpr>();

//         BoolLitExpr * const reint_ptr = static_cast<BoolLitExpr*>(ptr.get());

//         ptr->line = peek().line;
//         ptr->col = peek().col;
//         reint_ptr->value = expect(TokenID::BOOL_LITERAL).text == "true";
//         return ptr;
//     }

//     if(check(TokenID::NULLPTR_LITERAL))
//     {
//         auto ptr = std::make_unique<NullptrLitExpr>();

//         ptr->line = peek().line;
//         ptr->col = peek().col;
//         expect(TokenID::NULLPTR_LITERAL);
//         return ptr;
//     }

//     if(consume_if(TokenID::LPAREN))
//     {
//         std::cout << "Shouldn't make it here\n";
//         exit(1);

//         auto ptr = parse_expression();
//         expect(TokenID::RPAREN);

//         return ptr;
//     }

//     if(check(TokenID::IDENTIFIER))
//     {
//         auto ptr = std::make_unique<IdentExpr>();

//         IdentExpr * const reint_ptr = static_cast<IdentExpr*>(ptr.get());

//         ptr->line = peek().line;
//         ptr->col = peek().col;

//         reint_ptr->ident_path.push_back(expect(TokenID::IDENTIFIER).text);

//         while(consume_if(TokenID::SCOPE_RESOLUTION))
//         {
//             reint_ptr->ident_path.push_back(expect(TokenID::IDENTIFIER).text);
//         }

//         return ptr;
//     }

//     print_error_location(start_line, start_col);    
//     std::cout << " -> Invalid expression, got unexpected token: \n" << 
//         peek() << '\n';
//     exit(1);    
// }

std::unique_ptr<TypeDecl> Parser::parse_type_decl_recurse(bool error_on_invalid) 
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

        // if(!check(TokenID::IDENTIFIER)) return nullptr;

        // uint32_t ident_line = peek().line;
        // uint32_t ident_col = peek().col;

        reint_ptr->ident_path.push_back(expect(TokenID::IDENTIFIER).text);
    
        while(consume_if(TokenID::SCOPE_RESOLUTION))
        {
            reint_ptr->ident_path.push_back(expect(TokenID::IDENTIFIER).text);
        }

        // // If the identifier is not a type symbol, this can't be a type.
        // if(defined_types.find(reint_ptr->ident_path.back()) == 
        //     defined_types.end())
        // {
        //     if(error_on_invalid)
        //     {
        //         print_error_location(ident_line, ident_col);
        //         std::cout << " -> Undefined symbol (Parser): \"";
        //         print_ident_path(reint_ptr->ident_path);
        //         std::cout << "\"\n";
        //         exit(1);
        //     }

        //     return nullptr;
        // }
    }

    // Pointer 
    else if(consume_if(TokenID::ASTERISK))
    {
        ptr = std::make_unique<PtrTypeDecl>();
        ptr->kind = TypeKind::PTR;
        PtrTypeDecl *reint_ptr = static_cast<PtrTypeDecl*>(ptr.get());

        reint_ptr->points_to_mutable = consume_if(TokenID::KW_MUT);

        auto pointee = parse_type_decl_recurse(error_on_invalid);

        if(pointee == nullptr) return nullptr;

        reint_ptr->pointee = std::move(pointee);
    }

    // Reference
    else if(consume_if(TokenID::KW_REF))
    {   
        ptr = std::make_unique<RefTypeDecl>();
        ptr->kind = TypeKind::REF;
        RefTypeDecl *reint_ptr = static_cast<RefTypeDecl*>(ptr.get());

        reint_ptr->ref_to_mutable = consume_if(TokenID::KW_MUT);

        auto pointee = parse_type_decl_recurse(error_on_invalid);

        if(pointee == nullptr) return nullptr;

        reint_ptr->referred = std::move(pointee);

        if(reint_ptr->referred->kind == TypeKind::REF)
        {
            print_error_location(start_line, start_col);
            std::cout << " Cannot have a reference of a reference\n";
            exit(1);
        }
    }

    // Function pointer
    else if(consume_if(TokenID::KW_FN))
    {
        ptr = std::make_unique<FuncPtrDecl>();
        ptr->kind = TypeKind::FUNC_PTR;
        FuncPtrDecl *reint_ptr = static_cast<FuncPtrDecl*>(ptr.get());

        if(!check(TokenID::LPAREN)) return nullptr;

        expect(TokenID::LPAREN);

        if(!check(TokenID::LPAREN)) return nullptr;

        expect(TokenID::LPAREN);

        // Parse parameters
        while(true)
        {
            reint_ptr->param_types.push_back(parse_param());

            // If the next token is not a comma, it must be a right paren
            if(!check(TokenID::COMMA))
            {
                if(!check(TokenID::RPAREN)) return nullptr;
                expect(TokenID::RPAREN);
                break;
            }

            expect(TokenID::COMMA);
        }

        if(check(TokenID::ARROW)) return nullptr;
        expect(TokenID::ARROW);

        // Read return type

        auto ret_type = parse_type_decl(error_on_invalid);

        if(ret_type == nullptr) return nullptr;

        reint_ptr->ret_type = std::move(ret_type);
    }

    else return nullptr;

    ptr->line = start_line;
    ptr->col = start_col;

    return ptr;
}

std::unique_ptr<TypeDecl> Parser::parse_type_decl(
    bool error_on_invalid)
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    std::unique_ptr<TypeDecl> parsed_type = 
        parse_type_decl_recurse(error_on_invalid);

    // If the first time we parsed a type was a nullptr, we failed to parse
    if(parsed_type == nullptr)
    {
        if(error_on_invalid)
        {
            print_error_location(start_line, start_col);
            std::cout << " -> Invalid type declaration.\n";
            exit(1);
        }

        return nullptr; // Return nullptr to signal we failed to parse type decl
    }

    // After parsing the type declaration, check if this is an array of that 
    // type.
    if(!check(TokenID::LBRACKET)) return parsed_type;

    std::unique_ptr<ArrTypeDecl> arr_decl = std::make_unique<ArrTypeDecl>();
    arr_decl->line = parsed_type->line;
    arr_decl->col = parsed_type->col;
    arr_decl->element_type = std::move(parsed_type);

    while(consume_if(TokenID::LBRACKET))
    {
        arr_decl->size_exprs.push_back(parse_expression());
        ++arr_decl->depth;
        expect(TokenID::RBRACKET);
    }

    return arr_decl;
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

    // This field has an init expression
    if(consume_if(TokenID::ASSIGN))
    {
        ptr->init_expr = parse_expression();
    }

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
        if(peek().id == TokenID::END_OF_FILE)
        {
            print_missing_brace(peek().line, peek().col, body.line, body.col);
            exit(1);
        }

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
            std::cout << " -> Reference parameters cannot be passed in by val.\n";
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

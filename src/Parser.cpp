// Parser.cpp

#include "Parser.hpp"

#include <charconv>

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

    prog.func_sigs = std::move(func_sigs);

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
    std::cout << " -> Expected token of type: " << 
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


std::optional<uint64_t> Parser::expr_to_u64(const Expression *ptr) 
{
    if(ptr->exp_type != ExpressionType::INT_LITERAL) return {};

    const IntLitExpr *reint_ptr = static_cast<const IntLitExpr*>(ptr);

    std::from_chars_result result;
    uint64_t output = 0;

    result = std::from_chars(reint_ptr->value.data(), 
        reint_ptr->value.data() + reint_ptr->value.size(), output);

    const bool success = 
        result.ec == std::errc() && result.ptr == reint_ptr->value.data() + 
        reint_ptr->value.size();

    return success ? output : std::optional<uint64_t>{};
}


std::string Parser::type_to_rdbl(TypeDecl *ptr)
{
    std::string res;

    switch(ptr->kind)
    {
        case TypeKind::ARRAY:
        {
            const ArrTypeDecl *reint_ptr = 
                static_cast<const ArrTypeDecl *>(ptr); 

            res.push_back('[');
            res += type_to_rdbl(reint_ptr->element_type.get());
            res.push_back(']');

            for(uint64_t i = 0; i < reint_ptr->size_exprs.size(); ++i)
            {
                res.push_back('[');
                res += std::to_string(reint_ptr->size_expr_as_num.at(i));
                res.push_back(']');
            }
            break;
        }

        case TypeKind::FUNC_PTR:

            break;

        case TypeKind::NAMED:
        {
            const NamedTypeDecl *reint_ptr = 
                static_cast<const NamedTypeDecl*>(ptr);

            res += reint_ptr->ident_path.back();
            break;
        }

        case TypeKind::PTR:
        {
            const PtrTypeDecl *reint_ptr = 
                static_cast<const PtrTypeDecl*>(ptr);

            res += "ptr";
            if(reint_ptr->points_to_mutable) res += "mut";
            res.push_back('_');

            res += type_to_rdbl(reint_ptr->pointee.get());
            break;
        }

        case TypeKind::REF:
        {
            const RefTypeDecl *reint_ptr =  
                static_cast<const RefTypeDecl*>(ptr);

            res += "ref";
            if(reint_ptr->ref_to_mutable) res += "mut";
            res.push_back('_');

            res += type_to_rdbl(reint_ptr->referred.get());
            break;
        }

        default:

            print_error_location(ptr->line, ptr->col);
            std::cout << " -> Invalid TypeKind found.\n";
            exit(1);
    }

    return res;
}

std::unique_ptr<Declaration> Parser::parse_top_level() 
{
    Token t = peek();
    
    bool is_pub = false;

    if(consume_if(TokenID::KW_PUB)) is_pub = true;

    if(check(TokenID::END_OF_FILE)) return nullptr;

    if(check(TokenID::KW_MODULE)) 
    {
        if(is_pub)
        {
            std::cout << "Sub Modules cannot be marked \"export\"\n";
            exit(1);
        }

        return parse_module();
    }

    if(check(TokenID::KW_FN)) return parse_function(is_pub);

    if(consume_if(TokenID::KW_TYPE)) 
    {
        if(check(TokenID::KW_STRUCT)) return parse_struct(is_pub);
        if(check(TokenID::KW_COMPONENT)) return parse_component(is_pub);
        
        print_error_location(peek().line, peek().col);
        std::cout << " -> \"type\" not followed by a declaration of a struct or "
            "component.\n";
        exit(1);
    }

    // Global variable
    if(check(TokenID::IDENTIFIER) || check(TokenID::KW_MUT)) 
        return parse_field(is_pub);

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
        // p.is_unqual_param = false;
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

    std::string func_as_rdbl;

    

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

    //                                     Here v 
    // If we have a nested Arr expression, ie { {1, 2}, {1, 2}}
    if(check(TokenID::LBRACE))
    {
        while(true)
        {
            ptr->init_args.push_back(parse_arr_init());
            if(!consume_if(TokenID::COMMA)) break;
        }
    }

    // Parse as an expression
    else
    {
        while(true)
        {
            ptr->init_args.push_back(parse_expression());
            if(!consume_if(TokenID::COMMA)) break;
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

        if(tok_id == TokenID::PLUS_PLUS || tok_id == TokenID::MINUS_MINUS)
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
        {TokenID::AMPERSAND, BinaryOp::BIT_AND},
        {TokenID::VERT_BAR, BinaryOp::BIT_OR},
        {TokenID::CARROT, BinaryOp::BIT_XOR},
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
    std::unique_ptr<SubscriptExpr> ptr = std::make_unique<SubscriptExpr>();

    ptr->line = lhs->line;
    ptr->col = lhs->col;

    ptr->base_expr = std::move(lhs);

    if(!check(TokenID::LBRACKET))
    {
        handle_tok_mismatch(peek(), TokenID::LBRACKET);
        exit(1);
    }

    while(consume_if(TokenID::LBRACKET))
    {
        ptr->index_exprs.push_back(parse_expression());
        expect(TokenID::RBRACKET);
    }

    // expect(TokenID::LBRACKET);

    // std::unique_ptr<SubscriptExpr> ptr = std::make_unique<SubscriptExpr>();

    // ptr->line = lhs->line;
    // ptr->col = lhs->col;

    // ptr->base_expr = std::move(lhs);
    // ptr->index_expr = parse_expression();
    // expect(TokenID::RBRACKET);

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

        // Reference of
        case TokenID::DOLLAR_SIGN:
        {
            ptr = std::make_unique<UnaryExpr>();

            UnaryExpr * reint_ptr = static_cast<UnaryExpr*>(ptr.get());

            reint_ptr->op_type = UnaryOp::REF_OF;

            // Consume '$'
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

        // case TokenID::LPAREN: return parse_paren_or_cast();  
        case TokenID::LPAREN:
        {   
            expect(TokenID::LPAREN);

            // Cast expression.
            if(peek().id == TokenID::COMPILE_DIRECTIVE)
            {
                std::unique_ptr<CastExpr> cast_expr = parse_cast();
                cast_expr->line = start_line;
                cast_expr->col = start_col;
                return cast_expr;
            }

            ptr = std::make_unique<Expression>();
            ptr = parse_expression();
        
            expect(TokenID::RPAREN);

            break;
        }

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

std::unique_ptr<CastExpr> Parser::parse_cast()
{
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;

    expect(TokenID::COMPILE_DIRECTIVE);
    
    const Token &t = peek();
    
    if(t.id != TokenID::IDENTIFIER)
    {
        print_error_location(t.line, t.col);
        std::cerr << ": Expected compile directive.\n";
        exit(1);
    }

    if(t.text != "cast")
    {
        print_error_location(t.line, t.col);
        std::cerr << ": Invalid compile directive.\n";
        exit(1);
    }

    expect(TokenID::IDENTIFIER);

    expect(TokenID::LESS_THAN);

    std::unique_ptr<TypeDecl> type_decl = parse_type_decl();

    expect(TokenID::GREATER_THAN);

    expect(TokenID::RPAREN);

    std::unique_ptr<CastExpr> ptr = std::make_unique<CastExpr>();

    ptr->line = start_line;
    ptr->col = start_col;
    ptr->to_cast_type = std::move(type_decl);
    ptr->expr_to_cast = parse_expression();

    return ptr;
}

// std::unique_ptr<TypeDecl> Parser::parse_type_decl_recurse(bool error_on_invalid) 
std::unique_ptr<TypeDecl> Parser::parse_type_decl(bool error_on_invalid)
{
    std::unique_ptr<TypeDecl> ptr = nullptr;
    
    uint32_t start_line = peek().line;
    uint32_t start_col = peek().col;
    
    // Consume the global addressing scope if there is one ie "::SomeModule"
    consume_if(TokenID::SCOPE_RESOLUTION);

    // Named 
    if(check(TokenID::IDENTIFIER))
    {
        if(peek().text == "opaque_ptr")
        {
            expect(TokenID::IDENTIFIER);
            ptr = std::make_unique<PtrTypeDecl>();
            
            PtrTypeDecl *reint_ptr = static_cast<PtrTypeDecl*>(ptr.get());

            reint_ptr->builtin_type = BuiltinPtrType::OPAQUE_PTR;
        }

        else
        {
            ptr = std::make_unique<NamedTypeDecl>();
            NamedTypeDecl *reint_ptr = static_cast<NamedTypeDecl*>(ptr.get());

            reint_ptr->ident_path.push_back(expect(TokenID::IDENTIFIER).text);
        
            while(consume_if(TokenID::SCOPE_RESOLUTION))
            {
                reint_ptr->ident_path.push_back(
                    expect(TokenID::IDENTIFIER).text);
            }
        }
    }

    // Pointer 
    else if(consume_if(TokenID::ASTERISK))
    {
        ptr = std::make_unique<PtrTypeDecl>();
        PtrTypeDecl *reint_ptr = static_cast<PtrTypeDecl*>(ptr.get());

        reint_ptr->points_to_mutable = consume_if(TokenID::KW_MUT);

        auto pointee = parse_type_decl(error_on_invalid);

        if(pointee == nullptr) return nullptr;

        reint_ptr->pointee = std::move(pointee);
    }

    // Reference
    else if(consume_if(TokenID::KW_REF))
    {   
        ptr = std::make_unique<RefTypeDecl>();
        RefTypeDecl *reint_ptr = static_cast<RefTypeDecl*>(ptr.get());

        reint_ptr->ref_to_mutable = consume_if(TokenID::KW_MUT);

        auto pointee = parse_type_decl(error_on_invalid);

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

    // Array
    else if(consume_if(TokenID::LBRACKET))
    {   
        ptr = std::make_unique<ArrTypeDecl>();
        ArrTypeDecl *reint_ptr = static_cast<ArrTypeDecl*>(ptr.get());

        reint_ptr->element_type = parse_type_decl();
        
        expect(TokenID::RBRACKET);

        while(consume_if(TokenID::LBRACKET))
        {
            std::unique_ptr<Expression> expr = parse_expression();

            std::optional<uint64_t> expr_as_num = expr_to_u64(expr.get());

            if(!expr_as_num.value())
            {
                print_error_location(expr->line, expr->col);
                std::cout << " -> Array size expression must be an unsigned"
                    " int literal type.\n";
                exit(1);
            }
            
            reint_ptr->size_expr_as_num.push_back(*expr_as_num);
            reint_ptr->size_exprs.push_back(std::move(expr));

            expect(TokenID::RBRACKET);
        }
    }

    else
    {
        if(!error_on_invalid) return nullptr;
        
        print_error_location(peek().line, peek().col);
        std::cout << " -> Unexpected token when parsing type: \n" <<
            peek() << '\n';
        exit(1);
    }

    ptr->line = start_line;
    ptr->col = start_col;

    std::string type_rdbl = type_to_rdbl(ptr.get());

    print_error_location(ptr->line, ptr->col);
    std::cout << " -> Parsed Type: " << type_rdbl << '\n';

    ptr->readable = std::move(type_rdbl);

    return ptr;
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
    // if(check(TokenID::IDENTIFIER))
    // {
    //     p.is_unqual_param = true;
    // }

    // Parameter is passed in as copy
    if(consume_if(TokenID::KW_VAL))
    {
        if(check(TokenID::KW_REF))
        {
            print_error_location(peek().line, peek().col);
            std::cout << " -> Reference parameters cannot be passed in by "
                "val.\n";
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

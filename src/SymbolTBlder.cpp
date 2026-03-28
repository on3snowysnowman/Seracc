// SymbolTBlder.cpp

#include "SymbolTBlder.hpp"

#include <iostream>


// Ctors / Dtor

SymbolTBlder::SymbolTBlder() {}


// Public

SymbolTable SymbolTBlder::build(Program &p) 
{
    symbols.clear();
    scopes.clear();

    parsed_file = p.source_file_name;

    build_top_level(p.ast.get());

    SymbolTable st;
    st.symbols = std::move(symbols);
    st.scopes = std::move(scopes);

    parsed_file = nullptr;

    return st;
}


// Private

void SymbolTBlder::add_symbol_to_scope(uint64_t scope_idx, uint64_t symbol_idx,
    std::string symbol_name, uint32_t symbol_line, uint32_t symbol_col,
    bool is_variable)
{
    // If this is  variable, we don't want to allow duplicate variable 
    // declarations shadowing parent scopes.
    if(is_variable)
    {
        std::optional<uint64_t> parsed_scope_idx = scope_idx;

        while(parsed_scope_idx.has_value())
        {
            // Ensure this symbol name has not been defined already in a parent 
            // scope.
            auto it = 
                scopes.at(parsed_scope_idx.value()).
                    sym_name_to_symbol_idx.find(symbol_name);

            // This symbol has already been defined here.
            if(it != scopes.at(parsed_scope_idx.value()).
                sym_name_to_symbol_idx.end())
            {
                std::cerr << parsed_file << ":" << symbol_line << ":" << 
                    symbol_col << ": Symbol already defined: " << 
                    symbol_name << '\n';
                exit(1);
            }

            parsed_scope_idx = scopes.at(parsed_scope_idx.value()).parent_scope_idx;
        }
    }

    else
    {
        // Ensure this symbol name has not been defined already in the current 
        // scope.
        auto it = 
            scopes.at(scope_idx).sym_name_to_symbol_idx.find(symbol_name);

        // This symbol has already been defined here.
        if(it != scopes.at(scope_idx).sym_name_to_symbol_idx.end())
        {
            std::cerr << parsed_file << ":" << symbol_line << ":" << 
                symbol_col << ": Symbol already defined: " << 
                symbol_name << '\n';
            exit(1);
        }
    }
    // Add our symbol the parent scope's symbol lookup.
    scopes.at(scope_idx).sym_name_to_symbol_idx[symbol_name] = 
        symbol_idx;
}

void SymbolTBlder::add_builtin_symbol(uint64_t global_scope_idx, 
    const std::string &symbol_name, BuiltinType b_type)
{
    uint64_t symbol_idx = get_next_symbol_idx();
    symbols.at(symbol_idx) = std::make_unique<BuiltinSymbol>();
    static_cast<BuiltinSymbol*>(symbols.at(symbol_idx).get())->b_type =
        b_type;
    scopes.at(global_scope_idx).sym_name_to_symbol_idx.emplace(
        symbol_name, symbol_idx);
}

void SymbolTBlder::add_builtin_symbols(uint64_t global_scope_idx)
{   
    for(const auto &elem : readable_to_builtin)
    {
        add_builtin_symbol(global_scope_idx,
            elem.first, elem.second);
    }
}

void SymbolTBlder::build_top_level(ModuleDecl * const ptr)
{
    uint64_t top_symbol_idx = get_next_symbol_idx();
    uint64_t top_scope_idx = get_next_scope_idx();
    
    ptr->symbol_idx = top_symbol_idx;

    symbols.at(top_symbol_idx) = std::make_unique<ModuleSymbol>();

    ModuleSymbol * const mod_sym_ptr = 
        static_cast<ModuleSymbol*>(symbols.at(top_symbol_idx).get());

    mod_sym_ptr->ast_node_ptr = ptr;
    mod_sym_ptr->scope_idx = top_scope_idx;

    // ModuleSymbol does not belong to a scope, therefore it's scope index will
    // be left as optional default constructed.
    
    mod_sym_ptr->created_scope_idx = top_scope_idx;

    add_builtin_symbols(top_scope_idx);

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::MODULE:

                build_module(static_cast<ModuleDecl*>(decl.get()), 
                    top_scope_idx);
                break;

            case DeclKind::STRUCT:

                build_struct(static_cast<StructDecl*>(decl.get()), 
                top_scope_idx);
                break;

            case DeclKind::FUNCTION:

                build_function(static_cast<FunctionDecl*>(decl.get()), 
                    top_scope_idx);
                break;

            case DeclKind::COMPONENT:

                build_component(static_cast<ComponentDecl*>(decl.get()), 
                    top_scope_idx);
                break;

            case DeclKind::FIELD:

                build_field(static_cast<FieldDecl*>(decl.get()),
                    top_scope_idx);
                break;

            default:

                std::cerr << "Symbol Table Builder parsed incorrect "
                    "declaration for module: " << decl->name << '\n';
                exit(1);
        }
    }
}

void SymbolTBlder::build_statement(Statement * const ptr, 
    uint64_t parent_scope_idx)
{
    switch(ptr->stmt_type)
        {
            case StatementType::BLOCK:

                build_scope_body(
                    static_cast<BlockStmt*>(ptr)->block_decl,
                    parent_scope_idx, {});
                break;

            case StatementType::COMPONENT_DECL:

                build_component(
                    static_cast<ComponentDeclStmt*>(ptr)->decl.get(),
                    parent_scope_idx);
                break;

            case StatementType::FOR:
            {
                ForStmt * const for_ptr = static_cast<ForStmt*>(ptr);

                uint64_t for_scope_idx = get_next_scope_idx();

                scopes.at(for_scope_idx).parent_scope_idx = parent_scope_idx;

                // If this for statement has an init declaration, this needs to
                // be added inside the scope body.
                if(for_ptr->init_stmt != nullptr)
                {
                    VarDeclStmt * var_init = 
                        static_cast<VarDeclStmt*>(for_ptr->init_stmt.get());

                    uint64_t var_symbol_idx = get_next_symbol_idx();

                    symbols.at(var_symbol_idx) = std::make_unique<VarSymbol>();

                    VarSymbol * const var_sym_ptr = 
                        static_cast<VarSymbol*>(symbols.at(
                        var_symbol_idx).get());

                    var_sym_ptr->scope_idx = for_scope_idx;
                    var_sym_ptr->ast_node_ptr = var_init;

                    add_symbol_to_scope(for_scope_idx, var_symbol_idx, 
                        var_init->var_name, var_init->line, var_init->col, 
                        true);
                }

                build_scope_body(for_ptr->body, parent_scope_idx, 
                    for_scope_idx);

                break;
            }

            case StatementType::IF:
            {
                IfStmt * const if_ptr = static_cast<IfStmt*>(ptr);

                build_scope_body(if_ptr->then_body, parent_scope_idx, {});

                if(if_ptr->else_branch != nullptr)
                {
                    build_statement(if_ptr->else_branch.get(), 
                        parent_scope_idx);
                }

                break;
            }

            case StatementType::STRUCT_DECL:

                build_struct(
                    static_cast<StructDeclStmt*>(ptr)->decl.get(),
                    parent_scope_idx);
                break; 

            case StatementType::VAR_DECL:
            {
                uint64_t symbol_idx = get_next_symbol_idx();

                VarDeclStmt * const var_decl_ptr = 
                    static_cast<VarDeclStmt*>(ptr);

                var_decl_ptr->symbol_idx = symbol_idx;

                symbols.at(symbol_idx) = std::make_unique<VarSymbol>();

                VarSymbol * const var_sym_ptr =
                    static_cast<VarSymbol*>(symbols.at(symbol_idx).get());

                var_sym_ptr->scope_idx = parent_scope_idx;
                var_sym_ptr->ast_node_ptr = var_decl_ptr;

                add_symbol_to_scope(parent_scope_idx, symbol_idx, 
                    var_decl_ptr->var_name, var_decl_ptr->line, var_decl_ptr->col,
                    true);
                break;
            }

            case StatementType::WHILE:

                build_scope_body(
                    static_cast<WhileStmt*>(ptr)->body, parent_scope_idx, 
                    {});
                break;

            default:

                break;
        }
}

void SymbolTBlder::build_scope_body(ScopeBody &body, 
    uint64_t parent_scope_idx, std::optional<uint64_t> existing_scope)
{
    uint64_t scope_idx;

    if(existing_scope.has_value())
    {
        scope_idx = existing_scope.value();
        // parent_scope_idx of the existing scope should already be set.
    }

    else
    {
        scope_idx = get_next_scope_idx();
        // Set our scope to point to our parent scope.
        scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;
    }

    body.scope_idx = scope_idx;
    
    // Parse statements
    for(const std::unique_ptr<Statement> &stmt : body.statements)
    {
        build_statement(stmt.get(), scope_idx);
    }
}

void SymbolTBlder::build_field(FieldDecl * const ptr, 
    uint64_t parent_scope_idx)
{
    uint64_t symbol_idx = get_next_symbol_idx();

    ptr->symbol_idx = symbol_idx;

    symbols.at(symbol_idx) = std::make_unique<FieldSymbol>();

    FieldSymbol * const field_ptr  = 
        static_cast<FieldSymbol*>(symbols.at(symbol_idx).get());

    field_ptr->scope_idx = parent_scope_idx;
    field_ptr->ast_node_ptr = ptr;

    add_symbol_to_scope(parent_scope_idx, symbol_idx, ptr->name,
        ptr->line, ptr->col);
}

void SymbolTBlder::build_struct(StructDecl * const ptr,
    uint64_t parent_scope_idx)
{
    uint64_t symbol_idx = get_next_symbol_idx();
    uint64_t scope_idx = get_next_scope_idx();

    ptr->symbol_idx = symbol_idx;

    symbols.at(symbol_idx) = std::make_unique<StructSymbol>();

    StructSymbol * const type_ptr = 
        static_cast<StructSymbol*>(symbols.at(symbol_idx).get());

    type_ptr->scope_idx = parent_scope_idx;
    type_ptr->created_scope_idx = scope_idx;
    type_ptr->ast_node_ptr = ptr;

    // Set our scope to point to our parent scope.
    scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;

    add_symbol_to_scope(parent_scope_idx, symbol_idx, ptr->name,
        ptr->line, ptr->col);

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::FIELD:

                build_field(static_cast<FieldDecl*>(decl.get()), scope_idx);
                break;

            case DeclKind::STRUCT:

                build_struct(static_cast<StructDecl*>(decl.get()), scope_idx);
                break;

            default:

                std::cerr << "Symbol Table Builder parsed incorrect "
                    " declaration for struct: " << decl->name << '\n';
                exit(1);
        }
    }
}

void SymbolTBlder::build_function(FunctionDecl * const ptr,
    uint64_t parent_scope_idx)
{
    uint64_t symbol_idx = get_next_symbol_idx();
    // uint64_t scope_idx = get_next_scope_idx();

    ptr->symbol_idx = symbol_idx;
    
    symbols.at(symbol_idx) = std::make_unique<FunctionSymbol>();

    FunctionSymbol * const func_ptr = 
        static_cast<FunctionSymbol*>(symbols.at(symbol_idx).get());

    func_ptr->scope_idx = parent_scope_idx;
    func_ptr->ast_node_ptr = ptr;

    add_symbol_to_scope(parent_scope_idx, symbol_idx, ptr->name,
        ptr->line, ptr->col);

    uint64_t body_scope_idx = get_next_scope_idx();

    scopes.at(body_scope_idx).parent_scope_idx = parent_scope_idx;

    func_ptr->created_scope_idx = body_scope_idx;

    // Parse receiver function if the function has one
    if(ptr->receiver_data.has_value())
    {
        // uint64_t receiver_symbol_idx = get_next_symbol_idx();

        // ptr->receiver_data->symbol_idx = receiver_symbol_idx;

        // symbols.at(receiver_symbol_idx) = std::make_unique<VarSymbol>();

        // VarSymbol * const var_ptr = 
        //     static_cast<VarSymbol*>(symbols.at(receiver_symbol_idx).get());

        // var_ptr->scope_idx = body_scope_idx;

        // add_symbol_to_scope(body_scope_idx, receiver_symbol_idx, 
        //     ptr->receiver_data->receiver_name, ptr->line, ptr->col);

        uint64_t receiver_symbol_idx = get_next_symbol_idx();

        ptr->receiver_data->symbol_idx = receiver_symbol_idx;

        symbols.at(receiver_symbol_idx) = std::make_unique<ParamSymbol>();

        ParamSymbol * const param_sym_ptr = 
            static_cast<ParamSymbol*>(symbols.at(receiver_symbol_idx).get());

        param_sym_ptr->ast_node_ptr = &ptr->receiver_data.value();

        add_symbol_to_scope(body_scope_idx, receiver_symbol_idx,
            ptr->receiver_data->name, ptr->receiver_data->line, 
            ptr->receiver_data->col);
    }

    // Parse parameters
    for(Parameter &p : ptr->params)
    {
        uint64_t param_symbol_idx = get_next_symbol_idx();

        p.symbol_idx.emplace(param_symbol_idx);

        symbols.at(param_symbol_idx) = std::make_unique<ParamSymbol>();

        ParamSymbol * const param_sym_ptr = 
            static_cast<ParamSymbol*>(symbols.at(param_symbol_idx).get());

        param_sym_ptr->scope_idx = body_scope_idx;
        param_sym_ptr->ast_node_ptr = &p;

        add_symbol_to_scope(body_scope_idx, param_symbol_idx, p.name,
            p.line, p.col);
    }

    build_scope_body(ptr->body, parent_scope_idx, body_scope_idx);
}

void SymbolTBlder::build_component(ComponentDecl * const ptr,
    uint64_t parent_scope_idx)
{
    uint64_t symbol_idx = get_next_symbol_idx();
    uint64_t scope_idx = get_next_scope_idx();

    ptr->symbol_idx = symbol_idx;

    symbols.at(symbol_idx) = std::make_unique<ComponentSymbol>();

    ComponentSymbol * const comp_sym_ptr = 
        static_cast<ComponentSymbol*>(symbols.at(symbol_idx).get());

    comp_sym_ptr->scope_idx = parent_scope_idx;
    comp_sym_ptr->created_scope_idx = scope_idx;
    comp_sym_ptr->ast_node_ptr = ptr;

    // Set our scope to point to our parent scope.
    scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;

    add_symbol_to_scope(parent_scope_idx, symbol_idx, 
            ptr->name, ptr->line, ptr->col);

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::FIELD:

                build_field(static_cast<FieldDecl*>(decl.get()), scope_idx);
                break;

            case DeclKind::STRUCT:

                build_struct(static_cast<StructDecl*>(decl.get()), scope_idx);
                break;

            case DeclKind::FUNCTION:

                build_function(static_cast<FunctionDecl*>(decl.get()), 
                    scope_idx);
                break;

            case DeclKind::COMPONENT:

                build_component(static_cast<ComponentDecl*>(decl.get()), 
                    scope_idx);
                break;

            default:

                std::cerr << "Symbol Table Builder parsed incorrect "
                    " declaration for component: " << decl->name << '\n';
                exit(1);
        }
    }
}

void SymbolTBlder::build_module(ModuleDecl * const ptr,
    uint64_t parent_scope_idx)
{
    uint64_t symbol_idx = get_next_symbol_idx();
    uint64_t scope_idx = get_next_scope_idx();

    ptr->symbol_idx = symbol_idx;

    symbols.at(symbol_idx) = std::make_unique<ModuleSymbol>();

    ModuleSymbol * const mod_ptr = 
        static_cast<ModuleSymbol*>(symbols.at(symbol_idx).get());

    mod_ptr->scope_idx = parent_scope_idx;
    mod_ptr->created_scope_idx = scope_idx;
    mod_ptr->ast_node_ptr = ptr;

    // Set our scope to point to our parent scope.
    scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;

    add_symbol_to_scope(parent_scope_idx, symbol_idx, 
            ptr->name, ptr->line, ptr->col);
    
    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::MODULE:

                build_module(static_cast<ModuleDecl*>(decl.get()), 
                    scope_idx);
                break;

            case DeclKind::STRUCT:

                build_struct(static_cast<StructDecl*>(decl.get()), scope_idx);
                break;

            case DeclKind::FUNCTION:

                build_function(static_cast<FunctionDecl*>(decl.get()), 
                    scope_idx);
                break;

            case DeclKind::COMPONENT:

                build_component(static_cast<ComponentDecl*>(decl.get()), 
                    scope_idx);
                break;

            case DeclKind::FIELD:

                build_field(static_cast<FieldDecl*>(decl.get()),
                    scope_idx);
                break;

            default:

                std::cerr << "Symbol Table Builder parsed incorrect "
                    " declaration for module: " << decl->name << '\n';
                exit(1);
        }
    }
}

uint64_t SymbolTBlder::get_next_scope_idx()
{
    scopes.emplace_back();

    return scopes.size() - 1;
}

uint64_t SymbolTBlder::get_next_symbol_idx()
{
    symbols.emplace_back();

    return symbols.size() - 1;
}
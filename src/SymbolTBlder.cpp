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

void SymbolTBlder::build_top_level(ModuleDecl * const ptr)
{
    uint64_t top_scope_idx = get_next_scope_idx();

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

            default:

                std::cerr << "Symbol Table Builder parsed incorrect "
                    " declaration for module: " << decl->name << '\n';
                exit(1);
        }
    }
}

void SymbolTBlder::build_scope_body(ScopeBody &body, uint64_t parent_scope_idx)
{
    uint64_t scope_idx = get_next_scope_idx();

    body.scope_idx = scope_idx;

    // Set our scope to point to our parent scope.
    scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;

    // Parse statements
    for(const std::unique_ptr<Statement> &stmt : body.statements)
    {
        switch(stmt->stmt_type)
        {

            case StatementType::BLOCK:

                build_scope_body(
                    static_cast<BlockStmt*>(stmt.get())->block_decl,
                    scope_idx);
                break;

            case StatementType::COMPONENT_DECL:

                build_component(
                    static_cast<ComponentDeclStmt*>(stmt.get())->decl.get(),
                    scope_idx);
                break;

            case StatementType::FOR:

                build_scope_body(
                    static_cast<ForStmt*>(stmt.get())->body,
                    parent_scope_idx
                );
                break;

            case StatementType::IF:
            {
                IfStmt * const if_ptr = static_cast<IfStmt*>(stmt.get());

                build_scope_body(if_ptr->then_body, parent_scope_idx);

                if(if_ptr->else_branch != nullptr)
                {
                    build_scope_body(static_cast<IfStmt*>(
                            if_ptr->else_branch.get())->then_body,
                        parent_scope_idx);
                }

                break;
            }

            case StatementType::STRUCT_DECL:

                build_struct(
                    static_cast<StructDeclStmt*>(stmt.get())->decl.get(),
                    scope_idx);
                break; 

            case StatementType::VAR_DECL:
            {
                uint64_t symbol_idx = get_next_symbol_idx();

                VarDeclStmt * const var_ptr = 
                    static_cast<VarDeclStmt*>(stmt.get());

                var_ptr->symbol_idx = symbol_idx;

                symbols.at(symbol_idx) = std::make_unique<VarSymbol>();

                symbols.at(symbol_idx)->scope_idx = scope_idx;
                break;
            }

            case StatementType::WHILE:

                build_scope_body(
                    static_cast<WhileStmt*>(stmt.get())->body,
                    parent_scope_idx
                );
                break;

            default:

                break;
        }
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

    // Ensure this symbol name has not been defined already in the parent scope.
    auto it = 
        scopes.at(parent_scope_idx).sym_name_to_symbol_idx.find(ptr->name);

    // This symbol has already been defined here.
    if(it != scopes.at(parent_scope_idx).sym_name_to_symbol_idx.end())
    {
        std::cerr << parsed_file << ":" << ptr->line << ":" << ptr->col << 
            ": Symbol already defined: " << ptr->name << '\n';
        exit(1);
    }

    // Add our symbol the parent scope's symbol lookup.
    scopes.at(parent_scope_idx).sym_name_to_symbol_idx[ptr->name] = 
        symbol_idx;
}

void SymbolTBlder::build_struct(StructDecl * const ptr,
    uint64_t parent_scope_idx)
{
    uint64_t symbol_idx = get_next_symbol_idx();
    uint64_t scope_idx = get_next_scope_idx();

    ptr->symbol_idx = symbol_idx;

    symbols.at(symbol_idx) = std::make_unique<TypeSymbol>();

    TypeSymbol * const type_ptr = 
        static_cast<TypeSymbol*>(symbols.at(symbol_idx).get());

    type_ptr->scope_idx = parent_scope_idx;
    type_ptr->created_scope_idx = scope_idx;

    // Set our scope to point to our parent scope.
    scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;

    // Ensure this symbol name has not been defined already in the parent scope.
    auto it = 
        scopes.at(parent_scope_idx).sym_name_to_symbol_idx.find(ptr->name);

    // This symbol has already been defined here.
    if(it != scopes.at(parent_scope_idx).sym_name_to_symbol_idx.end())
    {
        std::cerr << parsed_file << ":" << ptr->line << ":" << ptr->col << 
            ": Symbol already defined: " << ptr->name << '\n';
        exit(1);
    }

    // Add our symbol the parent scope's symbol lookup.
    scopes.at(parent_scope_idx).sym_name_to_symbol_idx[ptr->name] = 
        symbol_idx;

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
    uint64_t scope_idx = get_next_scope_idx();

    ptr->symbol_idx = symbol_idx;
    ptr->body.scope_idx = scope_idx;
    
    symbols.at(symbol_idx) = std::make_unique<FunctionSymbol>();

    FunctionSymbol * const func_ptr = 
        static_cast<FunctionSymbol*>(symbols.at(symbol_idx).get());

    func_ptr->scope_idx = parent_scope_idx;
    func_ptr->created_scope_idx = scope_idx;


    // Ensure this symbol name has not been defined already in the parent scope.
    auto it = 
        scopes.at(parent_scope_idx).sym_name_to_symbol_idx.find(ptr->name);

    // This symbol has already been defined here.
    if(it != scopes.at(parent_scope_idx).sym_name_to_symbol_idx.end())
    {
        std::cerr << parsed_file << ":" << ptr->line << ":" << ptr->col << 
            ": Symbol already defined: " << ptr->name << '\n';
        exit(1);
    }

    // Add our symbol the parent scope's symbol lookup.
    scopes.at(parent_scope_idx).sym_name_to_symbol_idx[ptr->name] = 
        symbol_idx;

    // Parse receiver function if the function has one
    if(ptr->receiver_data.has_value())
    {
        uint64_t receiver_symbol_idx = get_next_symbol_idx();

        ptr->receiver_data->symbol_idx = receiver_symbol_idx;

        symbols.at(receiver_symbol_idx) = std::make_unique<VarSymbol>();

        VarSymbol * const var_ptr = 
            static_cast<VarSymbol*>(symbols.at(receiver_symbol_idx).get());

        var_ptr->scope_idx = parent_scope_idx;
    }

    // Parse parameters
    for(Parameter &p : ptr->params)
    {
        uint64_t param_symbol_idx = get_next_symbol_idx();

        p.symbol_idx.value() = param_symbol_idx;

        symbols.at(param_symbol_idx) = std::make_unique<VarSymbol>();

        symbols.at(param_symbol_idx)->scope_idx = parent_scope_idx;
    }

    build_scope_body(ptr->body, parent_scope_idx);
}

void SymbolTBlder::build_component(ComponentDecl * const ptr,
    uint64_t parent_scope_idx)
{
    uint64_t symbol_idx = get_next_symbol_idx();
    uint64_t scope_idx = get_next_scope_idx();

    ptr->symbol_idx = symbol_idx;

    symbols.at(symbol_idx) = std::make_unique<TypeSymbol>();

    TypeSymbol * const type_ptr = 
        static_cast<TypeSymbol*>(symbols.at(symbol_idx).get());

    type_ptr->scope_idx = parent_scope_idx;
    type_ptr->created_scope_idx = scope_idx;

    // Set our scope to point to our parent scope.
    scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;

    // Ensure this symbol name has not been defined already in the parent scope.
    auto it = 
        scopes.at(parent_scope_idx).sym_name_to_symbol_idx.find(ptr->name);

    // This symbol has already been defined here.
    if(it != scopes.at(parent_scope_idx).sym_name_to_symbol_idx.end())
    {
        std::cerr << parsed_file << ":" << ptr->line << ":" << ptr->col << 
            ": Symbol already defined: " << ptr->name << '\n';
        exit(1);
    }

    // Add our symbol the parent scope's symbol lookup.
    scopes.at(parent_scope_idx).sym_name_to_symbol_idx[ptr->name] = 
        symbol_idx;

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

    // Set our scope to point to our parent scope.
    scopes.at(scope_idx).parent_scope_idx = parent_scope_idx;

    // Ensure this symbol name has not been defined already in the parent scope.
    auto it = 
        scopes.at(parent_scope_idx).sym_name_to_symbol_idx.find(ptr->name);

    // This symbol has already been defined here.
    if(it != scopes.at(parent_scope_idx).sym_name_to_symbol_idx.end())
    {
        std::cerr << parsed_file << ":" << ptr->line << ":" << ptr->col << 
            ": Symbol already defined: " << ptr->name << '\n';
        exit(1);
    }

    // Add our symbol the parent scope's symbol lookup.
    scopes.at(parent_scope_idx).sym_name_to_symbol_idx[ptr->name] = 
        symbol_idx;
    
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
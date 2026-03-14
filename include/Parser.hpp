// Parser.hpp

#pragma once

#include <unordered_map>

#include "Program.hpp"
#include "Token.hpp"
#include "Lexer.hpp"


class Parser
{

public:

    Parser();

    Program parse(const char *in_file_path);

private:

    // Members

    // Token current_token;

    uint64_t current_token_idx = 0;

    std::vector<Token> tokens;

    const char *parsed_file = nullptr;

    struct DefinedType
    {
        uint32_t line;
        uint32_t col;
        const char * const file_defined; 
    };

    // Map of defined type names to the line and column they were defined on.
    std::unordered_map<std::string, DefinedType> defined_types
    {
        // Register initial builtins
        {"int", {0, 0, "BUILTIN"}},
        {"u8", {0, 0, "BUILTIN"}},
        {"i8", {0, 0, "BUILTIN"}},
        {"u16", {0, 0, "BUILTIN"}},
        {"i16", {0, 0, "BUILTIN"}},
        {"u32", {0, 0, "BUILTIN"}},
        {"i32", {0, 0, "BUILTIN"}},
        {"u64", {0, 0, "BUILTIN"}},
        {"i64", {0, 0, "BUILTIN"}},
        {"float", {0, 0, "BUILTIN"}},
        {"double", {0, 0, "BUILTIN"}},
        {"bool", {0, 0, "BUILTIN"}},
        {"char", {0, 0, "BUILTIN"}}
    };

    // Methods

    void print_error_location(uint32_t line,uint32_t col) const;
    void handle_tok_mismatch(const Token &got_tok, TokenID expected);
    void handle_unexpected_token(const Token &got_tok);

    std::unique_ptr<Declaration> parse_top_level();
    std::unique_ptr<ModuleDecl> parse_module();
    std::unique_ptr<FunctionDecl> parse_function(bool is_pub);
    std::unique_ptr<StructDecl> parse_struct(bool is_pub);
    std::unique_ptr<ComponentDecl> parse_component(bool is_pub);
    std::unique_ptr<Statement> parse_statement();

    // Expression parsing functions
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_assignment();
    std::unique_ptr<Expression> parse_log_or();
    std::unique_ptr<Expression> parse_log_and();
    std::unique_ptr<Expression> parse_equality();
    std::unique_ptr<Expression> parse_relational();
    std::unique_ptr<Expression> parse_additive();
    std::unique_ptr<Expression> parse_multiplicative();
    std::unique_ptr<Expression> parse_unary();
    std::unique_ptr<Expression> parse_postfix();
    std::unique_ptr<Expression> parse_primary();

    // If error_on_invalid is false, returns nullptr instead of crashing.
    std::unique_ptr<TypeDecl> parse_type_decl(bool error_on_invalid = true);
    std::unique_ptr<FieldDecl> parse_field(bool is_pub);
    ScopeBody parse_scope();
    Parameter parse_param();

    bool check(TokenID id) const;
    bool consume_if(TokenID id);
    Token expect(TokenID id);
    const Token& peek() const; 
    Token advance();
    void rewind(uint64_t token_idx);
};

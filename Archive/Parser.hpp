// Parser.hpp

#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <string>

#include "Lexer.hpp"
#include "ASTDeclarations.hpp"


struct Program
{
    std::string file_path; // File the program came from.
    std::vector<std::unique_ptr<Decl>> decls;
};

class Parser
{

public:

    Parser();

    Program parse_program(const char *file_path);

private:

    // Members

    const char *file_path;

    Token current_token;

    Lexer lexer;


    // Methods

    void print_error_location(uint32_t line, uint32_t col) const;

    void handle_tok_mismatch(const Token &got_token, TokenType expected);

    void expect_scope();

    std::unique_ptr<Decl> parse_top_level();

    std::unique_ptr<NamespaceDecl> parse_namespace();

    std::unique_ptr<FunctionDecl> parse_function();

    std::unique_ptr<StructDecl> parse_struct();

    std::unique_ptr<ComponentDecl> parse_component();

    Field parse_field();

    TypeRef parse_type_ref();

    bool check(TokenType t) const;

    bool consume_if(TokenType t);

    Token expect(TokenType t);

    const Token& peek() const; 
    
    Token advance();
}; 

namespace ProgramPrinter
{

static inline void print_type(const TypeRef &t)
{
    switch(t.ref_type)
    {
        case ReferenceType::NONE:

            break;

        case ReferenceType::REF:

            std::cout << "ref ";
            break;

        case ReferenceType::REFMUT:

            std::cout << "ref mut ";
            break;
    }

    for(bool b : t.ptr_pointee_mut)
    {
        std::cout << (b ? "*mut " : "* ");
    }

    std::cout << t.name;
}

static inline void print_field(const Field &f)
{
    std::cout << "Mutable: " << f.is_mut << "\nPublic: " << f.is_pub << 
        '\n';
    print_type(f.t);
    std::cout << "\nName: " << f.name << '\n';
}

static inline void print_struct(StructDecl *ptr)
{
    std::cout << "Struct: " << ptr->name << "\nLine: " << ptr->line << 
        "\nCol: " << ptr->col  << "\n";

    if(ptr->fields.size() > 0)
    {
        std::cout << "--Fields--\n";

        for(const Field &f : ptr->fields)
        {
            std::cout << "{\n";
            print_field(f);
            std::cout << "} // Field: " << f.name << '\n';
        }

        std::cout << "--Fields--\n";
    }
}   

static inline void print_function(FunctionDecl *ptr)
{
    std::cout << "Function: " << ptr->name << "\nLine: " << ptr->line
        << "\nCol: " << ptr->col << "\nPublic: " << ptr->is_pub << "\n";

    if(ptr->parameters.size() > 0)
    {
        std::cout << "--Parameters--\n";
    
        for(const Param &p : ptr->parameters)
        {
            std::cout << "{\nMutable: " << p.is_mut << "\nUnqualified: " << 
                p.is_unqual_param << "\nType: ";
            print_type(p.t);
            std::cout << "\n\tName: " << p.name << "\n}\n";
        }

        std::cout << "--Parameters--\n";
    }

    if(ptr->receiver_meta.has_value())
    {
        std::cout << "Receiver Meta:\n{\nName: " << 
            ptr->receiver_meta->receiver_name << "\nType: ";
        print_type(ptr->receiver_meta->receiver_type);
        std::cout << "\n}\n";
    }

    std::cout << "Return Type: ";
    print_type(ptr->return_type); 
    std::cout << '\n';
}

static inline void print_component(ComponentDecl *ptr)
{
    std::cout << "Component: " << ptr->name << "\nLine: " << ptr->line
        << "\nCol: " << ptr->col << "\n"; 
        
    // Print Fields
    if(ptr->fields.size() > 0)
    {
        std::cout << "--Fields--\n"; 

        for(const Field &f : ptr->fields)
        {
            std::cout << "{\n";
            print_field(f);
            std::cout << "} // Field: " << f.name << '\n';
        }

        std::cout << "--Fields--\n"; 
    }

    // Print Functions
    if(ptr->functions.size() > 0)
    {
        std::cout << "--Functions--\n";

        for(const std::unique_ptr<FunctionDecl> &f : ptr->functions)
        {
            std::cout << "{\n";
            print_function(f.get());
            std::cout << "} // Function: " << f->name << '\n';
        }

        std::cout << "--Functions--\n";
    }

    // Print Nested Containers
    if(ptr->nested_cnts.size() > 0)
    {
        std::cout << "--Nested Containers--\n";

        for(const NestedContainer &n : ptr->nested_cnts)
        {
            std::cout << "{\n";
            
            if(auto deduced_decl = dynamic_cast<StructDecl*>(n.decl.get()))
            {
                print_struct(deduced_decl);
            }

            else if(auto deduced_decl = 
                dynamic_cast<ComponentDecl*>(n.decl.get()))
            {
                print_component(deduced_decl);
            }

            else
            {
                std::cout << "Unkown nested container\n";
                exit(1);
            }

            std::cout << "}\n";
        }

        std::cout << "--Nested Containers--\n";
    }
}

static inline void print_namespace(NamespaceDecl *ptr)
{
    std::cout << "Namespace: " << ptr->name << "\nLine: " << ptr->line
        << "\nCol: " << ptr->col << '\n';

    if(ptr->decls.size() > 0)
    {
        std::cout << "--ASTDeclarations--\n";

        for(const std::unique_ptr<Decl> &decl : ptr->decls)
        {
            std::cout << "{\n";

            if(auto deduced_decl = dynamic_cast<NamespaceDecl*>(decl.get()))
            {
                print_namespace(deduced_decl);
            }

            else if(auto deduced_decl = dynamic_cast<FunctionDecl*>(decl.get()))
            {
                print_function(deduced_decl);
            }

            else if(auto deduced_decl = dynamic_cast<StructDecl*>(decl.get()))
            {
                print_struct(deduced_decl);
            }

            else if(auto deduced_decl = dynamic_cast<ComponentDecl*>(decl.get()))
            {
                print_component(deduced_decl);
            }

            else
            {
                std::cout << "Couldn't deduce parsed\n";
            }

            std::cout << "}\n";
        }

        std::cout << "--ASTDeclarations--\n";
    }
}

static inline void print_decl(const std::unique_ptr<Decl> &decl)
{
    std::cout << "----------------------\n";

    if(auto deduced_decl = dynamic_cast<NamespaceDecl*>(decl.get()))
    {
        print_namespace(deduced_decl);
    }

    else if(auto deduced_decl = dynamic_cast<FunctionDecl*>(decl.get()))
    {
        print_function(deduced_decl);
    }

    else if(auto deduced_decl = dynamic_cast<StructDecl*>(decl.get()))
    {
        print_struct(deduced_decl);
    }

    else if(auto deduced_decl = dynamic_cast<ComponentDecl*>(decl.get()))
    {
        print_component(deduced_decl);
    }

    else
    {
        std::cout << "Couldn't deduce parsed\n";
    }
}

static inline void print_program(const Program &p)
{
    for(const auto &decl : p.decls)
    {
        print_decl(decl);
    }
}

}

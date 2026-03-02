
#include <iostream>

#include "Lexer.hpp"
#include "Parser.hpp"

void print_type(const TypeRef &t)
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


void print_field(const Field &f)
{
    std::cout << "\tMutable: " << f.is_mut << "\n\tPublic: " << f.is_pub << 
        "\n\t";
    print_type(f.t);
    std::cout << "\n\tName: " << f.name;
}

void print_struct(StructDecl *ptr)
{
    std::cout << "Parsed Struct: " << ptr->name << "\nFields:\n";

    for(const Field &f : ptr->fields)
    {
        print_field(f);
        std::cout << "\n\n";
    }
}   

void print_function(FunctionDecl *ptr)
{
    std::cout << "Parsed Function: " << ptr->name << "\nLine: " << ptr->line
        << "\nCol: " << ptr->col << "\nPublic: " << ptr->is_pub << "\n";
    std::cout << "Parameters:\n";

    for(const Param &p : ptr->parameters)
    {
        std::cout << "\tMutable: " << p.is_mut << "\n\tUnqualified: " << 
            p.is_unqual_param << "\n\tType: ";
        print_type(p.t);
        std::cout << "\n\tName: " << p.name << "\n\n";
    }

    std::cout << "Return Type: ";
    print_type(ptr->return_type); 
    std::cout << '\n';
}


void print_decl(const std::unique_ptr<Decl> &decl)
{
    if(auto deduced_decl = dynamic_cast<NamespaceDecl*>(decl.get()))
    {
        std::cout << "Parsed Namespace\n";
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
        std::cout << "Parsed Component\n";
    }

    else
    {
        std::cout << "Couldn't deduce parsed\n";
    }
}

int main()
{
    // Lexer l;
    // l.load("TestFiles/Test2.sr");

    // Token t = l.next_token();

    // while(t.type != END_OF_FILE)
    // {   
    //     std::cout << "Read Token: " << t << "\n\n";
    //     t = l.next_token();
    // }

    Parser parser;
    Program prog = parser.parse_program("TestFiles/Test2.sr");

    for(const std::unique_ptr<Decl> &decl : prog.decls)
    {
        print_decl(decl);
    }

    return 0;
}
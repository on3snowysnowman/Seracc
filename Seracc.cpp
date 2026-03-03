
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
    std::cout << "Mutable: " << f.is_mut << "\nPublic: " << f.is_pub << 
        '\n';
    print_type(f.t);
    std::cout << "\nName: " << f.name << '\n';
}

void print_struct(StructDecl *ptr)
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

void print_function(FunctionDecl *ptr)
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

void print_component(ComponentDecl *ptr)
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

void print_namespace(NamespaceDecl *ptr)
{
    std::cout << "Namespace: " << ptr->name << "\nLine: " << ptr->line
        << "\nCol: " << ptr->col << '\n';

    if(ptr->decls.size() > 0)
    {
        std::cout << "--Declarations--\n";

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

        std::cout << "--Declarations--\n";
    }
}

void print_decl(const std::unique_ptr<Decl> &decl)
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
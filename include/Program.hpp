// Program.hpp

#pragma once

#include <vector>

#include "ASTDeclarations.hpp"


struct Program
{
    Program()
    {
        ast = std::make_unique<NamespaceDecl>();
        source_file_name = nullptr;
    }

    std::unique_ptr<NamespaceDecl> ast;
    const char *source_file_name;
};

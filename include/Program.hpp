// Program.hpp

#pragma once

#include <unordered_map>
#include <string>

#include "ASTDeclarations.hpp"


struct Program
{
    Program()
    {
        ast = std::make_unique<ModuleDecl>();
        source_file_name = nullptr;
    }

    std::unique_ptr<ModuleDecl> ast;

    const char *source_file_name;
};

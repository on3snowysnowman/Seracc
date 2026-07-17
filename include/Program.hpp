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

    // Function signatures to their Declarations.. Keys are the full formatted 
    // readable function signatures. 
    std::unordered_map<std::string, const FunctionDecl*> func_sigs;

    const char *source_file_name;
};

// CEmitter.hpp

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <ostream>

#include "Parser.hpp"
#include "Resolver.hpp"
#include "STBlder.hpp"

// Emits a single C translation unit for one Serac file (Program).
// Assumes Resolver has already filled TypeRef.resolved_type_id and produced types[].
class CEmitter
{
public:
    CEmitter();

    // Emit a .c file contents to the given stream.
    // - p: parsed program
    // - global_ns: symbol table root
    // - types: constructed types (TypeId -> ConstructedType)
    void emit_c(std::ostream& out,
                const Program& p,
                const NamespaceSymbol* global_ns,
                const std::vector<ConstructedType>& types);

private:
    // --- traversal / emission
    void emit_preamble(std::ostream& out);
    void emit_decl_list(std::ostream& out,
                        const std::vector<std::unique_ptr<Decl>>& decls);

    void emit_namespace(std::ostream& out, const NamespaceDecl* ns);
    void emit_struct(std::ostream& out, const StructDecl* st);
    void emit_component(std::ostream& out, const ComponentDecl* cp);
    void emit_function(std::ostream& out,
                       const FunctionDecl* fn,
                       const ComponentDecl* enclosing_component);

    // --- type / name lowering
    std::string c_type_from_typeref(const TypeRef& tr) const;
    std::string c_type_from_typeid(uint64_t type_id) const;

    // namespaces become a prefix stack used for mangling
    std::string mangle_name(const std::string& base) const;
    std::string mangle_qualified(const std::string& qualified) const;

    // method -> free function naming
    std::string mangle_method(const ComponentDecl* cp,
                              const FunctionDecl* fn) const;

    // minimal builtin mapping (normalized builtins from Resolver)
    std::string builtin_to_c(const std::string& builtin) const;

private:
    const Program* current_prog = nullptr;
    const std::vector<ConstructedType>* types = nullptr;

    // namespace stack for mangling
    std::vector<std::string> ns_stack;
};
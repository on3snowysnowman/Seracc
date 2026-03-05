// Resolver.cpp

#include "Resolver.hpp"

#include <iostream>
#include <sstream>

// ---------------- TypeKey utils ----------------

bool TypeKey::operator==(const TypeKey& o) const
{
    if (is_builtin != o.is_builtin) return false;
    if (ref_type != o.ref_type) return false;
    if (ptr_pointee_mut != o.ptr_pointee_mut) return false;

    if (is_builtin) return base_builtin == o.base_builtin;
    return base_user == o.base_user;
}

static inline void hash_combine(std::size_t& h, std::size_t v)
{
    // standard-ish combine
    h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
}

std::size_t TypeKeyHash::operator()(const TypeKey& k) const noexcept
{
    std::size_t h = 0;

    hash_combine(h, std::hash<bool>{}(k.is_builtin));
    hash_combine(h, std::hash<int>{}(static_cast<int>(k.ref_type)));

    if (k.is_builtin)
    {
        hash_combine(h, std::hash<std::string>{}(k.base_builtin));
    }
    else
    {
        hash_combine(h, std::hash<const void*>{}(static_cast<const void*>(k.base_user)));
    }

    // pointer chain
    for (bool b : k.ptr_pointee_mut)
    {
        hash_combine(h, std::hash<bool>{}(b));
    }

    return h;
}

// ---------------- Resolver ----------------

Resolver::Resolver()
{
    // Minimal builtin set for now.
    // Normalize aliases like your spec says.
    builtin_alias = {
        {"int", "i32"},
        {"char", "u8"},

        {"i8", "i8"},
        {"u8", "u8"},
        {"i16", "i16"},
        {"u16", "u16"},
        {"i32", "i32"},
        {"u32", "u32"},
        {"i64", "i64"},
        {"u64", "u64"},

        {"bool", "bool"},
        {"float", "float"},
        {"double", "double"},

        // if you want these in MVP:
        {"void", "void"},
        {"opaque", "opaque"},
    };
}

const std::vector<ConstructedType>& Resolver::resolve(Program& p, const NamespaceSymbol* gns)
{
    current_prog = &p;
    global_ns = gns;
    current_ns = gns;

    types.clear();
    type_intern.clear();

    // Optional: reserve TypeId 0 for "invalid" to catch missed resolutions.
    // But you currently initialize resolved_type_id=0, so using 0 as invalid is nice.
    // We'll insert a dummy type at index 0.
    {
        ConstructedType dummy;
        dummy.base_kind = ConstructedType::BaseKind::BUILTIN;
        dummy.builtin_name = "<invalid>";
        dummy.ref_type = ReferenceType::NONE;
        types.push_back(dummy);
    }

    resolve_decl_list(p.decls);

    current_prog = nullptr;
    global_ns = nullptr;
    current_ns = nullptr;

    return types;
}

// ---- Traversal ----

void Resolver::resolve_decl_list(std::vector<std::unique_ptr<Decl>>& decls)
{
    for (auto& d : decls)
    {
        if (auto ns = dynamic_cast<NamespaceDecl*>(d.get()))
        {
            resolve_namespace(ns);
        }
        else if (auto fn = dynamic_cast<FunctionDecl*>(d.get()))
        {
            resolve_function(fn);
        }
        else if (auto st = dynamic_cast<StructDecl*>(d.get()))
        {
            resolve_struct(st);
        }
        else if (auto cp = dynamic_cast<ComponentDecl*>(d.get()))
        {
            resolve_component(cp);
        }
        else
        {
            error_at(d->line, d->col, "Internal error: unknown Decl subclass during resolve.");
        }
    }
}

void Resolver::resolve_namespace(NamespaceDecl* ns_decl)
{
    // Move current namespace context into child namespace symbol.
    auto it = current_ns->nspaces.find(ns_decl->name);
    if (it == current_ns->nspaces.end())
    {
        error_at(ns_decl->line, ns_decl->col,
                 "Namespace \"" + ns_decl->name + "\" not found in symbol table (STBlder mismatch).");
    }

    const NamespaceSymbol* prev = current_ns;
    current_ns = it->second;

    resolve_decl_list(ns_decl->decls);

    current_ns = prev;
}

void Resolver::resolve_struct(StructDecl* s_decl)
{
    // Resolve all field types.
    for (auto& f : s_decl->fields)
    {
        resolve_type_ref(f.t, s_decl->line, s_decl->col);
    }
}

void Resolver::resolve_component(ComponentDecl* c_decl)
{
    // Resolve own fields
    for (auto& f : c_decl->fields)
    {
        resolve_type_ref(f.t, c_decl->line, c_decl->col);
    }

    // Resolve receiver funcs + normal funcs
    for (auto& fn : c_decl->functions)
    {
        resolve_function(fn.get());

        // Receiver validation (MVP-friendly):
        // If receiver exists, ensure it resolves to *this* component type.
        if (fn->receiver_meta.has_value())
        {
            // Ensure receiver base is this component type.
            // We'll compare the resolved user type symbol pointer identity.
            const auto& recv_tr = fn->receiver_meta->receiver_type;
            uint64_t recv_id = recv_tr.resolved_type_id;

            if (recv_id == 0 || recv_id >= types.size())
            {
                error_at(fn->line, fn->col, "Internal error: receiver type not resolved.");
            }

            const ConstructedType& ct = types[recv_id];

            if (ct.base_kind != ConstructedType::BaseKind::USER_TYPE ||
                !ct.user_sym ||
                ct.user_sym->decl_ptr != c_decl)
            {
                error_at(fn->line, fn->col,
                         "Receiver type must be the enclosing component type \"" + c_decl->name + "\".");
            }

            // Pointer receiver already rejected in parser; ensure it’s ref/ref mut (should be).
            if (recv_tr.ref_type == ReferenceType::NONE)
            {
                error_at(fn->line, fn->col, "Receiver parameter must be a reference (ref or ref mut).");
            }
        }
    }

    // Resolve nested containers:
    // Important: they introduce new types in the current namespace.
    for (auto& nc : c_decl->nested_cnts)
    {
        if (auto st = dynamic_cast<StructDecl*>(nc.decl.get()))
        {
            resolve_struct(st);
        }
        else if (auto cp = dynamic_cast<ComponentDecl*>(nc.decl.get()))
        {
            resolve_component(cp);
        }
        else
        {
            error_at(c_decl->line, c_decl->col, "Internal error: unknown nested container kind.");
        }
    }
}

void Resolver::resolve_function(FunctionDecl* f_decl)
{
    // Receiver type (if any)
    if (f_decl->receiver_meta.has_value())
    {
        resolve_type_ref(f_decl->receiver_meta->receiver_type, f_decl->line, f_decl->col);
    }

    // Params (you currently don’t parse params; but keep this for when you do)
    for (auto& p : f_decl->parameters)
    {
        resolve_type_ref(p.t, f_decl->line, f_decl->col);
    }

    // Return type
    resolve_type_ref(f_decl->return_type, f_decl->line, f_decl->col);
}

// ---- Type resolution ----

void Resolver::resolve_type_ref(TypeRef& tr, uint32_t line, uint32_t col)
{
    // Build a TypeKey: base + modifiers
    TypeKey key;
    key.ref_type = tr.ref_type;
    key.ptr_pointee_mut = tr.ptr_pointee_mut;

    // Determine base
    if (is_builtin_name(tr.name))
    {
        key.is_builtin = true;
        key.base_builtin = normalize_builtin(tr.name);
    }
    else
    {
        key.is_builtin = false;
        key.base_user = lookup_user_type(tr.name, line, col);
    }

    tr.resolved_type_id = intern_type(key);
}

uint64_t Resolver::intern_type(const TypeKey& key)
{
    auto it = type_intern.find(key);
    if (it != type_intern.end())
    {
        return it->second;
    }

    ConstructedType ct;

    ct.ref_type = key.ref_type;
    ct.ptr_pointee_mut = key.ptr_pointee_mut;

    if (key.is_builtin)
    {
        ct.base_kind = ConstructedType::BaseKind::BUILTIN;
        ct.builtin_name = key.base_builtin;
    }
    else
    {
        ct.base_kind = ConstructedType::BaseKind::USER_TYPE;
        ct.user_sym = key.base_user;

        // Optional: store for debugging/diagnostics
        ct.user_qual_name = ct.user_sym ? ct.user_sym->decl_ptr->name : "<unknown>";
    }

    uint64_t id = static_cast<uint64_t>(types.size());
    types.push_back(std::move(ct));

    type_intern.emplace(key, id);
    return id;
}

// ---- User type lookup ----

static inline std::vector<std::string> split_qualified(const std::string& s)
{
    std::vector<std::string> parts;
    std::string cur;

    for (size_t i = 0; i < s.size();)
    {
        if (i + 1 < s.size() && s[i] == ':' && s[i + 1] == ':')
        {
            parts.push_back(cur);
            cur.clear();
            i += 2;
        }
        else
        {
            cur.push_back(s[i++]);
        }
    }

    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

const TypeSymbol* Resolver::lookup_user_type(const std::string& name, uint32_t line, uint32_t col)
{
    // Qualified name: A::B::T
    if (name.find("::") != std::string::npos)
    {
        auto parts = split_qualified(name);
        if (parts.size() < 2)
        {
            error_at(line, col, "Invalid qualified type name: " + name);
        }

        // last part is type name
        std::string type_name = parts.back();
        parts.pop_back();

        const NamespaceSymbol* ns = resolve_namespace_path(parts, line, col);

        auto it = ns->types.find(type_name);
        if (it == ns->types.end())
        {
            error_at(line, col, "Unknown type \"" + name + "\".");
        }
        return &it->second;
    }

    // Unqualified: only current namespace (per your spec)
    auto it = current_ns->types.find(name);
    if (it == current_ns->types.end())
    {
        error_at(line, col,
                 "Unknown type \"" + name + "\" in namespace \"" + current_ns->name + "\".");
    }
    return &it->second;
}

const NamespaceSymbol* Resolver::resolve_namespace_path(const std::vector<std::string>& parts, uint32_t line, uint32_t col)
{
    const NamespaceSymbol* ns = global_ns;
    for (const auto& p : parts)
    {
        auto it = ns->nspaces.find(p);
        if (it == ns->nspaces.end())
        {
            error_at(line, col, "Unknown namespace in qualified name: \"" + p + "\".");
        }
        ns = it->second;
    }
    return ns;
}

// ---- Builtins ----

bool Resolver::is_builtin_name(const std::string& name) const
{
    return builtin_alias.find(name) != builtin_alias.end();
}

std::string Resolver::normalize_builtin(const std::string& name) const
{
    auto it = builtin_alias.find(name);
    if (it == builtin_alias.end()) return name;
    return it->second;
}

// ---- Diagnostics ----

[[noreturn]] void Resolver::error_at(uint32_t line, uint32_t col, const std::string& msg) const
{
    std::cerr << current_prog->file_path << ":" << line << ":" << col << ": " << msg << "\n";
    std::exit(1);
}
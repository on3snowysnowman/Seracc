/**
 * @file ASTDecl.h
 * @author Joel Height (On3SnowySnowman@gmail.com)
 * @brief AST declarations, contains all the types that can appear in the 
 * AST.
 * @version 0.1
 * @date 2026-07-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef ASTDECL_H
#define ASTDECL_H

#include "tundra/common/TypeDef.h"
#include "TundraContainers/DynamicArrayStrVw.h"
#include "TundraContainers/DynamicArrayU64.h"
#include "TundraContainers/DynamicArrayTDeclPtr.h"

// Forward decls.
typedef struct TypeDecl TypeDecl;
typedef struct Expression Expression;

// Define these containers here so they have forward decls.
#include "TundraContainers/DynamicArrayExprPtr.h"


// Expressions -----------------------------------------------------------------




// Types ----------------------------------------------------------------------- 

// Kinds of TypeDecl specialization. 
typedef enum 
{
    TYPEKIND_NAMED,     // Named types such as "int" or "SomeUserType"
    TYPEKIND_PTR,       // Pointers to types: "*int" or "*mut SomeUserType"
    TYPEKIND_REF,       // References to types: "ref int" or 
                        // "ref mut SomeUserType"
    TYPEKIND_ARRAY,     // Arrays of types such as: "[int][3][2]"
    TYPEKIND_FN_PTR,    // Pointers to functions such as:
                        // "fn_ptr(ref int, bool) -> void"
    TYPEKIND_LITERAL,    // Literal types such as "3", "true" or "null_ptr"
    TYPEKIND_ENUM_END,   // Invalid sentinel

} TypeKind;

/**
 * @brief Represents a specification of TypeDecl that is a named symbol such
 * "int", "bool" or custom user types like "SomeUserType".
 */
typedef struct
{
    // The full identifier path of the type. For instance, if this type was 
    // wrapped in a module, the identifier in the source might look like:
    // "TopMod::SubMod::SomeType", then this DynamicArray would contain:
    // [TopMod, SubMod, SomeType] respectively.
    Tundra_DynamicArrayStrVw ident_path;
    
    // Index into the symbol table of the symbol this identifier resolves to.
    u64 resolved_sym_idx;
} NamedTypeDecl;

/**
 * @brief Represents a specification of TypeDecl that is a pointer to another
 * type such as "*int" or "*mut SomeUserType". 
 */
typedef struct
{
    // If this pointer represents an opaque ptr, which does not point to a type.
    bool is_opaque_ptr;

    // If this pointer type points to a mutable type.
    bool points_to_mut;

    // Type this PtrType points to.
    TypeDecl *pointee;
} PtrTypeDecl;

/**
 * @brief Represents a specification of TypeDecl that is a reference to another
 * type such as "ref int" or "ref mut SomeUserType". 
 */
typedef struct
{
    // If this reference type referrs to a mutable type.
    bool refers_to_mut;

    // Type this RefType points to.
    TypeDecl *referred;
} RefTypeDecl;

/**
 * @brief Represents a specification of TypeDecl that is an Array of a type. Can
 * have multiple dimensions like: "[int][3]" or [SomeUserType][4][2].
 */
typedef struct
{
    // Type the Array stores.
    TypeDecl *element_type;

    // List of size expressions of this Array type in unresolved expression 
    // form. For instance if we had the type: "[int][3][4][2]", size_exprs would 
    // hold 3 IntLitExpressions. This Array is filled during parsing. The size
    // expressions are not validated until the TypeChecking stage is reached and
    // the below `size_vals` Array is constructed from the expressions in this
    // Array.
    Tundra_DynamicArrayExprPtr size_exprs;

    // List of size expressions of this Array type in converted numerical form.
    // For the same example above this would be {3, 4, 2}. This Array is not
    // filled until the type checking stage where the expressions in 
    // `size_exprs` are validated and converted into numerical form.
    Tundra_DynamicArrayU64 size_vals;
} ArrTypeDecl;

/**
 * @brief Represents a specification of TypeDecl that is a pointer to a 
 * function such as: "fn_ptr(int, ref mut u64) -> void"
 */
typedef struct 
{
    // List of the paramater types.
    Tundra_DynamicArrayTDeclPtr param_types;

    TypDecl *return_type;
} FnPtrTypeDecl;

typedef struct
{
    // Whether this literal is a null_ptr. 
    bool is_nullptr;

    // List of symbol idxs that this Literal can "accept" and take the form of. 
    // For instance, -1 can be i8, i16 and so on. 
    Tundra_DynamicArrayU64 accepted_symbol_idx;
} LiteralTypeDecl;

/**
 * @brief Represents a type such as "int", "*SomeUserType", 
 * "fn_ptr(int, ref mut u64) -> void", etc. Uses a tagged union to represent
 * the specialization this type represents.
 */
typedef struct TypeDecl
{
    u32 line;   // Line in the source this type is defined on.
    u32 col;    // Column in the source this type is defined on.

    // Kind this type represents, the tag for the union.
    TypeKind kind;

    // Represents the different types this declaration can represent.
    union 
    {
        NamedTypeDecl named;
        PtrTypeDecl ptr;
        RefTypeDecl ref;
        ArrTypeDecl arr;
        FnPtrTypeDecl fn_ptr;
    } as;
} TypeDecl;



/**
 * @brief Initializes a NamedTDecl. This simply puts the decl in a state where
 * its components can be modified. It does not create a meaningful 
 * representation of a Type.
 * 
 * @param decl Decl.
 */
void init_NamedTDecl(TypeDecl *decl);

/**
 * @brief Initializes a PtrTDecl. This simply puts the decl in a state where
 * its components can be modified. It does not create a meaningful 
 * representation of a Type.
 * 
 * @param decl Decl.
 */
void init_PtrTDecl(TypeDecl *decl);

/**
 * @brief Initializes a RefTDecl. This simply puts the decl in a state where
 * its components can be modified. It does not create a meaningful 
 * representation of a Type.
 * 
 * @param decl Decl.
 */
void init_RefTDecl(TypeDecl *decl);

/**
 * @brief Initializes an ArrTDecl. This simply puts the decl in a state where
 * its components can be modified. It does not create a meaningful 
 * representation of a Type.
 * 
 * @param decl Decl.
 */
void init_ArrTDecl(TypeDecl *decl);

/**
 * @brief Initializes a FnPtrTDecl. This simply puts the decl in a state where
 * its components can be modified. It does not create a meaningful 
 * representation of a Type.
 * 
 * @param decl Decl.
 */
void init_FnPtrTDecl(TypeDecl *decl);

#endif
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

#include "ASTDecl.h"


static void init_TDecl(TypeDecl *decl, TypeKind kind)
{
    decl->line = 0;
    decl->col = 0;
    decl->kind = kind;
}


void init_NamedTDecl(TypeDecl *decl)
{
    init_TDecl(decl, TYPEKIND_NAMED);

    NamedTypeDecl *named_decl = &decl->as.named;

    named_decl->resolved_sym_idx = 0;

    Tundra_DynArrStrVw_init(&named_decl->ident_path);
}

void init_PtrTDecl(TypeDecl *decl)
{
    init_TDecl(decl, TYPEKIND_PTR);

    PtrTypeDecl *ptr_decl = &decl->as.ptr;

    ptr_decl->is_opaque_ptr = false;
    ptr_decl->pointee = NULL;
    ptr_decl->points_to_mut = false;
}

void init_RefTDecl(TypeDecl *decl)
{
    init_TDecl(decl, TYPEKIND_REF);

    RefTypeDecl *ref_decl = &decl->as.ref;

    ref_decl->refers_to_mut = false;
    ref_decl->referred = NULL;
}

void init_ArrTDecl(TypeDecl *decl)
{
    init_TDecl(decl, TYPEKIND_ARRAY);

    ArrTypeDecl *arr_decl = &decl->as.arr;

    arr_decl->element_type = NULL;
    Tundra_DynArrExprPtr_init(&arr_decl->size_exprs);
    Tundra_DynArrU64_init(&arr_decl->size_vals);
}

void init_FnPtrTDecl(TypeDecl *decl)
{
    init_TDecl(decl, TYPEKIND_FN_PTR);

    FnPtrTypeDecl *fn_decl = &decl->as.fn_ptr;

    Tundra_DynArrTDeclPtr_init(&fn_decl->param_types);

    fn_decl->return_type = NULL;
}

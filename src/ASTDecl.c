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

#include "tundra/utils/ConsoleIO.h"


static void init_Type(Type *type, TypeKind kind)
{
    type->line = 0;
    type->col = 0;
    type->kind = kind;
}

void init_NamedType(Type *type)
{
    init_Type(type, TYPEKIND_NAMED);

    NamedType *named_decl = &type->as.named;

    named_decl->resolved_sym_idx = 0;

    Tundra_DynArrStrVw_init(&named_decl->ident_path);
}

void init_PtrType(Type *type)
{
    init_Type(type, TYPEKIND_PTR);

    PtrType *ptr_decl = &type->as.ptr;

    ptr_decl->is_opaque_ptr = false;
    ptr_decl->pointee = NULL;
    ptr_decl->points_to_mut = false;
}

void init_RefType(Type *type)
{
    init_Type(type, TYPEKIND_REF);

    RefType *ref_decl = &type->as.ref;

    ref_decl->refers_to_mut = false;
    ref_decl->referred = NULL;
}

void init_ArrType(Type *type)
{
    init_Type(type, TYPEKIND_ARRAY);

    ArrType *arr_decl = &type->as.arr;

    arr_decl->element_type = NULL;
    Tundra_DynArrExprPtr_init(&arr_decl->size_exprs);
    Tundra_DynArrU64_init(&arr_decl->size_vals);
}

void init_FuncPtrType(Type *type)
{
    init_Type(type, TYPEKIND_FN_PTR);

    FuncPtr *fn_decl = &type->as.fn_ptr;

    Tundra_DynArrTypePtr_init(&fn_decl->param_types);

    fn_decl->return_type = NULL;
}

void init_LitType(Type *type)
{
    init_Type(type, TYPEKIND_LITERAL);

    LiteralType *lit_decl = &type->as.lit;

    lit_decl->is_nullptr = false;
    Tundra_DynArrBTType_init(&lit_decl->accepted_builtins);
}


/**
 * @brief Outputs a NamedType to stdout.
 * 
 * @param type Type
 */
static void stdout_named_type(const Type *type)
{
    const NamedType *nmd_type = &type->as.named;

    // Tundra_print_cstr_w_len(nmd_type->)
    // for(u64 i = 0; i < Tundra_DynArrStrVw_size(&nmd_type->ident_path); ++i)
    
    const u64 ipath_size = Tundra_DynArrStrVw_size(&nmd_type->ident_path);

    TUNDRA_RT_ASSERT(ipath_size > 0,
        "Attempted to print a NamedType, but its identifier path was empty.\n");

    u64 i = 0;
        
    while(true)
    {
        const Tundra_StringView *view = 
            Tundra_DynArrStrVw_at(&nmd_type->ident_path, i);
            
        Tundra_print_cstr_w_len(view->view, view->num_char);

        ++i;

        if(i == ipath_size) break;

        Tundra_print_cstr("::");
    }
}

/**
 * @brief Outputs a PtrType to stdout.
 * 
 * @param type Type.
 */
static void stdout_ptr_type(const Type *type)
{
    const PtrType *p_type = &type->as.ptr;

    Tundra_print_cstr("* ");

    if(p_type->points_to_mut) { Tundra_print_cstr("mut "); }

    stdout_type(p_type->pointee);
}

/**
 * @brief Outputs a RefType to stdout.
 * 
 * @param type Type.
 */
static void stdout_ref_type(const Type *type)
{
    const RefType *r_type = &type->as.ref;

    Tundra_print_cstr("ref ");

    if(r_type->refers_to_mut) { Tundra_print_cstr("mut "); }

    stdout_type(r_type->referred);
}

/**
 * @brief Outputs an ArrType to stdout.
 * 
 * Do note that if an Array type is printed before the TypeChecking stage, its
 * size expressions will not have been parsed yet and will not be available.
 * 
 * @param type Type.
 */
static void stdout_arr_type(const Type *type)
{
    const ArrType *arr_type = &type->as.arr;

    // [int]
    Tundra_print_char('[');
    stdout_type(arr_type->element_type);
    Tundra_print_char(']');

    // We don't have access to size values yet, can't print them.
    if(Tundra_DynArrU64_size(&arr_type->size_vals) == 0)
    {
        for(u64 i = 0; 
            i < Tundra_DynArrExprPtr_size(&arr_type->size_exprs); 
            ++i)
        {
            Tundra_print_cstr("[]");
        }
    }
    else
    {
        for(u64 i = 0;
            i < Tundra_DynArrU64_size(&arr_type->size_vals);
            ++i)
        {
            Tundra_print_char('[');
            Tundra_print_u64(*Tundra_DynArrU64_at(&arr_type->size_vals, i));
            Tundra_print_char(']');
        }
    }
}

/**
 * @brief Outputs a FuncPtrType to stdout.
 * 
 * @param type Type.
 */
static void stdout_fnptr_type(const Type *type)
{
    const FuncPtr *f_ptr = &type->as.fn_ptr;

    Tundra_print_cstr("fn_ptr(");

    const u64 param_types_size = Tundra_DynArrTypePtr_size(&f_ptr->param_types);

    // Function has parameters.
    if(param_types_size != 0)
    {
        u64 i = 0;

        while(true)
        {
            stdout_type(*Tundra_DynArrTypePtr_at(&f_ptr->param_types, i));

            ++i;

            if(i == param_types_size) break;

            Tundra_print_cstr(", ");
        }
    }

    Tundra_print_cstr(") -> ");
    stdout_type(f_ptr->return_type);
}

static void stdout_lit_type(const Type *type)
{
    const LiteralType *t_ptr = &type->as.lit;

    if(t_ptr->is_nullptr)
    {
        Tundra_print_cstr("null_ptr");
        return;
    }

    const u64 acc_bt_size = Tundra_DynArrBTType_size(&t_ptr->accepted_builtins);

    if(acc_bt_size == 0)
    {
        Tundra_print_cstr("|NONE|");
        return;
    }

    u64 i = 0;

    Tundra_print_char('[');

    while(true)
    {
        stdout_bt_type(*Tundra_DynArrBTType_at(&t_ptr->accepted_builtins, i));
        Tundra_print_char(']');
        ++i;
    }
}

void stdout_type(const Type *type)
{
    TUNDRA_RT_ASSERT(type != NULL, "Attempted to print Type but type pointer "
        "was NULL.\n");

    switch(type->kind)
    {

        case TYPEKIND_NAMED:

            stdout_named_type(type);
            break;

        case TYPEKIND_PTR:

            stdout_ptr_type(type);
            break;

        case TYPEKIND_REF:

            stdout_ref_type(type);
            break;
    
        case TYPEKIND_ARRAY:

            stdout_arr_type(type);
            break;

        case TYPEKIND_FN_PTR:

            stdout_fnptr_type(type);
            break;

        case TYPEKIND_LITERAL:

            stdout_lit_type(type);
            break;

        default:

            Tundra_printf("Invalid TypeKind found when printing Type.\n");
            Tundra_flush_stdout();
            Tundra_exit(-1);
    }
}

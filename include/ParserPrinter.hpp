// ParserPrinter.hpp

#include "Parser.hpp"
#include "Program.hpp"

namespace ParserPrinter
{

static void print_expression(const Expression * const ptr, int tab_depth);


static void handle_tab_print(int tab_depth)
{
    for(int i = 0; i < tab_depth; ++i)
    {
        std::cout << "   ";
    }
}

static void handle_newline(int tab_depth)
{
    std::cout << '\n';
    handle_tab_print(tab_depth);
}

static void print_named_type(const NamedTypeDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Name: " << ptr->type_name << '\n';
}

static void print_type_decl(const TypeDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Type Kind: ";

    switch(ptr->kind)
    {
        case TypeKind::INVALID:

            std::cout << "INVALID\n";
            break;

        case TypeKind::NAMED:

            std::cout << "NAMED\n";
            print_named_type(static_cast<const NamedTypeDecl*>(ptr), 
                tab_depth);
            break;

        case TypeKind::PTR:
        {
            const PtrTypeDecl * const reint_ptr = 
                static_cast<const PtrTypeDecl*>(ptr);

            std::cout << "PTR";
            handle_newline(tab_depth);
            std::cout << "Points to mutable: " << reint_ptr->points_to_mutable;
            handle_newline(tab_depth);
            std::cout << "Pointed to type: ";
            handle_newline(tab_depth);
            std::cout << "{\n";
            print_type_decl(reint_ptr->pointee.get(), tab_depth + 1);
            handle_tab_print(tab_depth);
            std::cout << "}\n";
            break;
        }
        
        case TypeKind::REF:
        {
            const RefTypeDecl * const reint_ptr = 
                static_cast<const RefTypeDecl*>(ptr);

            std::cout << "REF";
            handle_newline(tab_depth);
            std::cout << "Reference to mutable: " << 
                reint_ptr->ref_to_mutable;
            handle_newline(tab_depth);
            std::cout << "Referenced to type: ";
            handle_newline(tab_depth);
            std::cout << "{\n";
            print_type_decl(reint_ptr->referred.get(), tab_depth + 1);
            handle_tab_print(tab_depth);
            std::cout << "}\n";
            break;
        }

        case TypeKind::ARRAY:
        {

            const ArrTypeDecl * const reint_ptr = 
                static_cast<const ArrTypeDecl*>(ptr);

            std::cout << "ARRAY";
            handle_newline(tab_depth);
            std::cout << "Depth: " << (int)reint_ptr->depth;
            handle_newline(tab_depth);
            std::cout << "Type: ";
            handle_newline(tab_depth);
            std::cout << "{\n";
            print_type_decl(reint_ptr->element_type.get(), tab_depth + 1);
            handle_tab_print(tab_depth);
            std::cout << "}";
            handle_newline(tab_depth);

            std::cout << "Size Expressions:";
            handle_newline(tab_depth);
            std::cout << "{\n";

            for(const auto &p : reint_ptr->size_exprs)
            {
                print_expression(p.get(), tab_depth + 1);
                std::cout << '\n';
            }

            handle_tab_print(tab_depth);
            std::cout << "}\n";
            break;
        }

        case TypeKind::FUNC_PTR:

            std::cout << "FUNC_PTR\n";
            break;

        default:

            std::cout << "UNKNOWN TYPE\n";
            break;
    }
}

static void print_field(const FieldDecl * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Field: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Binding Mutable: " << ptr->is_binding_mutable;
    handle_newline(tab_depth);
    std::cout << "Public: " << ptr->is_pub;
    handle_newline(tab_depth); 
    std::cout << "Type:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_type_decl(ptr->type_decl.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_param(const Parameter &param, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Param: " << param.name;
    handle_newline(tab_depth);
    std::cout << "Line: " << param.line;
    handle_newline(tab_depth);
    std::cout << "Col: " << param.col;
    handle_newline(tab_depth); 
    std::cout << "Binding Mutable: " << param.is_binding_mutable;
    handle_newline(tab_depth);
    std::cout << "Unqual noied: " << param.is_unqual_param;
    handle_newline(tab_depth);
    std::cout << "Passed by copy: " << param.passed_by_copy;
    handle_newline(tab_depth);
    std::cout << "Type:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_type_decl(param.type_decl.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_struct(const StructDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Struct: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    
    if(ptr->decls.size() == 0)
    {
        std::cout << '\n';
        return;
    }
    
    handle_newline(tab_depth);
    std::cout << "Members: ";
    handle_newline(tab_depth);
    std::cout << "{\n";

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::FIELD:

                print_field(
                    static_cast<FieldDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::STRUCT:

                print_struct(
                    static_cast<StructDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            default:

                std::cout << "Invalid declaration type: "
                    << static_cast<int>(decl->kind) << '\n';
        }
    }

    handle_newline(tab_depth);
    std::cout << "}\n";
}

static void print_bin_lit_expr(const BinaryLitExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Binary Literal: " << ptr->value << '\n';
}

static void print_hex_lit_expr(const HexLitExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Hex Literal: " << ptr->value << '\n';
}

static void print_int_lit_expr(const IntLitExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Int Literal: " << ptr->value << '\n';
}

static void print_float_lit_expr(const FloatLitExpr * const ptr, int 
    tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Float Literal: " << ptr->value << '\n';
}

static void print_str_lit_expr(const StringLitExpr * const ptr, int 
    tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "String Literal: " << ptr->value << '\n';
}

static void print_char_lit_expr(const CharLitExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Char Literal: " << ptr->value << '\n';
}

static void print_bool_lit_expr(const BoolLitExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Bool Literal: " << ptr->value << '\n';
}

static void print_nullptr_lit_expr(const NullptrLitExpr * const ptr, 
    int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Nullptr Literal" << '\n';
}

static void print_ident_expr(const IdentExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Identifier: " << ptr->name << '\n';
}

static void print_unary_expr(const UnaryExpr* const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Unary Operation: ";

    static constexpr const char * OP_READABLE_LOOKUP[]
    {
        "INVALID",
        "PRE_INC",
        "PRE_DEC",
        "POST_INC",
        "POST_DEC",
        "NEGATE",
        "BIT_NOT",
        "LO_NOT",
        "ADDRESS_OF",
        "DEREF"
    };

    std::cout << OP_READABLE_LOOKUP[static_cast<int>(ptr->op_type)];
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);

    std::cout << "Operand:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->operand.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_binary_expr(const BinaryExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Binary Operation: ";

    static constexpr const char * OP_READABLE_LOOKUP[]
    {
        "INVALID",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "MOD",
        "LSHIFT",
        "RSHIFT",
        "LT",
        "GT",
        "LE",
        "GE",
        "EQ",
        "NE",
        "BIT_AND",
        "BIT_OR",
        "BIT_XOR",
        "LOG_AND",
        "LOG_OR"
    };

    std::cout << OP_READABLE_LOOKUP[static_cast<int>(ptr->op_type)];

    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);

    std::cout << "Left Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->lhs.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);
    std::cout << "Right Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->rhs.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_assign_expr(const AssignExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Assignment Operation: ";

    static constexpr const char * OP_READABLE_LOOKUP[]
    {
        "INVALID",
        "ASSIGN",
        "ADD_ASSIGN",
        "SUB_ASSIGN",
        "MUL_ASSIGN",
        "DIV_ASSIGN",
        "MOD_ASSIGN",
        "LSHIFT_ASSIGN",
        "RSHIFT_ASSIGN",
        "BIT_AND_ASSIGN",
        "BIT_OR_ASSIGN",
        "BIT_XOR_ASSIGN"
    };

    std::cout << OP_READABLE_LOOKUP[static_cast<int>(ptr->op_type)];

    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);

    std::cout << "Left Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->lhs.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);
    std::cout << "Right Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->rhs.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_call_expr(const CallExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth); 
    std::cout << "Call Expression: ";
    handle_newline(tab_depth);
    std::cout << "Callee: ";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->callee_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";

    if(ptr->args.size() == 0) return;
    
    handle_tab_print(tab_depth);
    std::cout << "Args:";
    handle_newline(tab_depth);
    std::cout << "{\n";

    for(const auto &p : ptr->args)
    {
        print_expression(p.get(), tab_depth + 1);
        std::cout << '\n';
    }

    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_cast_expr(const CastExpr * const ptr, int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth); 
    std::cout << "Cast Expression:";
    handle_newline(tab_depth);
    std::cout << "Cast type: ";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_type_decl(ptr->to_cast_type.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);
    std::cout << "To Cast Expression: ";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->expr_to_cast.get(), tab_depth);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_sbscrpt_expr(const SubscriptExpr * const ptr, 
    int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth); 
    std::cout << "Subscript Expression:";
    handle_newline(tab_depth);
    std::cout << "Base Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->base_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);
    std::cout << "Index Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->index_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_memb_acc_expr(const MemberAccExpr * const ptr, 
    int tab_depth) 
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth); 
    std::cout << "Member Access Expression:";
    handle_newline(tab_depth);
    std::cout << "Via Pointer: " << ptr->via_pointer;
    handle_newline(tab_depth);
    std::cout << "Base Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->base_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);
    std::cout << "Member Name: " << ptr->member_name << '\n';
}
 
static void print_expression(const Expression * const ptr, int tab_depth)
{
    switch(ptr->exp_type)
    {
        case ExpressionType::BIN_LITERAL:

            print_bin_lit_expr(static_cast<const BinaryLitExpr*>(ptr), 
                tab_depth);
            break;

        case ExpressionType::HEX_LITERAL:

            print_hex_lit_expr(static_cast<const HexLitExpr*>(ptr),
                tab_depth);
            break;

        case ExpressionType::INT_LITERAL:

            print_int_lit_expr(static_cast<const IntLitExpr*>(ptr), 
                tab_depth);
            break;

        case ExpressionType::FLOAT_LITERAL:

            print_float_lit_expr(static_cast<const FloatLitExpr*>(ptr),
                tab_depth);
            break;

        case ExpressionType::STR_LITERAL:

            print_str_lit_expr(static_cast<const StringLitExpr*>(ptr),
                tab_depth);
            break;

        case ExpressionType::CHAR_LITERAL:

            print_char_lit_expr(static_cast<const CharLitExpr*>(ptr),
                tab_depth);
            break;

        case ExpressionType::BOOL_LITERAL:

            print_bool_lit_expr(static_cast<const BoolLitExpr*>(ptr), 
                tab_depth);
            break;

        case ExpressionType::NULLPTR_LITERAL:

            print_nullptr_lit_expr(static_cast<const NullptrLitExpr*>(ptr),
                tab_depth);
            break;

        case ExpressionType::IDENTIFIER:

            print_ident_expr(static_cast<const IdentExpr*>(ptr),
                tab_depth);
            break;

        case ExpressionType::UNARY:

            print_unary_expr(static_cast<const UnaryExpr*>(ptr), tab_depth);
            break;

        case ExpressionType::BINARY:

            print_binary_expr(static_cast<const BinaryExpr*>(ptr), tab_depth);
            break;

        case ExpressionType::ASSIGN:

            print_assign_expr(static_cast<const AssignExpr*>(ptr),
                tab_depth);
            break;

        case ExpressionType::CALL:

            print_call_expr(static_cast<const CallExpr*>(ptr), tab_depth);
            break;

        case ExpressionType::CAST:

            print_cast_expr(static_cast<const CastExpr*>(ptr), tab_depth);
            break;

        case ExpressionType::SUBSCRIPT:

            print_sbscrpt_expr(static_cast<const SubscriptExpr*>(ptr), 
                tab_depth);
            break;

        case ExpressionType::MEMBER_ACCESS:

            print_memb_acc_expr(static_cast<const MemberAccExpr*>(ptr),
                tab_depth);
            break;

        default:

            std::cout << "No printing for this expression\n";
            break;
    }
}

static void print_var_decl(const VarDeclStmt * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Variable Declaration:";
    handle_newline(tab_depth);
    std::cout << "Name: " << ptr->var_name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Binding Mutable: " << ptr->is_binding_mutable;
    handle_newline(tab_depth);
    std::cout << "Type:";
    handle_newline(tab_depth);
    std::cout << "{\n"; 
    print_type_decl(ptr->type_decl.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";

    if(ptr->init_expr != nullptr)
    {
        handle_tab_print(tab_depth);
        std::cout << "Init Expression:";
        handle_newline(tab_depth);
        std::cout << "{\n";
        print_expression(ptr->init_expr.get(), tab_depth + 1);
        handle_tab_print(tab_depth);
        std::cout << "}\n";
    }
}

static void print_ret_decl(const RetStmt * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Return Statement:";
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Expression:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->ret_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_scope(const ScopeBody &scope, int tab_depth);

static void print_if_stmt(const IfStmt * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "If Statement:";
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Condition Expr:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->condition_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);
    std::cout << "Body:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(ptr->then_body, tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";

    if(ptr->else_branch != nullptr)
    {
        handle_newline(tab_depth);
        std::cout << "Else Branch Statement:";
        handle_newline(tab_depth);
        std::cout << "{\n";
        print_if_stmt(static_cast<IfStmt*>(ptr->else_branch.get()), 
            tab_depth + 1);
        handle_tab_print(tab_depth);
        std::cout << "}";
    }

    std::cout << '\n';
}

static void print_while_stmt(const WhileStmt * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "If Statement:";
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Condition Expr:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->condition_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);
    std::cout << "Body:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(ptr->body, tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_statement(const Statement * const ptr, int tab_depth);

static void print_for_stmt(const ForStmt * const ptr, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "For Statement:";
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);

    std::cout << "Init Statement:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_statement(ptr->init_stmt.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);

    std::cout << "Condition Expr:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->condition_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);

    std::cout << "Increment Expr:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_expression(ptr->increment_expr.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}";
    handle_newline(tab_depth);

    std::cout << "Body:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(ptr->body, tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

static void print_statement(const Statement * const ptr, int tab_depth)
{
    switch(ptr->stmt_type)
    {   
        case StatementType::BLOCK:

            print_scope(static_cast<const BlockStmt*>(ptr)->block_decl, 
                tab_depth + 1);
            break;

        case StatementType::COMPONENT_DECL:

            std::cout << "Component Decl";
            break;

        case StatementType::EXPR:

            print_expression(
                static_cast<const ExprStmt*>(ptr)->expr.get(),
                tab_depth + 1);
            break;

        case StatementType::FOR:

            print_for_stmt(static_cast<const ForStmt*>(ptr), tab_depth + 1);
            break;

        case StatementType::IF:

            print_if_stmt(
                static_cast<const IfStmt*>(ptr),
                tab_depth + 1
            );
            break;

        case StatementType::RETURN:

            print_ret_decl(
                static_cast<const RetStmt*>(ptr), tab_depth + 1);
            break;

        case StatementType::STRUCT_DECL:

            print_struct(
                static_cast<const StructDeclStmt*>(ptr)->decl.get(),
                tab_depth + 1);
            break;

        case StatementType::VAR_DECL:

            print_var_decl(
                static_cast<const VarDeclStmt*>(ptr), 
                tab_depth + 1);
            break;

        case StatementType::WHILE:

            print_while_stmt(
                static_cast<const WhileStmt*>(ptr),
                tab_depth + 1
            );
            break;

        default:

            std::cout << "Invalid";
            break;
    }
}

static void print_scope(const ScopeBody &scope, int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "Line: " << scope.line;
    handle_newline(tab_depth);
    std::cout << "Col: " << scope.col;
    
    if(scope.statements.size() > 0)
    {
        handle_newline(tab_depth);
        std::cout << "Statements:";
        handle_newline(tab_depth);
        std::cout << "{";

        for(const std::unique_ptr<Statement> &stmt : scope.statements)
        {
            std::cout << '\n';

            print_statement(stmt.get(), tab_depth);
        }

        handle_newline(tab_depth);
        std::cout << "}";
    }

    std::cout << '\n';
}

static void print_function(const FunctionDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "-- FUNCTION --";
    handle_newline(tab_depth);
    std::cout << "Name: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Public: " << ptr->is_pub;
    handle_newline(tab_depth);
    std::cout << "Return type:";
    handle_newline(tab_depth);
    std::cout << "{\n"; 
    print_type_decl(ptr->ret_type.get(), tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
    // handle_tab_print(tab_depth);

    if(ptr->receiver_data.has_value())
    {
        handle_tab_print(tab_depth);
        std::cout << "Receiver Parameter";
        handle_newline(tab_depth);
        std::cout << "{";
        handle_newline(tab_depth);
        std::cout << "   Name: " << ptr->receiver_data->receiver_name;
        handle_newline(tab_depth);
        std::cout << "   Type:";
        handle_newline(tab_depth);
        std::cout << "   {\n";
        print_type_decl(ptr->receiver_data->receiver_type_decl.get(), 
            tab_depth + 2);
        handle_tab_print(tab_depth);
        std::cout << "   }";
        handle_newline(tab_depth);
        std::cout << "}\n";
    }
    
    if(ptr->params.size() > 0)
    {
        handle_tab_print(tab_depth);
        std::cout << "Parameters:";
        handle_newline(tab_depth);
        std::cout << "{\n";

        for(const Parameter &p : ptr->params)
        {
            print_param(p, tab_depth + 1);
        }

        handle_newline(tab_depth);
        std::cout << "}\n";
    }

    handle_tab_print(tab_depth);
    std::cout << "Body:";
    handle_newline(tab_depth);
    std::cout << "{\n";
    print_scope(ptr->body, tab_depth + 1);
    handle_tab_print(tab_depth);
    std::cout << "}\n";
    
}

static void print_component(const ComponentDecl * const ptr,
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "-- COMPONENT --";
    handle_newline(tab_depth);
    std::cout << "Name: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    
    if(ptr->decls.size() == 0)
    {
        std::cout << '\n';
        return;
    }
    
    handle_newline(tab_depth);
    std::cout << "Members: ";
    handle_newline(tab_depth);
    std::cout << "{\n";

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::FIELD:

                print_field(
                    static_cast<FieldDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::STRUCT:

                print_struct(
                    static_cast<StructDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::COMPONENT:

                print_component(
                    static_cast<ComponentDecl*>(decl.get()),
                    tab_depth + 1
                );
                break;

            case DeclKind::FUNCTION:

                print_function(
                    static_cast<FunctionDecl*>(decl.get()),
                    tab_depth + 1
                );
                break;

            default:

                std::cout << "Invalid declaration type: "
                    << static_cast<int>(decl->kind) << '\n';

        }
        
        std::cout << '\n';
    }

    handle_newline(tab_depth);
    std::cout << "}\n";
}

static void print_module(const ModuleDecl * const ptr, 
    int tab_depth)
{
    handle_tab_print(tab_depth);
    std::cout << "-- MODULE --";
    handle_newline(tab_depth);
    std::cout << "Name: " << ptr->name;
    handle_newline(tab_depth);
    std::cout << "Line: " << ptr->line;
    handle_newline(tab_depth);
    std::cout << "Col: " << ptr->col;
    handle_newline(tab_depth);
    std::cout << "Declarations:";
    handle_newline(tab_depth);
    std::cout << "{\n";

    for(const std::unique_ptr<Declaration> &decl : ptr->decls)
    {
        switch(decl->kind)
        {
            case DeclKind::FIELD:

                print_field(
                    static_cast<FieldDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::MODULE:

                print_module(
                    static_cast<ModuleDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::STRUCT:

                print_struct(
                    static_cast<StructDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::FUNCTION:

                print_function(
                    static_cast<FunctionDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            case DeclKind::COMPONENT:

                print_component(
                    static_cast<ComponentDecl*>(decl.get()),
                    tab_depth + 1);
                break;

            default:

                std::cerr << "Invalid declaration type\n";
                exit(1);
        }
        std::cout << "\n";
    }

    handle_tab_print(tab_depth);
    std::cout << "}\n";
}

}

static void print_parse_results(const char *in_file_path)
{
    Parser pars;
    Program prog = pars.parse(in_file_path);

    ParserPrinter::print_module(prog.ast.get(), 0);
}

// ASTDeclarations.hpp

#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include <iostream>

#include "Builtins.hpp"

// Forward Decls
struct TypeDecl;
struct FunctionDecl;
struct ComponentDecl;
struct StructDecl;

enum class TypeKind
{
    INVALID, // Uninitialized Sentinel
    NAMED,
    PTR,
    REF,
    ARRAY,
    FUNC_PTR
};

// Some Expression require full impl of TypeDecl, so we define it here.
struct TypeDecl
{
    uint32_t line = 0;
    uint32_t col = 0;
    // bool is_literal = false;
    TypeKind kind = TypeKind::INVALID;

    std::string readable;

    virtual ~TypeDecl() = default;

    // We need a clone function for the TypeChecker to be able to create its
    // own TypeDecls which require their own unique_ptr.
    virtual std::unique_ptr<TypeDecl> clone() const = 0;
};

static void print_ident_path(const std::vector<std::string> &ident_path,
    std::ostream &os = std::cout)
{
    if(ident_path.size() == 0) return;

    size_t i = 0;

    while(true)
    {
        os << ident_path.at(i);

        ++i;

        if(i >= ident_path.size()) break;

        os << "::";
    }
}


// EXPRESSIONS =================================================================

enum class ExpressionType
{
    INVALID, // Uninitialized Sentinel
    BIN_LITERAL,
    HEX_LITERAL,
    INT_LITERAL,
    FLOAT_LITERAL,
    STR_LITERAL,
    CHAR_LITERAL,
    BOOL_LITERAL,
    NULLPTR_LITERAL,
    IDENTIFIER,
    STRUCT_INIT,
    STRUCT_CREATE,
    ARR_INIT,

    UNARY,
    BINARY,
    TERNARY,
    ASSIGN,

    CALL,
    CAST,
    SUBSCRIPT,
    MEMBER_ACCESS
};

static inline std::ostream& operator<<(std::ostream &os, ExpressionType type)
{
    switch(type)
    {
        case ExpressionType::ASSIGN:

            os << "ASSIGN";
            break;

        case ExpressionType::BIN_LITERAL:

            os << "BIN_LITERAL";
            break;

        case ExpressionType::BINARY:

            os << "BINARY";
            break;

        case ExpressionType::BOOL_LITERAL:

            os << "BOOL_LITERAL";
            break;
        
        case ExpressionType::CALL:

            os << "CALL";
            break;

        case ExpressionType::CAST:

            os << "CAST";
            break;

        case ExpressionType::CHAR_LITERAL:

            os << "CHAR_LITERAL";
            break;
        
        case ExpressionType::FLOAT_LITERAL:

            os << "FLOAT_LITERAL";
            break;

        case ExpressionType::HEX_LITERAL:

            os << "HEX_LITERAL";
            break;

        case ExpressionType::IDENTIFIER:

            os << "IDENTIFIER";
            break;

        case ExpressionType::INT_LITERAL:

            os << "INT_LITERAL";
            break;

        case ExpressionType::INVALID:

            os << "INVALID";
            break;

        case ExpressionType::MEMBER_ACCESS:

            os << "MEMBER_ACCESS";
            break;

        case ExpressionType::NULLPTR_LITERAL:

            os << "NULLPTR_LITERAL";
            break;

        case ExpressionType::STR_LITERAL:

            os << "STR_LITERAL";
            break;

        case ExpressionType::STRUCT_INIT:

            os << "STRUCT_INIT";
            break;

        case ExpressionType::STRUCT_CREATE:

            os << "STRUCT_CREATE";
            break;

        case ExpressionType::ARR_INIT:

            os << "ARR_INIT";
            break;

        case ExpressionType::SUBSCRIPT:

            os << "SUBSCRIPT";
            break;

        case ExpressionType::TERNARY:

            os << "TERNARY";
            break;

        case ExpressionType::UNARY:

            os << "UNARY";
            break;
    }

    return os;
}

enum class UnaryOp
{
    INVALID, // Uninitialized Sentinel
    PRE_INC,
    PRE_DEC,
    POST_INC,
    POST_DEC,
    NEGATE,
    BIT_NOT,
    LOG_NOT,
    ADDRESS_OF,
    REF_OF,
    DEREF
};

static inline std::ostream& operator<<(std::ostream &os, UnaryOp op)
{
    switch(op)
    {
        case UnaryOp::PRE_INC:
        {
            os << "PRE_INC";
            break;
        }

        case UnaryOp::PRE_DEC:
        {
            os << "PRE_DEC";
            break;
        }

        case UnaryOp::POST_INC:
        {
            os << "POST_INC";
            break;
        }

        case UnaryOp::POST_DEC:
        {
            os << "POST_DEC";
            break;
        }

        case UnaryOp::NEGATE:
        {
            os << "NEGATE";
            break;
        }

        case UnaryOp::BIT_NOT:
        {
            os << "BIT_NOT";
            break;
        }

        case UnaryOp::LOG_NOT:
        {
            os << "LOG_NOT";
            break;
        }

        case UnaryOp::ADDRESS_OF:
        {
            os << "ADDRESS_OF";
            break;
        }

        case UnaryOp::REF_OF:
            
            os << "REFERENCE OF";
            break;

        case UnaryOp::DEREF:
        {
            os << "DEREF";
            break;
        }

        case UnaryOp::INVALID:

            os << "INVALID";
            break;
    }

    return os;
}

enum class BinaryOp
{
    INVALID, // Uninitialized Sentinel
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    LSHIFT,
    RSHIFT,
    LT,
    GT,
    LE,
    GE,
    EQ,
    NE,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    LOG_AND,
    LOG_OR
};

enum class AssignOp
{
    INVALID, // Uninitialized Sentinel
    ASSIGN,
    ADD_ASSIGN,
    SUB_ASSIGN,
    MUL_ASSIGN,
    DIV_ASSIGN,
    MOD_ASSIGN,
    LSHIFT_ASSIGN,
    RSHIFT_ASSIGN,
    BIT_AND_ASSIGN,
    BIT_OR_ASSIGN,
    BIT_XOR_ASSIGN
};

struct Expression
{
    uint32_t line = 0;
    uint32_t col = 0;

    ExpressionType exp_type = ExpressionType::INVALID;

    virtual ~Expression() = default;
    virtual std::unique_ptr<Expression> clone() const
    {
        std::unique_ptr<Expression> new_obj = 
            std::make_unique<Expression>();

        new_obj->line = line;
        new_obj->col = col;

        return new_obj;
    }
};

struct BinaryLitExpr : Expression
{
    BinaryLitExpr() { exp_type = ExpressionType::BIN_LITERAL; }
    std::string value;

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<BinaryLitExpr>(*this);
    }
};

struct HexLitExpr : Expression
{
    HexLitExpr() { exp_type = ExpressionType::HEX_LITERAL; }
    std::string value;

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<HexLitExpr>(*this);
    }
};

struct IntLitExpr : Expression
{
    IntLitExpr() { exp_type = ExpressionType::INT_LITERAL; }
    std::string value;

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<IntLitExpr>(*this);
    }
};

struct FloatLitExpr : Expression
{
    FloatLitExpr() { exp_type = ExpressionType::FLOAT_LITERAL; }
    std::string value;

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<FloatLitExpr>(*this);
    }
};

struct StringLitExpr : Expression
{
    StringLitExpr() { exp_type = ExpressionType::STR_LITERAL; }
    std::string value;
    
    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<StringLitExpr>(*this);
    }
};

struct CharLitExpr : Expression
{
    CharLitExpr() { exp_type = ExpressionType::CHAR_LITERAL; }
    char value;

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<CharLitExpr>(*this);
    }
};

struct BoolLitExpr : Expression
{
    BoolLitExpr() { exp_type = ExpressionType::BOOL_LITERAL; }
    bool value;

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<BoolLitExpr>(*this);
    }
};

struct NullptrLitExpr : Expression
{
    NullptrLitExpr() { exp_type = ExpressionType::NULLPTR_LITERAL; }

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<NullptrLitExpr>(*this);
    }
};

struct IdentExpr : Expression
{
    IdentExpr() { exp_type = ExpressionType::IDENTIFIER; }
    std::vector<std::string> ident_path;
    std::optional<uint64_t> resolved_symbol_idx;

    std::unique_ptr<Expression> clone() const final
    {
        return std::make_unique<IdentExpr>(*this);
    }
};

struct StructInitExpr : Expression
{
    StructInitExpr() { exp_type = ExpressionType::STRUCT_INIT; }
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> 
        init_args;
    
    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<StructInitExpr> new_obj =
            std::make_unique<StructInitExpr>(); 

        new_obj->line = line;
        new_obj->col = col;

        for(const auto &elem : init_args)
        {
            new_obj->init_args.emplace_back(elem.first, elem.second->clone());
        }

        return new_obj;
    }
};

struct StructCreateExpr : Expression
{
    StructCreateExpr() { exp_type = ExpressionType::STRUCT_CREATE; }
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> create_expr;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<StructCreateExpr> new_obj = 
            std::make_unique<StructCreateExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->lhs = lhs->clone();
        new_obj->create_expr = create_expr->clone();

        return new_obj;
    }
};

struct ArrInitExpr : Expression
{
    ArrInitExpr() { exp_type = ExpressionType::ARR_INIT; }
    std::vector<std::unique_ptr<Expression>> init_args;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<ArrInitExpr> new_obj =
            std::make_unique<ArrInitExpr>();

        new_obj->line = line;
        new_obj->col = col;

        for(const std::unique_ptr<Expression> &expr : init_args)
        {   
            new_obj->init_args.push_back(expr->clone());
        }

        return new_obj;
    }
};

struct UnaryExpr : Expression
{
    UnaryExpr() { exp_type = ExpressionType::UNARY; }

    UnaryOp op_type = UnaryOp::INVALID;
    std::unique_ptr<Expression> operand;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<UnaryExpr> new_obj =
            std::make_unique<UnaryExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->op_type = op_type;
        new_obj->operand = operand->clone();

        return new_obj;
    }
};

struct BinaryExpr : Expression
{
    BinaryExpr() { exp_type = ExpressionType::BINARY; }

    BinaryOp op_type = BinaryOp::INVALID;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<BinaryExpr> new_obj =
            std::make_unique<BinaryExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->op_type = op_type;
        new_obj->lhs = lhs->clone();
        new_obj->rhs = rhs->clone();

        return new_obj;
    }
};

struct TernaryExpr : Expression
{
    TernaryExpr() { exp_type = ExpressionType::TERNARY; }

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> on_true_expr;
    std::unique_ptr<Expression> on_false_expr;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<TernaryExpr> new_obj =
            std::make_unique<TernaryExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->condition = condition->clone();
        new_obj->on_true_expr = on_true_expr->clone();
        new_obj->on_false_expr = on_false_expr->clone();

        return new_obj;
    }
};

struct AssignExpr : Expression
{
    AssignExpr() { exp_type = ExpressionType::ASSIGN; }

    AssignOp op_type = AssignOp::INVALID;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<AssignExpr> new_obj =
            std::make_unique<AssignExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->op_type = op_type;
        new_obj->lhs = lhs->clone();
        new_obj->rhs = rhs->clone();

        return new_obj;
    }
};

struct CastExpr : Expression
{
    CastExpr() { exp_type = ExpressionType::CAST; }

    std::unique_ptr<TypeDecl> to_cast_type;
    std::unique_ptr<Expression> expr_to_cast;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<CastExpr> new_obj =
            std::make_unique<CastExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->to_cast_type = to_cast_type->clone();
        new_obj->expr_to_cast = expr_to_cast->clone();

        return new_obj;
    }
};

struct CallExpr : Expression
{
    CallExpr() { exp_type = ExpressionType::CALL; }

    std::unique_ptr<Expression> callee_expr;
    std::vector<std::unique_ptr<Expression>> args;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<CallExpr> new_obj =
            std::make_unique<CallExpr>();

        new_obj->callee_expr = callee_expr->clone();

        new_obj->line = line;
        new_obj->col = col;

        for(const std::unique_ptr<Expression> &expr : args)
        {
            new_obj->args.push_back(expr->clone());
        }

        return new_obj;
    }
};

struct SubscriptExpr : Expression
{
    SubscriptExpr() { exp_type = ExpressionType::SUBSCRIPT; }

    std::unique_ptr<Expression> base_expr;
    // std::unique_ptr<Expression> index_expr;
    std::vector<std::unique_ptr<Expression>> index_exprs;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<SubscriptExpr> new_obj =
            std::make_unique<SubscriptExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->base_expr = base_expr->clone();
        // new_obj->index_expr = index_expr->clone();

        for(const std::unique_ptr<Expression> &ptr : index_exprs)
            new_obj->index_exprs.push_back(ptr->clone());

        return new_obj;
    }
};

struct MemberAccExpr : Expression
{
    MemberAccExpr() { exp_type = ExpressionType::MEMBER_ACCESS; }

    bool via_pointer = false;

    std::unique_ptr<Expression> base_expr;
    std::string member_name;

    std::unique_ptr<Expression> clone() const final
    {
        std::unique_ptr<MemberAccExpr> new_obj =
            std::make_unique<MemberAccExpr>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->via_pointer = via_pointer;
        new_obj->base_expr = base_expr->clone();
        new_obj->member_name = member_name;

        return new_obj;
    }
};

// STATEMENTS ==================================================================

enum class StatementType
{
    INVALID, // Uninitialized Sentinel
    EXPR,
    VAR_DECL,
    STRUCT_DECL,
    COMPONENT_DECL,
    RETURN,
    IF,
    WHILE,
    FOR,
    BLOCK
};

static inline std::ostream& operator<<(std::ostream &os, StatementType type)
{
    switch(type)
    {
        case StatementType::BLOCK:
            os << "BLOCK";
            break;
        
        case StatementType::COMPONENT_DECL:
            os << "COMPONENT_DECL";
            break;

        case StatementType::EXPR:
            os << "EXPR";
            break;

        case StatementType::FOR:
            os << "FOR";
            break;

        case StatementType::IF:
            os << "IF";
            break;

        case StatementType::INVALID:
            os << "INVALID";
            break;

        case StatementType::RETURN:
            os << "RETURN";
            break;

        case StatementType::STRUCT_DECL:
            os << "STRUCT_DECL";
            break;

        case StatementType::VAR_DECL:
            os << "VAR_DECL";
            break;

        case StatementType::WHILE:
            os << "WHILE";
            break;
    }

    return os;
}

struct Statement
{
    uint32_t line = 0;
    uint32_t col = 0;
    StatementType stmt_type = StatementType::INVALID;
    
    virtual ~Statement() = default;
};

struct ExprStmt : Statement
{
    ExprStmt() { stmt_type = StatementType::EXPR; }

    std::unique_ptr<Expression> expr;
};

struct VarDeclStmt : Statement
{
    VarDeclStmt() { stmt_type = StatementType::VAR_DECL; }

    bool is_binding_mutable = false;
    std::unique_ptr<TypeDecl> type_decl;
    std::string var_name;
    std::unique_ptr<Expression> init_expr; // nullptr if none
    uint64_t symbol_idx; // Idx of the variable symbol, not type symbol
};

struct StructDeclStmt : Statement
{
    StructDeclStmt() { stmt_type = StatementType::STRUCT_DECL; }

    std::unique_ptr<StructDecl> decl;
};

struct ComponentDeclStmt : Statement
{
    ComponentDeclStmt() { stmt_type = StatementType::COMPONENT_DECL; }

    std::unique_ptr<ComponentDecl> decl;
};

struct RetStmt : Statement
{
    RetStmt() { stmt_type = StatementType::RETURN; }

    std::unique_ptr<Expression> ret_expr; 
};

struct ScopeBody
{
    uint32_t line = 0;
    uint32_t col = 0;

    std::vector<std::unique_ptr<Statement>> statements;
    uint64_t scope_idx = 0; // Idx of the scope of this body 
};

struct IfStmt : Statement
{
    IfStmt() { stmt_type = StatementType::IF; }

    std::unique_ptr<Expression> condition_expr;
    ScopeBody then_body;
    std::unique_ptr<Statement> else_branch; // nullptr if none
};

struct WhileStmt : Statement
{
    WhileStmt() { stmt_type = StatementType::WHILE; }

    std::unique_ptr<Expression> condition_expr;
    ScopeBody body;
};

struct ForStmt : Statement
{
    ForStmt() { stmt_type = StatementType::FOR; }

    std::unique_ptr<Statement> init_stmt;
    std::unique_ptr<Expression> condition_expr;
    std::unique_ptr<Expression> increment_expr;
    ScopeBody body;
};

struct BlockStmt : Statement
{
    BlockStmt() { stmt_type = StatementType::BLOCK; }

    ScopeBody block_decl;
};

// DECLARATIONS ================================================================

struct LiteralData
{
    // BuiltinType builtin_type = BuiltinType::INVALID;

    // List of builtin types that this type can take on. ie -1 can be i8, 
    // i16, int and so on...
    std::vector<BuiltinType> acceptable_builtin_ids;
    // bool is_integral_and_negative = false;
};

struct NamedTypeDecl : TypeDecl
{
    NamedTypeDecl() { kind = TypeKind::NAMED; }
    std::vector<std::string> ident_path;
    std::optional<uint64_t> resolved_symbol_idx;
    std::optional<LiteralData> literal_data;

    std::unique_ptr<TypeDecl> clone() const final
    {
        return std::make_unique<NamedTypeDecl>(*this);
    }
};

struct PtrTypeDecl : TypeDecl
{
    PtrTypeDecl() { kind = TypeKind::PTR; }

    bool points_to_mutable = false;
    std::unique_ptr<TypeDecl> pointee;
    std::optional<BuiltinPtrType> builtin_type;

    std::unique_ptr<TypeDecl> clone() const final
    {
        std::unique_ptr<PtrTypeDecl> new_obj =
            std::make_unique<PtrTypeDecl>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->points_to_mutable = points_to_mutable;
        new_obj->pointee = pointee->clone();
        new_obj->builtin_type = builtin_type;
    
        return new_obj;
    }
};

struct RefTypeDecl : TypeDecl
{
    RefTypeDecl() { kind = TypeKind::REF; }

    bool ref_to_mutable = false;
    std::unique_ptr<TypeDecl> referred;

    std::unique_ptr<TypeDecl> clone() const final
    {
        std::unique_ptr<RefTypeDecl> new_obj =
            std::make_unique<RefTypeDecl>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->ref_to_mutable = ref_to_mutable;
        new_obj->referred = referred->clone();
        
        return new_obj;
    }

};

struct ArrTypeDecl : TypeDecl
{
    ArrTypeDecl() { kind = TypeKind::ARRAY; }

    // uint8_t depth = 0;

    std::unique_ptr<TypeDecl> element_type;
    std::vector<std::unique_ptr<Expression>> size_exprs;
    
    // Numerical values of the size expressions.
    std::vector<size_t> size_expr_as_num;

    std::unique_ptr<TypeDecl> clone() const final
    {
        std::unique_ptr<ArrTypeDecl> new_obj =
            std::make_unique<ArrTypeDecl>();

        new_obj->line = line;
        new_obj->col = col;

        // new_obj->depth = depth;
        new_obj->element_type = element_type->clone();

        for(const std::unique_ptr<Expression> &expr : size_exprs)
        {
            new_obj->size_exprs.push_back(expr->clone());
        }

        for(size_t i : size_expr_as_num)
        {
            new_obj->size_expr_as_num.push_back(i);
        }
    
        return new_obj;
    }
};

struct Parameter
{
    uint32_t line = 0;
    uint32_t col = 0;
    std::string name;
    // bool is_unqual_param = false;
    bool is_binding_mutable = false;
    bool passed_by_copy = false;
    std::unique_ptr<TypeDecl> type_decl;
    std::optional<uint64_t> symbol_idx;

    Parameter() = default;

    Parameter(const Parameter &other)
    {
        line = other.line;
        col = other.col;
        name = other.name;
        // is_unqual_param = other.is_unqual_param;
        is_binding_mutable = other.is_binding_mutable;
        passed_by_copy = other.passed_by_copy;
        type_decl = other.type_decl ? other.type_decl->clone() : nullptr;
        symbol_idx = other.symbol_idx;
    }

    Parameter& operator=(const Parameter &other)
    {
        if (this == &other) return *this;

        line = other.line;
        col = other.col;
        name = other.name;
        // is_unqual_param = other.is_unqual_param;
        is_binding_mutable = other.is_binding_mutable;
        passed_by_copy = other.passed_by_copy;
        type_decl = other.type_decl ? other.type_decl->clone() : nullptr;
        symbol_idx = other.symbol_idx;

        return *this;
    }

    Parameter(Parameter&&) noexcept = default;
    Parameter& operator=(Parameter&&) noexcept = default;
};

struct FuncPtrDecl : TypeDecl
{
    FuncPtrDecl() { kind = TypeKind::FUNC_PTR; }

    std::unique_ptr<TypeDecl> ret_type;
    std::vector<Parameter> param_types;

    std::unique_ptr<TypeDecl> clone() const final
    {
        std::unique_ptr<FuncPtrDecl> new_obj =
            std::make_unique<FuncPtrDecl>();

        new_obj->line = line;
        new_obj->col = col;

        new_obj->ret_type = ret_type->clone();
        new_obj->param_types = param_types;
    
        return new_obj;
    }
};

enum class DeclKind
{
    INVALID,
    FIELD,
    MODULE,
    STRUCT,
    FUNCTION,
    COMPONENT
};

static inline std::ostream& operator<<(std::ostream &os, DeclKind kind)
{
    switch(kind)
    {
        case DeclKind::INVALID:
            os << "INVALID";
            break;

        case DeclKind::FIELD:
            os << "FIELD";
            break;

        case DeclKind::MODULE:
            os << "MODULE";
            break;

        case DeclKind::STRUCT:
            os << "STRUCT";
            break;

        case DeclKind::FUNCTION:
            os << "FUNCTION";
            break;

        case DeclKind::COMPONENT:
            os << "COMPONENT";
            break;
    }

    return os;
}

struct Declaration
{
    uint32_t line = 0;
    uint32_t col = 0;
    DeclKind kind = DeclKind::INVALID;
    std::optional<uint64_t> symbol_idx;

    virtual ~Declaration() = default;
};


struct FieldDecl : Declaration
{
    FieldDecl() { kind = DeclKind::FIELD; }

    bool is_binding_mutable = false;
    bool is_pub = false;
    std::string name;
    std::unique_ptr<TypeDecl> type_decl;
    std::unique_ptr<Expression> init_expr; // nullptr if none.
};

// Data for the receiver parameter of a component receiver function
struct ReceiverData
{
    std::unique_ptr<TypeDecl> receiver_type_decl;
    std::string receiver_name;
    std::optional<uint64_t> symbol_idx;
};

struct FunctionDecl : Declaration
{
    FunctionDecl() { kind = DeclKind::FUNCTION; }

    bool is_pub = false;

    std::unique_ptr<TypeDecl> ret_type;
    std::vector<Parameter> params;
    std::string name;

    // Receiver component parameter, if this function is a receiver function.
    std::optional<Parameter> receiver_data;
    ScopeBody body;
};

struct StructDecl : Declaration
{
    StructDecl() { kind = DeclKind::STRUCT; }

    bool is_pub = false;
    std::string name;
    std::vector<std::unique_ptr<Declaration>> decls;
};

struct ComponentDecl : Declaration
{
    ComponentDecl() { kind = DeclKind::COMPONENT; }

    bool is_pub = false;
    std::string name;
    std::vector<std::unique_ptr<Declaration>> decls;
};

struct ModuleDecl : Declaration
{
    ModuleDecl() { kind = DeclKind::MODULE; }
    
    std::string ident;
    std::vector<std::unique_ptr<Declaration>> decls;
};


// Declarations.hpp

#pragma once

#include <string>
#include <cstdint>
#include <vector>


enum class ReferenceType
{
    NONE,
    REF,
    REFMUT
};

struct TypeDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    ReferenceType ref_type;
    std::vector<bool> ptr_depth_mutability;
};

struct FieldDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    bool binding_mutable;
    TypeDecl type_decl;
};

enum class ExpressionType
{
    BIN_LITERAL,
    HEX_LITERAL,
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    IDENTIFIER,

    UNARY,
    BINARY,
    TERNARY,
    ASSIGN,
    CAST,
    COMPARISON,

    CALL,
    ARR_SUBSCRIPT,
    CONTAINER_SUBSCRIPT,
    PTR_MEMBER,

    POST_INC,
    POST_DEC,
    INC,
    DEC,
};

struct ExpressionDecl
{
    uint32_t line;
    uint32_t col;

    ExpressionType exp_type;
};

struct BinaryLitDecl : ExpressionDecl
{
    BinaryLitDecl() { exp_type = ExpressionType::BIN_LITERAL; }
};

struct HexLitDecl : ExpressionDecl
{
    HexLitDecl() { exp_type = ExpressionType::HEX_LITERAL; }
};

struct IntLitDecl : ExpressionDecl
{
    IntLitDecl() { exp_type = ExpressionType::INT_LITERAL; }
    std::string value;
};

struct FloatLitDecl : ExpressionDecl
{
    FloatLitDecl() { exp_type = ExpressionType::FLOAT_LITERAL; }
    std::string value;
};

struct StringLitDecl : ExpressionDecl
{
    StringLitDecl() { exp_type = ExpressionType::STRING_LITERAL; }
    std::string value;
};

struct IdentDecl : ExpressionDecl
{
    IdentDecl() { exp_type = ExpressionType::IDENTIFIER; }
    std::string name;
};

struct UnaryDecl : ExpressionDecl
{
    UnaryDecl() { exp_type = ExpressionType::UNARY; }

    enum class OperationType
    {
        PRE_INC,
        PRE_DEC,
        POST_INC,
        POST_DEC,
        BIT_NOT,
        LOG_NOT, 
        GET_ADDRESS,
        DEREF,
    };

    OperationType op_type;

    ExpressionDecl *operand;
};

struct BinaryDecl : ExpressionDecl
{
    BinaryDecl() { exp_type = ExpressionType::BINARY; }

    enum class OperationType
    {
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        LSHIFT,
        RSHIFT,
        BIT_AND,
        BIT_OR,
        BIT_XOR,
        LOG_AND,
        LOG_OR
    };

    OperationType op_type;

    // If this operation is an operation with assignment
    bool is_assign_op;

    ExpressionDecl *first_operand;
    ExpressionDecl *second_operand;
};

struct TernaryDecl : ExpressionDecl
{
    TernaryDecl() { exp_type = ExpressionType::TERNARY; }

    ExpressionDecl *condition;

    ExpressionDecl *on_true_result;
    ExpressionDecl *on_false_result;
};

struct AssignDecl : ExpressionDecl
{
    AssignDecl() { exp_type = ExpressionType::ASSIGN; }

    ExpressionDecl *lhs;
    ExpressionDecl *rhs;
};

struct CastDecl : ExpressionDecl
{
    CastDecl() { exp_type = ExpressionType::CAST; }

    TypeDecl *to_cast_type;
    ExpressionDecl *obj_to_cast;
};

struct CallDecl : ExpressionDecl
{
    CallDecl() { exp_type = ExpressionType::CALL; }

    FunctionDecl *targ_func;
};

struct ArraySubscriptDecl : ExpressionDecl
{
    ArraySubscriptDecl() { exp_type = ExpressionType::ARR_SUBSCRIPT; }

    IdentDecl *array;
    ExpressionDecl *index_exp;
};

struct ContainerSubscriptDecl : ExpressionDecl
{
    ContainerSubscriptDecl() { exp_type = ExpressionType::CONTAINER_SUBSCRIPT; }

    IdentDecl *container;
    IdentDecl *member;
};

struct ScopeDecl
{
    uint32_t line;
    uint32_t col;

    std::vector<ExpressionDecl> expression_decls;
};

struct ParameterDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    bool binding_mutable;
    TypeDecl type_decl;
};

struct FunctionDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    
    TypeDecl ret_type_decl;

    std::vector<ScopeDecl> scope_decls;
};


struct StructDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;

    
};

struct NamespaceDecl
{
    uint32_t line;
    uint32_t col;
    std::string name;
    std::vector<NamespaceDecl> namespace_decls;
    std::vector<StructDecl> structs_decls;
    std::vector<FunctionDecl> function_decls;
};


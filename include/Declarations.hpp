// Declarations.hpp

#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

// Forward Decls
struct TypeDecl;
struct FunctionDecl;
struct ComponentDecl;
struct StructDecl;


// EXPRESSIONS =================================================================

enum class ExpressionType
{
    INVALID, // Uninitialized Sentinel
    BIN_LITERAL,
    HEX_LITERAL,
    INT_LITERAL,
    FLOAT_LITERAL,
    STR_LITERAL,
    IDENTIFIER,

    UNARY,
    BINARY,
    TERNARY,
    ASSIGN,

    CALL,
    CAST,
    SUBSCRIPT,
    MEMBER_ACCESS
};

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
    DEREF
};

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
};

struct BinaryLitExpr : Expression
{
    BinaryLitExpr() { exp_type = ExpressionType::BIN_LITERAL; }
    std::string value;
};

struct HexLitExpr : Expression
{
    HexLitExpr() { exp_type = ExpressionType::HEX_LITERAL; }
    std::string value;
};

struct IntLitExpr : Expression
{
    IntLitExpr() { exp_type = ExpressionType::INT_LITERAL; }
    std::string value;
};

struct FloatLitExpr : Expression
{
    FloatLitExpr() { exp_type = ExpressionType::FLOAT_LITERAL; }
    std::string value;
};

struct StringLitExpr : Expression
{
    StringLitExpr() { exp_type = ExpressionType::STR_LITERAL; }
    std::string value;
};

struct IdentExpr : Expression
{
    IdentExpr() { exp_type = ExpressionType::IDENTIFIER; }
    std::string name;
};

struct UnaryExpr : Expression
{
    UnaryExpr() { exp_type = ExpressionType::UNARY; }

    UnaryOp op_type = UnaryOp::INVALID;
    std::unique_ptr<Expression> operand;
};

struct BinaryExpr : Expression
{
    BinaryExpr() { exp_type = ExpressionType::BINARY; }

    BinaryOp op_type = BinaryOp::INVALID;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

struct TernaryExpr : Expression
{
    TernaryExpr() { exp_type = ExpressionType::TERNARY; }

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> on_true_expr;
    std::unique_ptr<Expression> on_false_expr;
};

struct AssignExpr : Expression
{
    AssignExpr() { exp_type = ExpressionType::ASSIGN; }

    AssignOp op_type = AssignOp::INVALID;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

struct CastExpr : Expression
{
    CastExpr() { exp_type = ExpressionType::CAST; }

    std::unique_ptr<TypeDecl> to_cast_type;
    std::unique_ptr<Expression> expr_to_cast;
};

struct CallExpr : Expression
{
    CallExpr() { exp_type = ExpressionType::CALL; }

    std::unique_ptr<Expression> callee_expr;
    std::vector<std::unique_ptr<Expression>> args;
    std::optional<uint64_t> resolved_symbol_idx;
};

struct SubscriptExpr : Expression
{
    SubscriptExpr() { exp_type = ExpressionType::SUBSCRIPT; }

    std::unique_ptr<Expression> base_expr;
    std::unique_ptr<Expression> index_expr;
};

struct MemberAccExpr : Expression
{
    MemberAccExpr() { exp_type = ExpressionType::MEMBER_ACCESS; }

    bool via_pointer = false;

    std::unique_ptr<Expression> base_expr;
    std::string member_name;

    // Index into the symbol table of the resolved symbol.
    std::optional<uint64_t> resolved_symbol_idx;
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

    std::unique_ptr<Expression> ret_expr; // Should not be null, must have ret
};

struct ScopeBody
{
    uint32_t line = 0;
    uint32_t col = 0;

    std::vector<std::unique_ptr<Statement>> statements;
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

enum class TypeKind
{
    INVALID, // Uninitialized Sentinel
    NAMED,
    PTR,
    REF,
    ARRAY,
    FUNC_PTR
};

struct TypeDecl
{
    uint32_t line = 0;
    uint32_t col = 0;
    TypeKind kind = TypeKind::INVALID;

    virtual ~TypeDecl() = default;
};

struct NamedTypeDecl : TypeDecl
{
    NamedTypeDecl() { kind = TypeKind::NAMED; }

    std::string type_name;
    
    // Index into the symbol table of the resolved type.
    std::optional<uint64_t> resolved_symbol_idx;
};

struct PtrTypeDecl : TypeDecl
{
    PtrTypeDecl() { kind = TypeKind::PTR; }

    bool points_to_mutable = false;
    std::unique_ptr<TypeDecl> pointee;
};

struct RefTypeDecl : TypeDecl
{
    RefTypeDecl() { kind = TypeKind::REF; }

    bool ref_to_mutable = false;
    std::unique_ptr<TypeDecl> referred;
};

struct ArrTypeDecl : TypeDecl
{
    ArrTypeDecl() { kind = TypeKind::ARRAY; }

    uint8_t depth = 1;

    std::unique_ptr<TypeDecl> element_type;
    std::vector<std::unique_ptr<Expression>> size_exprs;
};

struct Parameter
{
    uint32_t line = 0;
    uint32_t col = 0;
    std::string name;
    bool is_unqual_param = false;
    bool is_binding_mutable = false;
    std::unique_ptr<TypeDecl> type_decl;
};

struct FuncPtrDecl : TypeDecl
{
    FuncPtrDecl() { kind = TypeKind::FUNC_PTR; }

    std::unique_ptr<TypeDecl> ret_type;
    std::vector<Parameter> param_types;
};

enum class DeclKind
{
    INVALID,
    FIELD,
    NAMESPACE,
    STRUCT,
    FUNCTION,
    COMPONENT
};

struct Declaration
{
    uint32_t line = 0;
    uint32_t col = 0;
    DeclKind kind = DeclKind::INVALID;
    std::string name;

    virtual ~Declaration() = default;
};

struct FieldDecl : Declaration
{
    FieldDecl() { kind = DeclKind::FIELD; }

    bool is_binding_mutable = false;
    bool is_pub = false;
    std::unique_ptr<TypeDecl> type_decl;
};

// Data for the receiver parameter of a component receiver function
struct ReceiverData
{
    std::unique_ptr<TypeDecl> receiver_type_decl;
    std::string receiver_name;
};


struct FunctionDecl : Declaration
{
    FunctionDecl() { kind = DeclKind::FUNCTION; }

    bool is_pub = false;

    std::unique_ptr<TypeDecl> ret_type;
    std::vector<Parameter> params;
    
    // Receiver component parameter, if this function is a receiver function.
    std::optional<ReceiverData> receiver_data;
    ScopeBody body;
};

struct StructDecl : Declaration
{
    StructDecl() { kind = DeclKind::STRUCT; }

    bool is_pub = false;

    std::vector<std::unique_ptr<Declaration>> decls;
};

struct ComponentDecl : Declaration
{
    ComponentDecl() { kind = DeclKind::COMPONENT; }

    bool is_pub = false;

    std::vector<std::unique_ptr<Declaration>> decls;
};

struct NamespaceDecl : Declaration
{
    NamespaceDecl() { kind = DeclKind::NAMESPACE; }

    std::vector<std::unique_ptr<Declaration>> decls;
};


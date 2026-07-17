// OperatorPrecedence.hpp

#pragma once

#include <cstdint>

#include "Token.hpp"

enum class Associativity
{
    NONE,
    LEFT,
    RIGHT
};

struct OpData
{
    uint32_t precedence = 0;
    Associativity assoc = Associativity::NONE;
};

static constexpr OpData op_precedence[] =
{
    /* END_OF_FILE */         {  0, Associativity::NONE },

    /* IDENTIFIER */          {  0, Associativity::NONE },
    /* COMPILE_DIRECTIVE */   {  0, Associativity::NONE },
    /* INT_LITERAL */         {  0, Associativity::NONE },
    /* FLOAT_LITERAL */       {  0, Associativity::NONE },
    /* HEX_LITERAL */         {  0, Associativity::NONE },
    /* BIN_LITERAL */         {  0, Associativity::NONE },
    /* STR_LITERAL */         {  0, Associativity::NONE },
    /* BOOL_LITERAL */        {  0, Associativity::NONE },
    /* NULLPTR_LITERAL */     {  0, Associativity::NONE },
    /* CHAR_LITERAL */        {  0, Associativity::NONE },

    // Keywords

    /* KW_FN */               {  0, Associativity::NONE },
    /* KW_MODULE */           {  0, Associativity::NONE },
    /* KW_TYPE */             {  0, Associativity::NONE },
    /* KW_STRUCT */           {  0, Associativity::NONE },
    /* KW_COMPONENT */        {  0, Associativity::NONE },
    /* KW_PUB */              {  0, Associativity::NONE },
    /* KW_MUT */              {  0, Associativity::NONE },
    /* KW_REF */              {  0, Associativity::NONE },
    /* KW_VAL */              {  0, Associativity::NONE },
    /* KW_RET */              {  0, Associativity::NONE },
    /* KW_IF */               {  0, Associativity::NONE },
    /* KW_ELSE */             {  0, Associativity::NONE },
    /* KW_WHILE */            {  0, Associativity::NONE },
    /* KW_FOR */              {  0, Associativity::NONE },
    /* KW_BREAK */            {  0, Associativity::NONE },
    /* KW_CONTINUE */         {  0, Associativity::NONE },
    /* KW_EXPORT */           {  0, Associativity::NONE },
    /* KW_LET */              {  0, Associativity::NONE },

    // Brackets

    /* LPAREN */              { 90, Associativity::LEFT }, // function call
    /* RPAREN */              {  0, Associativity::NONE },
    /* LBRACE */              {  90, Associativity::LEFT },
    /* RBRACE */              {  0, Associativity::NONE },
    /* LBRACKET */            { 90, Associativity::LEFT }, // subscript
    /* RBRACKET */            {  0, Associativity::NONE },

    // Punctuation

    /* COMMA */               {  0, Associativity::NONE },
    /* SEMICOLON */           {  0, Associativity::NONE },
    /* DOT */                 { 90, Associativity::LEFT },
    /* COLON */               {  0, Associativity::NONE },

    // Special Characters

    /* EXCLAMATION_POINT */   {  0, Associativity::NONE }, // prefix unary
    /* AMPERSAND */           {  24, Associativity::LEFT }, // bitwise AND
    /* TILDE */               {  0, Associativity::NONE }, // prefix unary
    /* DOLLAR_SIGN */         {  0, Associativity::NONE }, // prefix unary
    /* CARROT */              {  23, Associativity::LEFT }, // bitwise XOR

    // Operators

    /* SCOPE_RESOLUTION */    { 0, Associativity::NONE },
    /* TERNARY */             { 12, Associativity::RIGHT },

    /* ARROW */               { 90, Associativity::LEFT },

    /* ASSIGN */              { 10, Associativity::RIGHT },

    /* PLUS */                { 50, Associativity::LEFT },
    /* PLUS_ASSIGN */         { 10, Associativity::RIGHT },

    /* MINUS */               { 50, Associativity::LEFT },
    /* MINUS_ASSIGN */        { 10, Associativity::RIGHT },

    /* ASTERISK */            { 60, Associativity::LEFT },
    /* ASTERISK_ASSIGN */     { 10, Associativity::RIGHT },

    /* FORW_SLASH */          { 60, Associativity::LEFT },
    /* FORW_SLASH_ASSIGN */   { 10, Associativity::RIGHT },

    /* BACK_SLASH */          {  0, Associativity::NONE },

    /* MODULO */              { 60, Associativity::LEFT },
    /* MODULO_ASSIGN */       { 10, Associativity::RIGHT },
    /* BIT_AND_ASSIGN */      { 10, Associativity::RIGHT },
    /* BIT_OR_ASSIGN */       { 10, Associativity::RIGHT },
    /* BIT_XOR_ASSIGN */      { 10, Associativity::RIGHT },

    /* VERT_BAR */            {  22, Associativity::LEFT }, // bitwise OR

    /* EQUAL_EQUAL */         { 30, Associativity::LEFT },

    /* PLUS_PLUS */           { 90, Associativity::LEFT }, // postfix
    /* MINUS_MINUS */         { 90, Associativity::LEFT }, // postfix

    /* NOT_EQUAL */           { 30, Associativity::LEFT },

    /* LESS_THAN */           { 40, Associativity::LEFT },
    /* LESS_EQUAL */          { 40, Associativity::LEFT },

    /* GREATER_THAN */        { 40, Associativity::LEFT },
    /* GREATER_EQUAL */       { 40, Associativity::LEFT },

    /* LOGIC_AND */           { 20, Associativity::LEFT },
    /* LOGIC_OR */            { 15, Associativity::LEFT },

    /* SHIFT_LEFT */          { 45, Associativity::LEFT },
    /* SHIFT_LEFT_ASSIGN */   { 10, Associativity::RIGHT },
    /* SHIFT_RIGHT */         { 45, Associativity::LEFT },
    /* SHIFT_RIGHT_ASSIGN */  { 10, Associativity::RIGHT }
};

static constexpr uint32_t PREFIX_PRECEDENCE = 80;

static inline const OpData &get_opdata(TokenID id)
{
    return op_precedence[static_cast<size_t>(id)];
}
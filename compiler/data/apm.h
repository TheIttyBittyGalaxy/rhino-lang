#ifndef APM_H
#define APM_H

#include "list.h"
#include "macro.h"
#include "substr.h"
#include <stdbool.h>

// Expression
#define LIST_EXPRESSIONS(MACRO) \
    MACRO(INVALID_EXPRESSION)   \
                                \
    MACRO(IDENTITY_LITERAL)     \
    MACRO(BOOLEAN_LITERAL)      \
    MACRO(STRING_LITERAL)       \
                                \
    MACRO(FUNCTION_CALL)

DECLARE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)

DECLARE_SLICE_TYPE(Expression, expression)

typedef struct
{
    ExpressionKind kind;
    union
    {
        struct
        { // IDENTITY_LITERAL
            substr identity;
        };
        struct
        { // BOOLEAN_LITERAL
            bool bool_value;
        };
        struct
        { // STRING_LITERAL
            substr string_value;
        };
        struct // FUNCTION_CALL
        {
            size_t callee; // Expression
        };
    };
} Expression;

DECLARE_LIST_TYPE(Expression, expression)

// Statement
#define LIST_STATEMENTS(MACRO) \
    MACRO(INVALID_STATEMENT)   \
                               \
    MACRO(CODE_BLOCK)          \
    MACRO(SINGLE_BLOCK)        \
                               \
    MACRO(IF_STATEMENT)        \
    MACRO(ELSE_IF_STATEMENT)   \
    MACRO(ELSE_STATEMENT)      \
    MACRO(OUTPUT_STATEMENT)    \
                               \
    MACRO(EXPRESSION_STMT)

DECLARE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)

DECLARE_SLICE_TYPE(Statement, statement)

typedef struct
{
    StatementKind kind;
    union
    {
        struct // CODE_BLOCK / SINGLE_BLOCK
        {
            StatementSlice statements;
        };
        struct // IF_STATEMENT / ELSE_SEGMENT
        {
            size_t condition; // Expression
            size_t body;      // Statement
        };
        struct // OUTPUT_STATEMENT / EXPRESSION_STMT
        {
            size_t expression; // Expression
        };
    };
} Statement;

DECLARE_LIST_TYPE(Statement, statement)

// Function
DECLARE_SLICE_TYPE(Function, function)

typedef struct
{
    substr identity;
    size_t body;
} Function;

DECLARE_LIST_TYPE(Function, function)

// Program
typedef struct
{
    FunctionList function;
    StatementList statement;
    ExpressionList expression;

    size_t main;
} Program;

void init_program(Program *apm);
void dump_apm(Program *apm, const char *source_text);
void print_apm(Program *apm, const char *source_text);

// APM utility methods
size_t get_next_statement_in_code_block(Program *apm, Statement *code_block, size_t n);
size_t get_first_statement_in_code_block(Program *apm, Statement *code_block);
size_t get_last_statement_in_code_block(Program *apm, Statement *code_block);

#endif
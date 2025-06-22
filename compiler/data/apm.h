#ifndef APM_H
#define APM_H

#include "list.h"
#include "macro.h"
#include "substr.h"
#include <stdbool.h>

// Types

#define LIST_RHINO_TYPES(MACRO) \
    MACRO(INVALID_RHINO_TYPE)   \
    MACRO(RHINO_INT)            \
    MACRO(RHINO_NUM)            \
    MACRO(RHINO_STR)

DECLARE_ENUM(LIST_RHINO_TYPES, RhinoType, rhino_type)

// Variable
DECLARE_SLICE_TYPE(Variable, variable)

typedef struct
{
    substr identity;
    RhinoType type;
} Variable;

DECLARE_LIST_TYPE(Variable, variable)

// Expression
#define LIST_EXPRESSIONS(MACRO) \
    MACRO(INVALID_EXPRESSION)   \
                                \
    MACRO(IDENTITY_LITERAL)     \
    MACRO(NUMBER_LITERAL)       \
    MACRO(BOOLEAN_LITERAL)      \
    MACRO(STRING_LITERAL)       \
                                \
    MACRO(VARIABLE_REFERENCE)   \
    MACRO(FUNCTION_CALL)

DECLARE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)

DECLARE_SLICE_TYPE(Expression, expression)

typedef struct
{
    ExpressionKind kind;
    substr span;
    union
    {
        struct
        { // IDENTITY_LITERAL
            substr identity;
            bool identity_resolved;
        };
        struct
        { // BOOLEAN_LITERAL
            bool bool_value;
        };
        struct
        { // NUMBER_LITERAL
            int number_value;
        };
        struct
        { // STRING_LITERAL
            substr string_value;
        };
        struct // VARIABLE_REFERENCE
        {
            size_t variable;
        };
        struct // FUNCTION_CALL
        {
            size_t callee; // Expression -> Function
        };
    };
} Expression;

DECLARE_LIST_TYPE(Expression, expression)

// Statement
#define LIST_STATEMENTS(MACRO)  \
    MACRO(INVALID_STATEMENT)    \
                                \
    MACRO(CODE_BLOCK)           \
    MACRO(SINGLE_BLOCK)         \
                                \
    MACRO(IF_SEGMENT)           \
    MACRO(ELSE_IF_SEGMENT)      \
    MACRO(ELSE_SEGMENT)         \
                                \
    MACRO(ASSIGNMENT_STATEMENT) \
    MACRO(VARIABLE_DECLARATION) \
                                \
    MACRO(OUTPUT_STATEMENT)     \
    MACRO(EXPRESSION_STMT)

DECLARE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)

DECLARE_SLICE_TYPE(Statement, statement)

typedef struct
{
    StatementKind kind;
    substr span;
    union
    {
        struct // CODE_BLOCK / SINGLE_BLOCK
        {
            StatementSlice statements;
        };
        struct // IF_SEGMENT / ELSE_IF_SEGMENT / ELSE_SEGMENT
        {
            size_t condition; // Expression
            size_t body;      // Statement
        };
        struct // ASSIGNMENT_STATEMENT
        {
            size_t assignment_lhs; // Expression
            size_t assignment_rhs; // Expression
        };
        struct // VARIABLE_DECLARATION
        {
            size_t variable;        // Variable
            size_t initial_value;   // Expression
            size_t type_expression; // Expression
            bool has_initial_value;
            bool has_type_expression;
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
    substr span;
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
    VariableList variable;

    size_t main;
} Program;

void init_program(Program *apm);

// Display APM
void dump_apm(Program *apm, const char *source_text);
void print_parsed_apm(Program *apm, const char *source_text);
void print_analysed_apm(Program *apm, const char *source_text);

// APM utility methods
size_t get_next_statement_in_code_block(Program *apm, Statement *code_block, size_t n);
size_t get_first_statement_in_code_block(Program *apm, Statement *code_block);
size_t get_last_statement_in_code_block(Program *apm, Statement *code_block);

#endif
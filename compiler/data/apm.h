#ifndef APM_H
#define APM_H

#include "list.h"
#include "macro.h"
#include "substr.h"
#include <stdbool.h>

// Types
#define LIST_RHINO_SORTS(MACRO) \
    MACRO(INVALID_SORT)         \
    MACRO(SORT_BOOL)            \
    MACRO(SORT_INT)             \
    MACRO(SORT_NUM)             \
    MACRO(SORT_STR)             \
    MACRO(SORT_ENUM)

DECLARE_ENUM(LIST_RHINO_SORTS, RhinoSort, rhino_sort)

typedef struct
{
    RhinoSort sort;
    size_t index;
} RhinoType;

// Enum value
DECLARE_SLICE_TYPE(EnumValue, enum_value)

typedef struct
{
    substr span;
    substr identity;
} EnumValue;

DECLARE_LIST_TYPE(EnumValue, enum_value)

// Enum type
DECLARE_SLICE_TYPE(EnumType, enum_type)

typedef struct
{
    substr span;
    substr identity;
    EnumValueSlice values;
} EnumType;

DECLARE_LIST_TYPE(EnumType, enum_type)

// Variable
DECLARE_SLICE_TYPE(Variable, variable)

typedef struct
{
    substr identity;
    RhinoType type;
} Variable;

DECLARE_LIST_TYPE(Variable, variable)

// Expression Precedence
// Ordered from "happens last" to "happens first"
#define LIST_EXPR_PRECEDENCE(MACRO)    \
    MACRO(PRECEDENCE_NONE)             \
    MACRO(PRECEDENCE_LOGICAL_OR)       \
    MACRO(PRECEDENCE_LOGICAL_AND)      \
    MACRO(PRECEDENCE_COMPARE_EQUAL)    \
    MACRO(PRECEDENCE_COMPARE_RELATIVE) \
    MACRO(PRECEDENCE_TERM)             \
    MACRO(PRECEDENCE_FACTOR)           \
    MACRO(PRECEDENCE_UNARY)            \
    MACRO(PRECEDENCE_INDEX)            \
    MACRO(PRECEDENCE_CALL)

DECLARE_ENUM(LIST_EXPR_PRECEDENCE, ExprPrecedence, expr_precedence)

// Expression
#define LIST_EXPRESSIONS(MACRO)      \
    MACRO(INVALID_EXPRESSION)        \
                                     \
    MACRO(IDENTITY_LITERAL)          \
    MACRO(INTEGER_LITERAL)           \
    MACRO(FLOAT_LITERAL)             \
    MACRO(BOOLEAN_LITERAL)           \
    MACRO(STRING_LITERAL)            \
    MACRO(ENUM_VALUE_LITERAL)        \
                                     \
    MACRO(VARIABLE_REFERENCE)        \
    MACRO(TYPE_REFERENCE)            \
                                     \
    MACRO(FUNCTION_CALL)             \
                                     \
    MACRO(INDEX_BY_FIELD)            \
                                     \
    MACRO(BINARY_MULTIPLY)           \
    MACRO(BINARY_DIVIDE)             \
    MACRO(BINARY_REMAINDER)          \
    MACRO(BINARY_ADD)                \
    MACRO(BINARY_SUBTRACT)           \
    MACRO(BINARY_LESS_THAN)          \
    MACRO(BINARY_GREATER_THAN)       \
    MACRO(BINARY_LESS_THAN_EQUAL)    \
    MACRO(BINARY_GREATER_THAN_EQUAL) \
    MACRO(BINARY_EQUAL)              \
    MACRO(BINARY_NOT_EQUAL)          \
    MACRO(BINARY_LOGICAL_AND)        \
    MACRO(BINARY_LOGICAL_OR)

DECLARE_ENUM(LIST_EXPRESSIONS, ExpressionKind, expression_kind)

DECLARE_SLICE_TYPE(Expression, expression)

typedef struct
{
    ExpressionKind kind;
    substr span;
    union
    {
        struct // IDENTITY_LITERAL
        {
            substr identity;
            bool identity_resolved;
        };
        struct // BOOLEAN_LITERAL
        {
            bool bool_value;
        };
        struct // INTEGER_LITERAL
        {
            int integer_value;
        };
        struct // FLOAT_LITERAL
        {
            double float_value;
        };
        struct // STRING_LITERAL
        {
            substr string_value;
        };
        struct // ENUM_VALUE_LITERAL
        {
            size_t enum_value; // EnumValue
        };
        struct // VARIABLE_REFERENCE
        {
            size_t variable;
        };
        struct // TYPE_REFERENCE
        {
            RhinoType type;
        };
        struct // FUNCTION_CALL
        {
            size_t callee; // Expression -> Function
        };
        struct // INDEX_BY_FIELD
        {
            size_t subject; // Expression
            substr field;
        };
        struct // BINARY_*
        {
            size_t lhs; // Expression
            size_t rhs; // Expression
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
    MACRO(BREAK_LOOP)           \
    MACRO(FOR_LOOP)             \
    MACRO(BREAK_STATEMENT)      \
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
            size_t body;      // Statement
            size_t condition; // Expression
        };
        struct // BREAK_LOOP
        {
            size_t __loop_body; // NOTE: KEEP SYNCED WITH IF_SEGMENT body
        };
        struct // FOR_LOOP
        {
            size_t __for_body; // NOTE: KEEP SYNCED WITH IF_SEGMENT body
            size_t iterator;   // Variable

            // TODO: Rather than storing a first and last point, we should store the thing being iterated on as an expression
            size_t first; // Expression
            size_t last;  // Expression
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

    EnumTypeList enum_type;
    EnumValueList enum_value;

    size_t main;
} Program;

void init_program(Program *apm);

// Display APM
const char *rhino_type_string(Program *apm, RhinoType ty);
void dump_apm(Program *apm, const char *source_text);
void print_parsed_apm(Program *apm, const char *source_text);
void print_analysed_apm(Program *apm, const char *source_text);

// Access methods
size_t get_next_statement_in_code_block(Program *apm, Statement *code_block, size_t n);
size_t get_first_statement_in_code_block(Program *apm, Statement *code_block);
size_t get_last_statement_in_code_block(Program *apm, Statement *code_block);

// Type analysis methods
RhinoType get_expression_type(Program *apm, size_t expr_index);
size_t get_enum_type_of_enum_value(Program *apm, size_t enum_value_index);
bool is_expression_boolean(Program *apm, size_t expr_index);
bool are_types_equal(RhinoType a, RhinoType b);
bool can_assign_a_to_b(RhinoType a, RhinoType b);

// Expression precedence methods
ExprPrecedence precedence_of(ExpressionKind expr_kind);

#endif
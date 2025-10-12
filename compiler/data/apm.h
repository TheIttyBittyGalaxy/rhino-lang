#ifndef APM_H
#define APM_H

#include "list.h"
#include "macro.h"
#include "substr.h"
#include <stdbool.h>

// Types
#define LIST_RHINO_SORTS(MACRO) \
    MACRO(INVALID_SORT)         \
    MACRO(ERROR_SORT)           \
                                \
    MACRO(SORT_NONE)            \
    MACRO(SORT_BOOL)            \
    MACRO(SORT_INT)             \
    MACRO(SORT_NUM)             \
    MACRO(SORT_STR)             \
    MACRO(SORT_ENUM)            \
    MACRO(SORT_STRUCT)

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

// Property
DECLARE_SLICE_TYPE(Property, property)

typedef struct
{
    substr span;
    substr identity;
    size_t type_expression;
    RhinoType type;
} Property;

DECLARE_LIST_TYPE(Property, property)

// Struct type
DECLARE_SLICE_TYPE(StructType, struct_type)

typedef struct
{
    substr span;
    substr identity;
    PropertySlice properties;
} StructType;

DECLARE_LIST_TYPE(StructType, struct_type)

// Variable
DECLARE_SLICE_TYPE(Variable, variable)

typedef struct
{
    substr identity;
    RhinoType type;
} Variable;

DECLARE_LIST_TYPE(Variable, variable)

// Symbol table
#define LIST_SYMBOL_TAG(MACRO) \
    MACRO(INVALID_SYMBOL)      \
    MACRO(VARIABLE_SYMBOL)     \
    MACRO(FUNCTION_SYMBOL)     \
    MACRO(ENUM_TYPE_SYMBOL)    \
    MACRO(STRUCT_TYPE_SYMBOL)

DECLARE_ENUM(LIST_SYMBOL_TAG, SymbolTag, symbol_tag)

typedef struct
{
    SymbolTag tag;
    size_t index;
    substr identity;
} Symbol;

#define SYMBOL_TABLE_SIZE 16

typedef struct
{
    size_t next;
    size_t symbol_count;
    Symbol symbol[SYMBOL_TABLE_SIZE];
} SymbolTable;

DECLARE_SLICE_TYPE(SymbolTable, symbol_table)
DECLARE_LIST_TYPE(SymbolTable, symbol_table)

void init_symbol_table(SymbolTable *symbol_table);
void set_symbol_table_parent(SymbolTable *symbol_table, size_t parent_index);

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
    MACRO(PRECEDENCE_RANGE)            \
    MACRO(PRECEDENCE_UNARY)            \
    MACRO(PRECEDENCE_INDEX)            \
    MACRO(PRECEDENCE_CALL_OR_INCREMENT)

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
    MACRO(FUNCTION_REFERENCE)        \
    MACRO(TYPE_REFERENCE)            \
                                     \
    MACRO(FUNCTION_CALL)             \
                                     \
    MACRO(INDEX_BY_FIELD)            \
                                     \
    MACRO(RANGE_LITERAL)             \
                                     \
    MACRO(UNARY_POS)                 \
    MACRO(UNARY_NEG)                 \
    MACRO(UNARY_INCREMENT)           \
    MACRO(UNARY_DECREMENT)           \
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
            bool given_error;
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
        struct // FUNCTION_REFERENCE
        {
            size_t function;
        };
        struct // TYPE_REFERENCE
        {
            RhinoType type;
        };
        struct // FUNCTION_CALL
        {
            size_t callee; // Expression
        };
        struct // INDEX_BY_FIELD
        {
            size_t subject; // Expression
            substr field;
        };
        struct // RANGE
        {
            size_t first; // Expression
            size_t last;  // Expression
        };
        struct // UNARY_*
        {
            size_t operand; // Expression
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
#define LIST_STATEMENTS(MACRO)     \
    MACRO(INVALID_STATEMENT)       \
                                   \
    MACRO(FUNCTION_DECLARATION)    \
    MACRO(ENUM_TYPE_DECLARATION)   \
    MACRO(STRUCT_TYPE_DECLARATION) \
    MACRO(VARIABLE_DECLARATION)    \
                                   \
    MACRO(DECLARATION_BLOCK)       \
    MACRO(CODE_BLOCK)              \
    MACRO(SINGLE_BLOCK)            \
                                   \
    MACRO(IF_SEGMENT)              \
    MACRO(ELSE_IF_SEGMENT)         \
    MACRO(ELSE_SEGMENT)            \
                                   \
    MACRO(BREAK_LOOP)              \
    MACRO(FOR_LOOP)                \
    MACRO(BREAK_STATEMENT)         \
                                   \
    MACRO(ASSIGNMENT_STATEMENT)    \
                                   \
    MACRO(OUTPUT_STATEMENT)        \
    MACRO(EXPRESSION_STMT)         \
    MACRO(RETURN_STATEMENT)

DECLARE_ENUM(LIST_STATEMENTS, StatementKind, statement_kind)

DECLARE_SLICE_TYPE(Statement, statement)

typedef struct
{
    StatementKind kind;
    substr span;
    union
    {
        struct // VARIABLE_DECLARATION
        {
            size_t variable;        // Variable
            size_t initial_value;   // Expression
            size_t type_expression; // Expression
            bool has_type_expression;
            bool has_initial_value;
        };
        struct // FUNCTION_DECLARATION
        {
            size_t function; // Function
        };
        struct // ENUM_TYPE_DECLARATION
        {
            size_t enum_type; // EnumType
        };
        struct // STRUCT_TYPE_DECLARATION
        {
            size_t struct_type; // StructType
        };
        struct // DECLARATION_BLOCK / CODE_BLOCK / SINGLE_BLOCK
        {
            StatementSlice statements;
            size_t symbol_table;
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
            size_t iterable;   // Expression
        };
        struct // ASSIGNMENT_STATEMENT
        {
            size_t assignment_lhs; // Expression
            size_t assignment_rhs; // Expression
        };
        struct // OUTPUT_STATEMENT / EXPRESSION_STMT / RETURN_STATEMENT
        {
            size_t expression; // Expression
        };
    };
} Statement;

DECLARE_LIST_TYPE(Statement, statement)

// Parameter
DECLARE_SLICE_TYPE(Parameter, parameter)

typedef struct
{
    substr span;
    substr identity;
    size_t type_expression;
    RhinoType type;
} Parameter;

DECLARE_LIST_TYPE(Parameter, parameter)

// Function
DECLARE_SLICE_TYPE(Function, function)

typedef struct
{
    substr span;
    substr identity;
    size_t body; // Statement

    size_t return_type_expression; // Expression
    bool has_return_type_expression;
    RhinoType return_type;

    ParameterSlice parameters;
} Function;

DECLARE_LIST_TYPE(Function, function)

// Program
typedef struct
{
    FunctionList function;
    ParameterList parameter;

    StatementList statement;
    ExpressionList expression;
    VariableList variable;

    EnumTypeList enum_type;
    EnumValueList enum_value;

    StructTypeList struct_type;
    PropertyList property;

    SymbolTableList symbol_table;

    size_t main; // Function

    size_t program_block;       // Statement
    size_t global_symbol_table; // SymbolTable
} Program;

void init_program(Program *apm);

void append_symbol(Program *apm, size_t table_index, SymbolTag symbol_tag, size_t symbol_index, substr symbol_identity);

// Display APM
const char *rhino_type_string(Program *apm, RhinoType ty);
void dump_apm(Program *apm, const char *source_text);
void print_parsed_apm(Program *apm, const char *source_text);
void print_resolved_apm(Program *apm, const char *source_text);

// Access methods
size_t get_next_statement_in_block(Program *apm, Statement *code_block, size_t n);
size_t get_first_statement_in_block(Program *apm, Statement *code_block);
size_t get_last_statement_in_block(Program *apm, Statement *code_block);

// Type analysis methods
RhinoType get_expression_type(Program *apm, const char *source_text, size_t expr_index);
size_t get_enum_type_of_enum_value(Program *apm, size_t enum_value_index);
bool are_types_equal(RhinoType a, RhinoType b);
bool allow_assign_a_to_b(RhinoType a, RhinoType b);

// Expression precedence methods
ExprPrecedence precedence_of(ExpressionKind expr_kind);

#endif
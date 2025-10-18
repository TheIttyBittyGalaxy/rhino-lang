#ifndef APM_H
#define APM_H

#include "list.h"
#include "macro.h"
#include "../memory.h"
#include "substr.h"
#include <stdbool.h>

// Forward Declarations

typedef struct EnumValue EnumValue;
typedef struct EnumType EnumType;
DECLARE_LIST_ALLOCATOR(EnumValue, enum_value)
DECLARE_LIST_ALLOCATOR(EnumType, enum_type)

typedef struct StructType StructType;
DECLARE_LIST_ALLOCATOR(StructType, struct_type)

typedef struct Variable Variable;
DECLARE_LIST_ALLOCATOR(Variable, variable)

typedef struct Expression Expression;
DECLARE_LIST_ALLOCATOR(Expression, expression)

typedef struct Statement Statement;
DECLARE_LIST_ALLOCATOR(Statement, statement)

typedef struct Block Block;
DECLARE_LIST_ALLOCATOR(Block, block)

typedef struct Function Function;
DECLARE_LIST_ALLOCATOR(Function, function)

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
    union
    {
        EnumType *enum_type;
        StructType *struct_type;
    };
} RhinoType;

// Enum value
struct EnumValue
{
    substr span;
    substr identity;
    EnumType *type_of_enum_value; // FIXME: I would like to not have this if possible
};

// Enum type
struct EnumType
{
    substr span;
    substr identity;
    EnumValueList values;
};

// Property
DECLARE_SLICE_TYPE(Property, property)

typedef struct
{
    substr span;
    substr identity;
    Expression *type_expression;
    RhinoType type;
} Property;

DECLARE_LIST_TYPE(Property, property)

// Struct type
struct StructType
{
    substr span;
    substr identity;
    PropertySlice properties;
    Block *declarations;
};

// Variable
struct Variable
{
    substr identity;
    RhinoType type;
};

// Symbol table
#define LIST_SYMBOL_TAG(MACRO) \
    MACRO(INVALID_SYMBOL)      \
    MACRO(VARIABLE_SYMBOL)     \
    MACRO(FUNCTION_SYMBOL)     \
    MACRO(PARAMETER_SYMBOL)    \
    MACRO(ENUM_TYPE_SYMBOL)    \
    MACRO(STRUCT_TYPE_SYMBOL)

DECLARE_ENUM(LIST_SYMBOL_TAG, SymbolTag, symbol_tag)

// FIXME: This is a hot patch while I rework how memory is managed in the compiler.
//        In the long term I would hope to factor this out, or at least make it better.
typedef struct
{
    union
    {
        size_t index;
        Function *function;
        Variable *variable;
        EnumType *enum_type;
        StructType *struct_type;
    };
} SymbolPointer;

typedef struct
{
    SymbolTag tag;
    SymbolPointer to;
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

// Argument
DECLARE_SLICE_TYPE(Argument, argument)

typedef struct
{
    Expression *expr;
} Argument;

DECLARE_LIST_TYPE(Argument, argument)

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
    MACRO(PARAMETER_REFERENCE)       \
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

struct Expression
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
            EnumValue *enum_value;
        };
        struct // VARIABLE_REFERENCE
        {
            Variable *variable;
        };
        struct // FUNCTION_REFERENCE
        {
            Function *function;
        };
        struct // PARAMETER_REFERENCE
        {
            size_t parameter;
        };
        struct // TYPE_REFERENCE
        {
            RhinoType type;
        };
        struct // FUNCTION_CALL
        {
            Expression *callee;
            ArgumentSlice arguments;
        };
        struct // INDEX_BY_FIELD
        {
            Expression *subject;
            substr field;
        };
        struct // RANGE
        {
            Expression *first;
            Expression *last;
        };
        struct // UNARY_*
        {
            Expression *operand;
        };
        struct // BINARY_*
        {
            Expression *lhs;
            Expression *rhs;
        };
    };
};

// Statement
#define LIST_STATEMENTS(MACRO)     \
    MACRO(INVALID_STATEMENT)       \
                                   \
    MACRO(FUNCTION_DECLARATION)    \
    MACRO(ENUM_TYPE_DECLARATION)   \
    MACRO(STRUCT_TYPE_DECLARATION) \
    MACRO(VARIABLE_DECLARATION)    \
                                   \
    MACRO(CODE_BLOCK)              \
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

struct Statement
{
    StatementKind kind;
    substr span;
    union
    {
        struct // VARIABLE_DECLARATION
        {
            Variable *variable;
            Expression *initial_value;
            Expression *type_expression;
            bool has_type_expression;
            bool has_initial_value;
        };
        struct // FUNCTION_DECLARATION
        {
            Function *function;
        };
        struct // ENUM_TYPE_DECLARATION
        {
            EnumType *enum_type;
        };
        struct // STRUCT_TYPE_DECLARATION
        {
            StructType *struct_type;
        };
        struct // CODE_BLOCK
        {
            Block *block;
        };
        struct // IF_SEGMENT / ELSE_IF_SEGMENT / ELSE_SEGMENT
        {
            Block *body;
            Expression *condition;
        };
        struct // BREAK_LOOP
        {
            Block *__loop_body; // NOTE: KEEP SYNCED WITH IF_SEGMENT body
        };
        struct // FOR_LOOP
        {
            Block *__for_body; // NOTE: KEEP SYNCED WITH IF_SEGMENT body
            Variable *iterator;
            Expression *iterable;
        };
        struct // ASSIGNMENT_STATEMENT
        {
            Expression *assignment_lhs;
            Expression *assignment_rhs;
        };
        struct // OUTPUT_STATEMENT / EXPRESSION_STMT / RETURN_STATEMENT
        {
            Expression *expression;
        };
    };
};

// Block

struct Block
{
    bool declaration_block;
    bool singleton_block;
    size_t symbol_table;
    StatementList statements;
};

// Parameter
DECLARE_SLICE_TYPE(Parameter, parameter)

typedef struct
{
    substr span;
    substr identity;
    Expression *type_expression;
    RhinoType type;
} Parameter;

DECLARE_LIST_TYPE(Parameter, parameter)

// Function
struct Function
{
    substr span;
    substr identity;
    Block *body;

    Expression *return_type_expression;
    bool has_return_type_expression;
    RhinoType return_type;

    ParameterSlice parameters;
};

// Program
typedef struct
{
    Allocator statement_lists;
    Allocator enum_value_lists;

    ExpressionListAllocator expression;
    FunctionListAllocator function;
    VariableListAllocator variable;
    EnumTypeListAllocator enum_type;
    StructTypeListAllocator struct_type;
    BlockListAllocator block;

    // TODO: Old allocators, yet to be replaced
    ParameterList parameter;
    ArgumentList argument;
    EnumValueList enum_value;
    PropertyList property;
    SymbolTableList symbol_table;

    Function *main;

    Block *program_block;       // Statement
    size_t global_symbol_table; // SymbolTable
} Program;

void init_program(Program *apm, Allocator *allocator);

void append_symbol(Program *apm, size_t table_index, SymbolTag symbol_tag, SymbolPointer to, substr symbol_identity);

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
RhinoType get_expression_type(Program *apm, const char *source_text, Expression *expr);
bool are_types_equal(RhinoType a, RhinoType b);
bool allow_assign_a_to_b(RhinoType a, RhinoType b);

// Expression precedence methods
ExprPrecedence precedence_of(ExpressionKind expr_kind);

#endif
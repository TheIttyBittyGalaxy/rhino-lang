#ifndef APM_H
#define APM_H

#include "macro.h"
#include "../memory.h"
#include "substr.h"
#include <stdbool.h>

// Forward Declarations

typedef struct EnumValue EnumValue;
typedef struct EnumType EnumType;
DECLARE_LIST_ALLOCATOR(EnumValue, enum_value)
DECLARE_LIST_ALLOCATOR(EnumType, enum_type)

typedef struct Property Property;
typedef struct StructType StructType;
DECLARE_LIST_ALLOCATOR(Property, property)
DECLARE_LIST_ALLOCATOR(StructType, struct_type)

typedef struct Variable Variable;
DECLARE_LIST_ALLOCATOR(Variable, variable)

typedef struct Expression Expression;
DECLARE_LIST_ALLOCATOR(Expression, expression)

typedef struct Statement Statement;
DECLARE_LIST_ALLOCATOR(Statement, statement)

typedef struct Block Block;
typedef struct SymbolTable SymbolTable;
DECLARE_LIST_ALLOCATOR(Block, block)
DECLARE_LIST_ALLOCATOR(SymbolTable, symbol_table)

typedef struct Function Function;
typedef struct Parameter Parameter;
typedef struct Argument Argument;
DECLARE_LIST_ALLOCATOR(Function, function)
DECLARE_LIST_ALLOCATOR(Parameter, parameter)
DECLARE_LIST_ALLOCATOR(Argument, argument)

typedef struct Program Program;

// Types
#define LIST_RHINO_SORTS(MACRO)                                        \
    /* The compiler has not correctly assigned a value to this sort */ \
    MACRO(INVALID_SORT)                                                \
                                                                       \
    /* The compiler as marked this sort as "to be assigned" */         \
    MACRO(UNINITIALISED_SORT)                                          \
                                                                       \
    /* This sort value cannot be determined due to user error */       \
    MACRO(ERROR_SORT)                                                  \
                                                                       \
    /* This thing does not have a type */                              \
    MACRO(SORT_NONE)                                                   \
                                                                       \
    /* Native types */                                                 \
    MACRO(SORT_BOOL)                                                   \
    MACRO(SORT_INT)                                                    \
    MACRO(SORT_NUM)                                                    \
    MACRO(SORT_STR)                                                    \
                                                                       \
    /* User types */                                                   \
    MACRO(SORT_ENUM)                                                   \
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
struct Property
{
    substr span;
    substr identity;
    Expression *type_expression;
    RhinoType type;
};

// Struct type
struct StructType
{
    substr span;
    substr identity;
    PropertyList properties;
    Block *body;
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

typedef struct
{
    SymbolTag tag;
    union
    {
        void *ptr;
        Function *function;
        Variable *variable;
        Parameter *parameter;
        EnumType *enum_type;
        StructType *struct_type;
    };
    substr identity;
} Symbol;

#define SYMBOL_TABLE_SIZE 16

struct SymbolTable
{
    size_t symbol_count;
    SymbolTable *next;
    Symbol symbol[SYMBOL_TABLE_SIZE];
};

SymbolTable *allocate_symbol_table(Allocator *allocator, SymbolTable *parent);
void declare_symbol(Program *apm, SymbolTable *table, SymbolTag tag, void *ptr, substr identity);

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
    MACRO(UNARY_NOT)                 \
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
            Parameter *parameter;
        };
        struct // TYPE_REFERENCE
        {
            RhinoType type;
        };
        struct // FUNCTION_CALL
        {
            Expression *callee;
            ArgumentList arguments;
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
    MACRO(WHILE_LOOP)              \
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
            Expression *type_expression;
            Expression *initial_value;
            bool has_valid_identity;
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
        struct // IF_SEGMENT / ELSE_IF_SEGMENT / ELSE_SEGMENT / WHILE_LOOP
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
    SymbolTable *symbol_table;
    StatementList statements;
};

// Function
struct Function
{
    substr span;
    substr identity;
    Block *body;

    Expression *return_type_expression;
    bool has_return_type_expression;
    RhinoType return_type;

    ParameterList parameters;
};

// Parameter
struct Parameter
{
    substr span;
    substr identity;
    Expression *type_expression;
    RhinoType type;
};

// Argument
struct Argument
{
    Expression *expr;
};

// Program
struct Program
{
    Allocator statement_lists;
    Allocator enum_value_lists;
    Allocator property_lists;
    Allocator parameter_lists;
    Allocator arguments_lists;

    Allocator symbol_table;

    ExpressionListAllocator expression;
    FunctionListAllocator function;
    VariableListAllocator variable;
    EnumValueList enum_value;
    EnumTypeListAllocator enum_type;
    StructTypeListAllocator struct_type;
    BlockListAllocator block;

    Function *main;

    Block *program_block;
    SymbolTable *global_symbol_table;
};

void init_program(Program *apm, Allocator *allocator);

// Display APM
const char *rhino_type_string(Program *apm, RhinoType ty);
void dump_apm(Program *apm, const char *source_text);
void print_parsed_apm(Program *apm, const char *source_text);
void print_resolved_apm(Program *apm, const char *source_text);

// Type analysis methods
RhinoType get_expression_type(Program *apm, const char *source_text, Expression *expr);
bool are_types_equal(RhinoType a, RhinoType b);
bool allow_assign_a_to_b(RhinoType a, RhinoType b);

// Expression precedence methods
ExprPrecedence precedence_of(ExpressionKind expr_kind);

#endif